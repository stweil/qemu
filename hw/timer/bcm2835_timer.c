/*
 * Raspberry Pi emulation (c) 2012 Gregory Estrade
 * This code is licensed under the GNU GPLv2 and later.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/timer/bcm2835_timer.h"
#include "qemu/main-loop.h"

#define SYSCLOCK_FREQ (252000000)
#define APBCLOCK_FREQ (126000000)

#define CTRL_FRC_EN (1 << 9)
#define CTRL_TIMER_EN (1 << 7)
#define CTRL_IRQ_EN (1 << 5)
#define CTRL_PS_MASK (3 << 2)
#define CTRL_PS_SHIFT 2
#define CTRL_CNT_32 (1 << 1)
#define CTRL_FRC_PS_MASK (0xff << 16)
#define CTRL_FRC_PS_SHIFT 16

static void timer_tick(void *opaque)
{
    BCM2835TimerState *s = (BCM2835TimerState *)opaque;
    s->raw_irq = 1;
    if (s->control & CTRL_IRQ_EN) {
        qemu_set_irq(s->irq, 1);
    }
}
static void frc_timer_tick(void *opaque)
{
    BCM2835TimerState *s = (BCM2835TimerState *)opaque;
    s->frc_value++;
}

static uint64_t bcm2835_timer_read(void *opaque, hwaddr offset,
    unsigned size)
{
    BCM2835TimerState *s = (BCM2835TimerState *)opaque;
    uint32_t res = 0;

    assert(size == 4);

    switch (offset) {
    case 0x0:
        res = s->load;
        break;
    case 0x4:
        res = ptimer_get_count(s->timer);
        break;
    case 0x8:
        res = s->control;
        break;
    case 0xc:
        res = 0x544d5241;
        break;
    case 0x10:
        res = s->raw_irq;
        break;
    case 0x14:
        if (s->control & CTRL_IRQ_EN) {
            res = s->raw_irq;
        }
        break;
    case 0x18:
        res = s->load;
        break;
    case 0x1c:
        res = s->prediv;
        break;
    case 0x20:
        res = s->frc_value;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "bcm2835_timer_read: Bad offset %x\n", (int)offset);
        return 0;
    }

    return res;
}

static void bcm2835_timer_write(void *opaque, hwaddr offset,
    uint64_t value, unsigned size)
{
    BCM2835TimerState *s = (BCM2835TimerState *)opaque;
    uint32_t freq;

    assert(size == 4);

    switch (offset) {
    case 0x0:
        s->load = value;
        ptimer_set_limit(s->timer, s->load, 1);
        break;
    case 0x4:
        break;
    case 0x8:
        if (s->control & CTRL_FRC_EN) {
            ptimer_stop(s->frc_timer);
        }
        if (s->control & CTRL_TIMER_EN) {
            ptimer_stop(s->timer);
        }
        s->control = value & 0x00ff03ae;

        freq = SYSCLOCK_FREQ;
        ptimer_set_freq(s->frc_timer, freq);
        ptimer_set_limit(s->frc_timer,
            ((s->control & CTRL_FRC_PS_MASK) >> CTRL_FRC_PS_SHIFT) + 1,
            s->control & CTRL_FRC_EN);

        freq = APBCLOCK_FREQ;
        freq /= s->prediv + 1;
        switch ((s->control & CTRL_PS_MASK) >> CTRL_PS_SHIFT) {
        case 1:
            freq >>= 4;
            break;
        case 2:
            freq >>= 8;
            break;
        default:
            break;
        }
        ptimer_set_freq(s->timer, freq);
        ptimer_set_limit(s->timer, s->load, s->control & CTRL_TIMER_EN);

        if (s->control & CTRL_TIMER_EN) {
            ptimer_run(s->timer, 0);
        }
        if (s->control & CTRL_FRC_EN) {
            s->frc_value++;
            ptimer_run(s->frc_timer, 0);
        }
        break;
    case 0xc:
        s->raw_irq = 0;
        qemu_set_irq(s->irq, 0);
        break;
    case 0x10:
    case 0x14:
        break;
    case 0x18:
        s->load = value;
        ptimer_set_limit(s->timer, s->load, 0);
        break;
    case 0x1c:
        s->prediv = value & 0x3ff;
        break;
    case 0x20:
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "bcm2835_timer_write: Bad offset %x\n", (int)offset);
        return;
    }
}

static const MemoryRegionOps bcm2835_timer_ops = {
    .read = bcm2835_timer_read,
    .write = bcm2835_timer_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription vmstate_bcm2835_timer = {
    .name = TYPE_BCM2835_TIMER,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields      = (VMStateField[]) {
        VMSTATE_END_OF_LIST()
    }
};

static void bcm2835_timer_init(Object *obj)
{
    BCM2835TimerState *s = BCM2835_TIMER(obj);

    memory_region_init_io(&s->iomem, obj, &bcm2835_timer_ops, s,
                          TYPE_BCM2835_TIMER, 0x100);
    sysbus_init_mmio(SYS_BUS_DEVICE(s), &s->iomem);
    sysbus_init_irq(SYS_BUS_DEVICE(s), &s->irq);
}

static void bcm2835_timer_realize(DeviceState *dev, Error **errp)
{
    BCM2835TimerState *s = BCM2835_TIMER(dev);
    QEMUBH *bh;

    s->load = 0;
    s->control = 0x3e << 16;
    s->raw_irq = 0;
    s->prediv = 0x7d;

    bh = qemu_bh_new(timer_tick, s);
    s->timer = ptimer_init(bh, PTIMER_POLICY_DEFAULT);

    bh = qemu_bh_new(frc_timer_tick, s);
    s->frc_timer = ptimer_init(bh, PTIMER_POLICY_DEFAULT);
}

static void bcm2835_timer_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = bcm2835_timer_realize;
    dc->vmsd = &vmstate_bcm2835_timer;
}

static TypeInfo bcm2835_timer_info = {
    .name          = TYPE_BCM2835_TIMER,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(BCM2835TimerState),
    .class_init    = bcm2835_timer_class_init,
    .instance_init = bcm2835_timer_init,
};

static void bcm2835_timer_register_types(void)
{
    type_register_static(&bcm2835_timer_info);
}

type_init(bcm2835_timer_register_types)
