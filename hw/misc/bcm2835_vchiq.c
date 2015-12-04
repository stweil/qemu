/*
 * Raspberry Pi emulation (c) 2012 Gregory Estrade
 * This code is licensed under the GNU GPLv2 and later.
 */

#include "hw/misc/bcm2835_vchiq.h"
#include "hw/arm/bcm2835_mbox.h"

static uint64_t bcm2835_vchiq_read(void *opaque, hwaddr offset,
    unsigned size)
{
    BCM2835VchiqState *s = (BCM2835VchiqState *)opaque;
    uint32_t res = 0;

    switch (offset) {
    case 0:
        res = MBOX_CHAN_VCHIQ;
        s->pending = 0;
        qemu_set_irq(s->mbox_irq, 0);
        break;
    case 4:
        res = s->pending;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "bcm2835_vchiq_read: Bad offset %x\n", (int)offset);
        return 0;
    }
    return res;
}

static void bcm2835_vchiq_write(void *opaque, hwaddr offset,
    uint64_t value, unsigned size)
{
    BCM2835VchiqState *s = (BCM2835VchiqState *)opaque;
    switch (offset) {
    case 0:
        s->pending = 1;
        qemu_set_irq(s->mbox_irq, 1);
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "bcm2835_vchiq_write: Bad offset %x\n", (int)offset);
        return;
    }

}

static const MemoryRegionOps bcm2835_vchiq_ops = {
    .read = bcm2835_vchiq_read,
    .write = bcm2835_vchiq_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription vmstate_bcm2835_vchiq = {
    .name = TYPE_BCM2835_VCHIQ,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields      = (VMStateField[]) {
        VMSTATE_END_OF_LIST()
    }
};

static void bcm2835_vchiq_init(Object *obj)
{
    BCM2835VchiqState *s = BCM2835_VCHIQ(obj);

    sysbus_init_irq(SYS_BUS_DEVICE(s), &s->mbox_irq);
    memory_region_init_io(&s->iomem, obj, &bcm2835_vchiq_ops, s,
                          TYPE_BCM2835_VCHIQ, 0x10);
    sysbus_init_mmio(SYS_BUS_DEVICE(s), &s->iomem);
}

static void bcm2835_vchiq_realize(DeviceState *dev, Error **errp)
{
    BCM2835VchiqState *s = BCM2835_VCHIQ(dev);

    s->pending = 0;
}

static void bcm2835_vchiq_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = bcm2835_vchiq_realize;
    dc->vmsd = &vmstate_bcm2835_vchiq;
}

static TypeInfo bcm2835_vchiq_info = {
    .name          = TYPE_BCM2835_VCHIQ,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(BCM2835VchiqState),
    .class_init    = bcm2835_vchiq_class_init,
    .instance_init = bcm2835_vchiq_init,
};

static void bcm2835_vchiq_register_types(void)
{
    type_register_static(&bcm2835_vchiq_info);
}

type_init(bcm2835_vchiq_register_types)
