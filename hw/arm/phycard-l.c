/*
 * Phytec phyCARD-L
 *
 * Copyright (c) 2011, 2012 Stefan Weil
 *
 * ARM Cortex-A8, OMAP3530, similar to OMAP3430
 *
 */

#include "qemu/osdep.h"
#include "cpu.h"
#include "hw/sysbus.h"
#include "hw/boards.h"
#include "hw/arm/boot.h"
#include "exec/address-spaces.h" /* get_system_memory */
#include "net/net.h"

static struct arm_boot_info phycard_binfo;

static void phycard_init(MachineState *machine)
{
    ARMCPU *cpu;
    MemoryRegion *sysmem = get_system_memory();
    MemoryRegion *ram = g_new(MemoryRegion, 1);
    qemu_irq pic[64];
    DeviceState *dev;
    int i;

    if (!machine->cpu_model) {
        machine->cpu_model = "cortex-a8";
    }
    cpu = ARM_CPU(cpu_create(machine->cpu_type));

    /* RAM at address zero. */
    memory_region_allocate_system_memory(ram, NULL, "phycard.ram", ram_size);
    memory_region_add_subregion(sysmem, 0, ram);

    dev = sysbus_create_simple("syborg,interrupt", 0xC0000000,
                               qdev_get_gpio_in(DEVICE(cpu), ARM_CPU_IRQ));
    for (i = 0; i < 64; i++) {
        pic[i] = qdev_get_gpio_in(dev, i);
    }

    sysbus_create_simple("syborg,rtc", 0xC0001000, NULL);

    dev = qdev_create(NULL, "syborg,timer");
    qdev_prop_set_uint32(dev, "frequency", 1000000);
    qdev_init_nofail(dev);
    sysbus_mmio_map(SYS_BUS_DEVICE(dev), 0, 0xC0002000);
    sysbus_connect_irq(SYS_BUS_DEVICE(dev), 0, pic[1]);

    sysbus_create_simple("syborg,keyboard", 0xC0003000, pic[2]);
    sysbus_create_simple("syborg,pointer", 0xC0004000, pic[3]);
    sysbus_create_simple("syborg,framebuffer", 0xC0005000, pic[4]);
    sysbus_create_simple("syborg,serial", 0xC0006000, pic[5]);
    sysbus_create_simple("syborg,serial", 0xC0007000, pic[6]);
    sysbus_create_simple("syborg,serial", 0xC0008000, pic[7]);
    sysbus_create_simple("syborg,serial", 0xC0009000, pic[8]);

    if (nd_table[0].used || nd_table[0].netdev) {
        DeviceState *dev;
        SysBusDevice *s;

        qemu_check_nic_model(&nd_table[0], "virtio");
        dev = qdev_create(NULL, "syborg,virtio-net");
        qdev_set_nic_properties(dev, &nd_table[0]);
        qdev_init_nofail(dev);
        s = SYS_BUS_DEVICE(dev);
        sysbus_mmio_map(s, 0, 0xc000c000);
        sysbus_connect_irq(s, 0, pic[9]);
    }

    phycard_binfo.ram_size = machine->ram_size;
    phycard_binfo.kernel_filename = machine->kernel_filename;
    phycard_binfo.kernel_cmdline = machine->kernel_cmdline;
    phycard_binfo.initrd_filename = machine->initrd_filename;
    phycard_binfo.board_id = 0;
    arm_load_kernel(cpu, &phycard_binfo);
}

static void phycard_machine_init(MachineClass *mc)
{
    mc->desc = "phyCARD-L (ARM Cortex-A8)";
    mc->init = phycard_init;
};

DEFINE_MACHINE("phycard-l", phycard_machine_init)
