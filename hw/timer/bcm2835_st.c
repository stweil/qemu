/*
 * Raspberry Pi emulation (c) 2012 Gregory Estrade
 * This code is licensed under the GNU GPLv2 and later.
 */

/* Based on several timers code found in various QEMU source files. */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/timer/bcm2835_st.h"

static void bcm2835_st_update(BCM2835StState *s)
{
    int64_t now = qemu_clock_get_us(QEMU_CLOCK_VIRTUAL);
    uint32_t clo = (uint32_t)now;
    uint32_t delta;
    bool set = false;
    int i;

    /* Calculate new "next" value and reschedule */
    for (i = 0; i < 4; i++) {
        if (!(s->match & (1 << i))) {
            if (!set || s->compare[i] - clo < delta) {
                set = true;
                s->next = s->compare[i];
                delta = s->next - clo;
            }
        }
    }

    if (set) {
        timer_mod(s->timer, now + delta);
    } else {
        timer_del(s->timer);
    }
}

static void bcm2835_st_tick(void *opaque)
{
    BCM2835StState *s = (BCM2835StState *)opaque;
    int i;

    /* Trigger irqs for current "next" value */
    for (i = 0; i < 4; i++) {
        if (!(s->match & (1 << i)) && (s->next == s->compare[i])) {
            s->match |= (1 << i);
            qemu_set_irq(s->irq[i], 1);
        }
    }

    bcm2835_st_update(s);
}

static uint64_t bcm2835_st_read(void *opaque, hwaddr offset,
                           unsigned size)
{
    BCM2835StState *s = (BCM2835StState *)opaque;
    uint32_t res = 0;
    uint64_t now = qemu_clock_get_us(QEMU_CLOCK_VIRTUAL);

    assert(size == 4);

    switch (offset) {
    case 0x00:
        res = s->match;
        break;
    case 0x04:
        res = (uint32_t)now;
        /* Ugly temporary hack to get Plan9 to boot... */
        /* see http://plan9.bell-labs.com/sources/contrib/ \
         * miller/rpi/sys/src/9/bcm/clock.c */
        /* res = (now / 10000) * 10000; */
        break;
    case 0x08:
        res = (now >> 32);
        break;
    case 0x0c:
        res = s->compare[0];
        break;
    case 0x10:
        res = s->compare[1];
        break;
    case 0x14:
        res = s->compare[2];
        break;
    case 0x18:
        res = s->compare[3];
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "bcm2835_st_read: Bad offset %x\n", (int)offset);
        return 0;
    }

    return res;
}

static void bcm2835_st_write(void *opaque, hwaddr offset,
                        uint64_t value, unsigned size)
{
    BCM2835StState *s = (BCM2835StState *)opaque;
    int i;

    assert(size == 4);

    switch (offset) {
    case 0x00:
        s->match &= ~value & 0x0f;
        for (i = 0; i < 4; i++) {
            if (value & (1 << i)) {
                qemu_set_irq(s->irq[i], 0);
            }
        }
        break;
    case 0x0c:
        s->compare[0] = value;
        break;
    case 0x10:
        s->compare[1] = value;
        break;
    case 0x14:
        s->compare[2] = value;
        break;
    case 0x18:
        s->compare[3] = value;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "bcm2835_st_write: Bad offset %x\n", (int)offset);
        return;
    }
    bcm2835_st_update(s);
}

static const MemoryRegionOps bcm2835_st_ops = {
    .read = bcm2835_st_read,
    .write = bcm2835_st_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription vmstate_bcm2835_st = {
    .name = TYPE_BCM2835_ST,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields      = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(compare, BCM2835StState, 4),
        VMSTATE_UINT32(match, BCM2835StState),
        VMSTATE_END_OF_LIST()
    }
};

static void bcm2835_st_init(Object *obj)
{
    BCM2835StState *s = BCM2835_ST(obj);
    int i;

    for (i = 0; i < 4; i++) {
        sysbus_init_irq(SYS_BUS_DEVICE(s), &s->irq[i]);
    }

    memory_region_init_io(&s->iomem, obj, &bcm2835_st_ops, s,
                          TYPE_BCM2835_ST, 0x20);
    sysbus_init_mmio(SYS_BUS_DEVICE(s), &s->iomem);
}

static void bcm2835_st_realize(DeviceState *dev, Error **errp)
{
    BCM2835StState *s = BCM2835_ST(dev);
    int i;

    for (i = 0; i < 4; i++) {
        s->compare[i] = 0;
    }
    s->match = 0;
    s->timer = timer_new_us(QEMU_CLOCK_VIRTUAL, bcm2835_st_tick, s);

    bcm2835_st_update(s);
}

static void bcm2835_st_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = bcm2835_st_realize;
    dc->vmsd = &vmstate_bcm2835_st;
}

static TypeInfo bcm2835_st_info = {
    .name          = TYPE_BCM2835_ST,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(BCM2835StState),
    .class_init    = bcm2835_st_class_init,
    .instance_init = bcm2835_st_init,
};

static void bcm2835_st_register_types(void)
{
    type_register_static(&bcm2835_st_info);
}

type_init(bcm2835_st_register_types)
