/*
 * Rasperry Pi 2 emulation ARM control logic module.
 * Copyright (c) 2015, Microsoft
 * Written by Andrew Baumann
 *
 * At present, only implements interrupt routing, and mailboxes (i.e.,
 * not local timer, PMU interrupt, or AXI counters).
 *
 * Ref:
 * https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf
 *
 * Based on bcm2835_ic.c, terms below...
 */

/*
 * Raspberry Pi emulation (c) 2012 Gregory Estrade
 * This code is licensed under the GNU GPLv2 and later.
 */

/* Heavily based on pl190.c, copyright terms below. */

/*
 * Arm PrimeCell PL190 Vector Interrupt Controller
 *
 * Copyright (c) 2006 CodeSourcery.
 * Written by Paul Brook
 *
 * This code is licensed under the GPL.
 */

#include "hw/intc/bcm2836_control.h"

#define ROUTE_CORE(x) ((x) & 0x3)
#define ROUTE_FIQ(x)  (((x) & 0x4) != 0)

#define IRQ_BIT(cntrl, num) (((cntrl) & (1 << (num))) != 0)
#define FIQ_BIT(cntrl, num) (((cntrl) & (1 << ((num) + 4))) != 0)

#define IRQ_CNTPSIRQ    0
#define IRQ_CNTPNSIRQ   1
#define IRQ_CNTHPIRQ    2
#define IRQ_CNTVIRQ     3
#define IRQ_MAILBOX0    4
#define IRQ_MAILBOX1    5
#define IRQ_MAILBOX2    6
#define IRQ_MAILBOX3    7
#define IRQ_GPU         8
#define IRQ_PMU         9
#define IRQ_AXI         10
#define IRQ_TIMER       11
#define IRQ_MAX         IRQ_TIMER

/* Update interrupts.  */
static void bcm2836_control_update(BCM2836ControlState *s)
{
    int i, j;

    /*
     * reset pending IRQs/FIQs
     */

    for (i = 0; i < BCM2836_NCORES; i++) {
        s->irqsrc[i] = s->fiqsrc[i] = 0;
    }

    /*
     * apply routing logic, update status regs
     */

    if (s->gpu_irq) {
        assert(s->route_gpu_irq < BCM2836_NCORES);
        s->irqsrc[s->route_gpu_irq] |= (uint32_t)1 << IRQ_GPU;
    }

    if (s->gpu_fiq) {
        assert(s->route_gpu_fiq < BCM2836_NCORES);
        s->fiqsrc[s->route_gpu_fiq] |= (uint32_t)1 << IRQ_GPU;
    }

    for (i = 0; i < BCM2836_NCORES; i++) {
        /* handle local interrupts for this core */
        if (s->localirqs[i]) {
            assert(s->localirqs[i] < (1 << IRQ_MAILBOX0));
            for (j = 0; j < IRQ_MAILBOX0; j++) {
                if ((s->localirqs[i] & (1 << j)) != 0) {
                    /* local interrupt j is set */
                    if (FIQ_BIT(s->timercontrol[i], j)) {
                        /* deliver a FIQ */
                        s->fiqsrc[i] |= (uint32_t)1 << j;
                    } else if (IRQ_BIT(s->timercontrol[i], j)) {
                        /* deliver an IRQ */
                        s->irqsrc[i] |= (uint32_t)1 << j;
                    } else {
                        /* the interrupt is masked */
                    }
                }
            }
        }

        /* handle mailboxes for this core */
        for (j = 0; j < BCM2836_MBPERCORE; j++) {
            if (s->mailboxes[i * BCM2836_MBPERCORE + j] != 0) {
                /* mailbox j is set */
                if (FIQ_BIT(s->mailboxcontrol[i], j)) {
                    /* deliver a FIQ */
                    s->fiqsrc[i] |= (uint32_t)1 << (j + IRQ_MAILBOX0);
                } else if (IRQ_BIT(s->mailboxcontrol[i], j)) {
                    /* deliver an IRQ */
                    s->irqsrc[i] |= (uint32_t)1 << (j + IRQ_MAILBOX0);
                } else {
                    /* the interrupt is masked */
                }
            }
        }
    }

    /*
     * call set_irq appropriately for each output
     */

    for (i = 0; i < BCM2836_NCORES; i++) {
        qemu_set_irq(s->irq[i], s->irqsrc[i] != 0);
        qemu_set_irq(s->fiq[i], s->fiqsrc[i] != 0);
    }
}

