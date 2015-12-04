/*
 * Raspberry Pi emulation (c) 2012 Gregory Estrade
 * Upstreaming code cleanup [including bcm2835_*] (c) 2013 Jan Petrous
 *
 * Rasperry Pi 2 emulation Copyright (c) 2015, Microsoft
 * Written by Andrew Baumann
 *
 * This code is licensed under the GNU GPLv2 and later.
 */

/* Based on versatilepb.c, copyright terms below. */

/*
 * ARM Versatile Platform/Application Baseboard System emulation.
 *
 * Copyright (c) 2005-2007 CodeSourcery.
 * Written by Paul Brook
 *
 * This code is licensed under the GPL.
 */

#include "hw/arm/bcm2835.h"
#include "hw/arm/bcm2836.h"
#include "qemu/error-report.h"
#include "hw/boards.h"
#include "hw/loader.h"
#include "hw/arm/arm.h"
#include "sysemu/sysemu.h"
#include "hw/arm/raspi_platform.h"

#define SMPBOOT_ADDR    0x300 /* this should leave enough space for ATAGS */
#define MVBAR_ADDR      0x400 /* secure vectors */
#define BOARDSETUP_ADDR (MVBAR_ADDR + 0x20) /* board setup code */
#define FIRMWARE_ADDR   0x8000 /* Pi loads kernel.img here by default */

/* Table of Linux board IDs for different Pi versions */
static const int raspi_boardid[] = {[1] = 0xc42, [2] = 0xc43};

typedef struct RaspiMachineState {
    union {
        Object obj;
        BCM2835State pi1;
        BCM2836State pi2;
    } soc;
    MemoryRegion ram;
} RaspiMachineState;

static void write_smpboot(ARMCPU *cpu, const struct arm_boot_info *info)
{
    static const uint32_t smpboot[] = {
        0xE1A0E00F, /*    mov     lr, pc */
        0xE3A0FE42, /*    mov     pc, #0x420           ;call BOARDSETUP_ADDR */
        0xEE100FB0, /*    mrc     p15, 0, r0, c0, c0, 5;get core ID */
        0xE7E10050, /*    ubfx    r0, r0, #0, #2       ;extract LSB */
        0xE59F5014, /*    ldr     r5, =0x400000CC      ;load mbox base */
        0xE320F001, /* 1: yield */
        0xE7953200, /*    ldr     r3, [r5, r0, lsl #4] ;read mbox for our core*/
        0xE3530000, /*    cmp     r3, #0               ;spin while zero */
        0x0AFFFFFB, /*    beq     1b */
        0xE7853200, /*    str     r3, [r5, r0, lsl #4] ;clear mbox */
        0xE12FFF13, /*    bx      r3                   ;jump to target */
        0x400000CC, /* (constant: mailbox 3 read/clear base) */
    };

    assert(SMPBOOT_ADDR + sizeof(smpboot) <= MVBAR_ADDR);
    rom_add_blob_fixed("raspi_smpboot", smpboot, sizeof(smpboot),
                       info->smp_loader_start);
}

static void write_board_setup(ARMCPU *cpu, const struct arm_boot_info *info)
{
    static const uint32_t board_setup[] = {
        /* MVBAR_ADDR: secure monitor vectors */
        0xEAFFFFFE, /* (spin) */
        0xEAFFFFFE, /* (spin) */
        0xE1B0F00E, /* movs pc, lr ;SMC exception return */
        0xEAFFFFFE, /* (spin) */
        0xEAFFFFFE, /* (spin) */
        0xEAFFFFFE, /* (spin) */
        0xEAFFFFFE, /* (spin) */
        0xEAFFFFFE, /* (spin) */
        /* BOARDSETUP_ADDR */
        0xE3A00B01, /* mov     r0, #0x400             ;MVBAR_ADDR */
        0xEE0C0F30, /* mcr     p15, 0, r0, c12, c0, 1 ;set MVBAR */
        0xE3000131, /* movw    r0, #0x131             ;enable HVC, AW, FW, NS */
        0xEE010F11, /* mcr     p15, 0, r0, c1, c1, 0  ;write SCR */
        0xE1A0100E, /* mov     r1, lr                 ;save LR across SMC */
        0xE1600070, /* smc     #0                     ;monitor call */
        0xE1A0F001, /* mov     pc, r1                 ;return */
    };

    rom_add_blob_fixed("raspi_boardsetup", board_setup, sizeof(board_setup),
                       MVBAR_ADDR);
}

