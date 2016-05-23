/*
 * Raspberry Pi emulation (c) 2012 Gregory Estrade
 * This code is licensed under the GNU GPLv2 and later.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "hw/misc/bcm2835_power.h"
#include "hw/misc/bcm2835_mbox_defs.h"

static uint64_t bcm2835_power_read(void *opaque, hwaddr offset, unsigned size)
{
    BCM2835PowerState *s = (BCM2835PowerState *)opaque;
    uint32_t res = 0;

    switch (offset) {
    case 0:
        res = MBOX_CHAN_POWER;
        s->pending = 0;
        qemu_set_irq(s->mbox_irq, 0);
        break;
    case 4:
        res = s->pending;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "bcm2835_power_read: Bad offset %x\n", (int)offset);
        return 0;
    }
    return res;
}

static void bcm2835_power_write(void *opaque, hwaddr offset, uint64_t value,
                                unsigned size)
{
    BCM2835PowerState *s = (BCM2835PowerState *)opaque;
    switch (offset) {
    case 0:
        s->pending = 1;
        qemu_set_irq(s->mbox_irq, 1);
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "bcm2835_power_write: Bad offset %x\n", (int)offset);
        return;
    }

}

static const MemoryRegionOps bcm2835_power_ops = {
    .read = bcm2835_power_read,
    .write = bcm2835_power_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription vmstate_bcm2835_power = {
    .name = TYPE_BCM2835_POWER,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields      = (VMStateField[]) {
        VMSTATE_END_OF_LIST()
    }
};

static void bcm2835_power_init(Object *obj)
{
    BCM2835PowerState *s = BCM2835_POWER(obj);

    sysbus_init_irq(SYS_BUS_DEVICE(s), &s->mbox_irq);
    memory_region_init_io(&s->iomem, obj, &bcm2835_power_ops, s,
                          TYPE_BCM2835_POWER, 0x10);
    sysbus_init_mmio(SYS_BUS_DEVICE(s), &s->iomem);
}

static void bcm2835_power_realize(DeviceState *dev, Error **errp)
{
    BCM2835PowerState *s = BCM2835_POWER(dev);

    s->pending = 0;
}

static void bcm2835_power_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = bcm2835_power_realize;
    dc->vmsd = &vmstate_bcm2835_power;
}

static TypeInfo bcm2835_power_info = {
    .name          = TYPE_BCM2835_POWER,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(BCM2835PowerState),
    .class_init    = bcm2835_power_class_init,
    .instance_init = bcm2835_power_init,
};

static void bcm2835_power_register_types(void)
{
    type_register_static(&bcm2835_power_info);
}

type_init(bcm2835_power_register_types)