static void bcm2836_control_set_local_irq(void *opaque, int core, int local_irq,
                                          int level)
{
    BCM2836ControlState *s = (BCM2836ControlState *)opaque;

    assert(core >= 0 && core < BCM2836_NCORES);
    assert(local_irq >= 0 && local_irq <= IRQ_CNTVIRQ);

    if (level) {
        s->localirqs[core] |= 1 << local_irq;
    } else {
        s->localirqs[core] &= ~((uint32_t)1 << local_irq);
    }

    bcm2836_control_update(s);
}

/* XXX: the following wrapper functions are a kludgy workaround,
 * needed because I can't seem to pass useful information in the "irq"
 * parameter when using named interrupts. Feel free to clean this up!
 */

static void bcm2836_control_set_local_irq0(void *opaque, int core, int level)
{
    bcm2836_control_set_local_irq(opaque, core, 0, level);
}

static void bcm2836_control_set_local_irq1(void *opaque, int core, int level)
{
    bcm2836_control_set_local_irq(opaque, core, 1, level);
}

static void bcm2836_control_set_local_irq2(void *opaque, int core, int level)
{
    bcm2836_control_set_local_irq(opaque, core, 2, level);
}

static void bcm2836_control_set_local_irq3(void *opaque, int core, int level)
{
    bcm2836_control_set_local_irq(opaque, core, 3, level);
}

static void bcm2836_control_set_gpu_irq(void *opaque, int irq, int level)
{
    BCM2836ControlState *s = (BCM2836ControlState *)opaque;

    s->gpu_irq = level;

    bcm2836_control_update(s);
}

static void bcm2836_control_set_gpu_fiq(void *opaque, int irq, int level)
{
    BCM2836ControlState *s = (BCM2836ControlState *)opaque;

    s->gpu_fiq = level;

    bcm2836_control_update(s);
}

static uint64_t bcm2836_control_read(void *opaque, hwaddr offset,
    unsigned size)
{
    BCM2836ControlState *s = (BCM2836ControlState *)opaque;

    if (offset == 0xc) {
        /* GPU interrupt routing */
        assert(s->route_gpu_fiq < BCM2836_NCORES
               && s->route_gpu_irq < BCM2836_NCORES);
        return ((uint32_t)s->route_gpu_fiq << 2) | s->route_gpu_irq;
    } else if (offset >= 0x40 && offset < 0x50) {
        /* Timer interrupt control registers */
        return s->timercontrol[(offset - 0x40) >> 2];
    } else if (offset >= 0x50 && offset < 0x60) {
        /* Mailbox interrupt control registers */
        return s->mailboxcontrol[(offset - 0x50) >> 2];
    } else if (offset >= 0x60 && offset < 0x70) {
        /* IRQ source registers */
        return s->irqsrc[(offset - 0x60) >> 2];
    } else if (offset >= 0x70 && offset < 0x80) {
        /* FIQ source registers */
        return s->fiqsrc[(offset - 0x70) >> 2];
    } else if (offset >= 0xc0 && offset < 0x100) {
        /* Mailboxes */
        return s->mailboxes[(offset - 0xc0) >> 2];
    } else {
        qemu_log_mask(LOG_GUEST_ERROR,
            "bcm2836_control_read: Bad offset %x\n", (int)offset);
        return 0;
    }
}

static void bcm2836_control_write(void *opaque, hwaddr offset,
    uint64_t val, unsigned size)
{
    BCM2836ControlState *s = (BCM2836ControlState *)opaque;

    if (offset == 0xc) {
        /* GPU interrupt routing */
        s->route_gpu_irq = val & 0x3;
        s->route_gpu_fiq = (val >> 2) & 0x3;
    } else if (offset >= 0x40 && offset < 0x50) {
        /* Timer interrupt control registers */
        s->timercontrol[(offset - 0x40) >> 2] = val & 0xff;
    } else if (offset >= 0x50 && offset < 0x60) {
        /* Mailbox interrupt control registers */
        s->mailboxcontrol[(offset - 0x50) >> 2] = val & 0xff;
    } else if (offset >= 0x80 && offset < 0xc0) {
        /* Mailbox set registers */
        s->mailboxes[(offset - 0x80) >> 2] |= val;
    } else if (offset >= 0xc0 && offset < 0x100) {
        /* Mailbox clear registers */
        s->mailboxes[(offset - 0xc0) >> 2] &= ~val;
    } else {
        qemu_log_mask(LOG_GUEST_ERROR,
            "bcm2836_control_write: Bad offset %x\n", (int)offset);
        return;
    }

    bcm2836_control_update(s);
}