static void reset_secondary(ARMCPU *cpu, const struct arm_boot_info *info)
{
    CPUState *cs = CPU(cpu);
    cpu_set_pc(cs, info->smp_loader_start);
}

static void setup_boot(MachineState *machine, int version, size_t ram_size)
{
    static struct arm_boot_info binfo;
    int r;

    binfo.board_id = raspi_boardid[version];
    binfo.ram_size = ram_size;
    binfo.nb_cpus = smp_cpus;

    /* Pi2 supports security extensions, which require special setup code */
    if (version == 2) {
        binfo.smp_loader_start = SMPBOOT_ADDR,
        binfo.write_secondary_boot = write_smpboot,
        binfo.secondary_cpu_reset_hook = reset_secondary,
        binfo.board_setup_addr = BOARDSETUP_ADDR;
        binfo.write_board_setup = write_board_setup;
        binfo.secure_board_setup = true;
        binfo.secure_boot = true;
    }

    /* If the user specified a "firmware" image (e.g. UEFI), we bypass
       the normal Linux boot process */
    if (machine->firmware) {
        /* load the firmware image (typically kernel.img) */
        r = load_image_targphys(machine->firmware, FIRMWARE_ADDR,
                                ram_size - FIRMWARE_ADDR);
        if (r < 0) {
            error_report("Failed to load firmware from %s", machine->firmware);
            exit(1);
        }

        /* set variables so arm_load_kernel does the right thing */
        binfo.entry = FIRMWARE_ADDR;
        binfo.firmware_loaded = true;
    } else {
        /* Just let arm_load_kernel do everything for us... */
        binfo.kernel_filename = machine->kernel_filename;
        binfo.kernel_cmdline = machine->kernel_cmdline;
        binfo.initrd_filename = machine->initrd_filename;
    }

    arm_load_kernel(ARM_CPU(first_cpu), &binfo);
}

static void raspi_machine_init(MachineState *machine, int version)
{
    RaspiMachineState *s = g_new0(RaspiMachineState, 1);
    uint32_t vcram_size;

    /* Initialise the relevant SOC */
    assert(version == 1 || version == 2);
    switch (version) {
    case 1:
        object_initialize(&s->soc.pi1, sizeof(s->soc.pi1), TYPE_BCM2835);
        break;
    case 2:
        object_initialize(&s->soc.pi2, sizeof(s->soc.pi2), TYPE_BCM2836);
        break;
    }

    object_property_add_child(OBJECT(machine), "soc", &s->soc.obj,
                              &error_abort);

    /* Allocate and map RAM */
    memory_region_allocate_system_memory(&s->ram, OBJECT(machine), "ram",
                                         machine->ram_size);
    memory_region_add_subregion_overlap(get_system_memory(), 0, &s->ram, 0);

    /* Setup the SOC */
    object_property_set_bool(&s->soc.obj, true, "realized", &error_abort);

    vcram_size = object_property_get_int(&s->soc.obj, "vcram-size",
                                         &error_abort);

    /* Boot! */
    setup_boot(machine, version, machine->ram_size - vcram_size);
}

static void raspi1_init(MachineState *machine)
{
    raspi_machine_init(machine, 1);
}

static void raspi2_init(MachineState *machine)
{
    raspi_machine_init(machine, 2);
}

static void raspi1_machine_init(MachineClass *mc)
{
    mc->desc = "Raspberry Pi";
    mc->init = raspi1_init;
    mc->block_default_type = IF_SD;
    mc->no_parallel = 1;
    mc->no_floppy = 1;
    mc->no_cdrom = 1;
    mc->default_ram_size = 512 * 1024 * 1024;
};
DEFINE_MACHINE("raspi", raspi1_machine_init)

static void raspi2_machine_init(MachineClass *mc)
{
    raspi1_machine_init(mc);
    mc->desc = "Raspberry Pi 2";
    mc->init = raspi2_init;
    mc->max_cpus = BCM2836_NCPUS;
    mc->default_ram_size = 1024 * 1024 * 1024;
};
DEFINE_MACHINE("raspi2", raspi2_machine_init)