static const MemoryRegionOps bcm2836_control_ops = {
    .read = bcm2836_control_read,
    .write = bcm2836_control_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static void bcm2836_control_reset(DeviceState *d)
{
    BCM2836ControlState *s = BCM2836_CONTROL(d);
    int i;

    s->route_gpu_irq = s->route_gpu_fiq = 0;

    for (i = 0; i < BCM2836_NCORES; i++) {
        s->timercontrol[i] = 0;
        s->mailboxcontrol[i] = 0;
    }

    for (i = 0; i < BCM2836_NCORES * BCM2836_MBPERCORE; i++) {
        s->mailboxes[i] = 0;
    }
}

static void bcm2836_control_init(Object *obj)
{
    BCM2836ControlState *s = BCM2836_CONTROL(obj);
    DeviceState *dev = DEVICE(obj);

    memory_region_init_io(&s->iomem, obj, &bcm2836_control_ops, s,
                          TYPE_BCM2836_CONTROL, 0x100);
    sysbus_init_mmio(SYS_BUS_DEVICE(s), &s->iomem);

    /* inputs from each CPU core */
    qdev_init_gpio_in_named(dev, bcm2836_control_set_local_irq0, "cntpsirq",
                            BCM2836_NCORES);
    qdev_init_gpio_in_named(dev, bcm2836_control_set_local_irq1, "cntpnsirq",
                            BCM2836_NCORES);
    qdev_init_gpio_in_named(dev, bcm2836_control_set_local_irq2, "cnthpirq",
                            BCM2836_NCORES);
    qdev_init_gpio_in_named(dev, bcm2836_control_set_local_irq3, "cntvirq",
                            BCM2836_NCORES);
    /* qdev_init_gpio_in_named(dev, bcm2836_control_set_pmu_irq, "pmuirq",
                            BCM2836_NCORES); */

    /* IRQ and FIQ inputs from upstream bcm2835 controller */
    qdev_init_gpio_in_named(dev, bcm2836_control_set_gpu_irq, "gpu_irq", 1);
    qdev_init_gpio_in_named(dev, bcm2836_control_set_gpu_fiq, "gpu_fiq", 1);

    /* outputs to CPU cores */
    qdev_init_gpio_out_named(dev, s->irq, "irq", BCM2836_NCORES);
    qdev_init_gpio_out_named(dev, s->fiq, "fiq", BCM2836_NCORES);
}

static void bcm2836_control_realize(DeviceState *dev, Error **errp)
{
}

static const VMStateDescription vmstate_bcm2836_control = {
    .name = TYPE_BCM2836_CONTROL,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(mailboxes, BCM2836ControlState,
                             BCM2836_NCORES * BCM2836_MBPERCORE),
        VMSTATE_UINT8(route_gpu_irq, BCM2836ControlState),
        VMSTATE_UINT8(route_gpu_fiq, BCM2836ControlState),
        VMSTATE_UINT32_ARRAY(timercontrol, BCM2836ControlState, BCM2836_NCORES),
        VMSTATE_UINT32_ARRAY(mailboxcontrol, BCM2836ControlState,
                             BCM2836_NCORES),
        VMSTATE_END_OF_LIST()
    }
};

static void bcm2836_control_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = bcm2836_control_realize;
    dc->reset = bcm2836_control_reset;
    dc->vmsd = &vmstate_bcm2836_control;
}

static TypeInfo bcm2836_control_info = {
    .name          = TYPE_BCM2836_CONTROL,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(BCM2836ControlState),
    .class_init    = bcm2836_control_class_init,
    .instance_init = bcm2836_control_init,
};

static void bcm2836_control_register_types(void)
{
    type_register_static(&bcm2836_control_info);
}

type_init(bcm2836_control_register_types)
