/*
 * Raspberry Pi emulation (c) 2012 Gregory Estrade
 * This code is licensed under the GNU GPLv2 and later.
 */

#include "hw/misc/bcm2835_sbm.h"
#include "hw/arm/bcm2835_arm_control.h"

static void mbox_update_status(BCM2835Mbox *mb)
{
    if (mb->count == 0) {
        mb->status |= ARM_MS_EMPTY;
    } else {
        mb->status &= ~ARM_MS_EMPTY;
    }
    if (mb->count == MBOX_SIZE) {
        mb->status |= ARM_MS_FULL;
    } else {
        mb->status &= ~ARM_MS_FULL;
    }
}

static void mbox_init(BCM2835Mbox *mb)
{
    int n;
    mb->count = 0;
    mb->config = 0;
    for (n = 0; n < MBOX_SIZE; n++) {
        mb->reg[n] = MBOX_INVALID_DATA;
    }
    mbox_update_status(mb);
}

static uint32_t mbox_pull(BCM2835Mbox *mb, int index)
{
    int n;
    uint32_t val;

    assert(mb->count > 0);
    assert(index < mb->count);

    val = mb->reg[index];
    for (n = index + 1; n < mb->count; n++) {
        mb->reg[n - 1] = mb->reg[n];
    }
    mb->count--;
    mb->reg[mb->count] = MBOX_INVALID_DATA;

    mbox_update_status(mb);

    return val;
}

static void mbox_push(BCM2835Mbox *mb, uint32_t val)
{
    assert(mb->count < MBOX_SIZE);
    mb->reg[mb->count++] = val;
    mbox_update_status(mb);
}

static void bcm2835_sbm_update(BCM2835SbmState *s)
{
    int set;
    int done, n;
    uint32_t value;

    /* Avoid unwanted recursive calls */
    s->mbox_irq_disabled = 1;

    /* Get pending responses and put them in the vc->arm mbox */
    done = 0;
    while (!done) {
        done = 1;
        if (s->mbox[0].status & ARM_MS_FULL) {
            /* vc->arm mbox full, exit */
        } else {
            for (n = 0; n < MBOX_CHAN_COUNT; n++) {
                if (s->available[n]) {
                    value = ldl_phys(&s->mbox_as, n<<4);
                    if (value != MBOX_INVALID_DATA) {
                        mbox_push(&s->mbox[0], value);
                    } else {
                        /* Hmmm... */
                    }
                    done = 0;
                    break;
                }
            }
        }
    }

    /* Try to push pending requests from the arm->vc mbox */
    /* TODO (?) */

    /* Re-enable calls from the IRQ routine */
    s->mbox_irq_disabled = 0;

    /* Update ARM IRQ status */
    set = 0;
    if (s->mbox[0].status & ARM_MS_EMPTY) {
        s->mbox[0].config &= ~ARM_MC_IHAVEDATAIRQPEND;
    } else {
        s->mbox[0].config |= ARM_MC_IHAVEDATAIRQPEND;
        if (s->mbox[0].config & ARM_MC_IHAVEDATAIRQEN) {
            set = 1;
        }
    }
    qemu_set_irq(s->arm_irq, set);
}

static void bcm2835_sbm_set_irq(void *opaque, int irq, int level)
{
    BCM2835SbmState *s = (BCM2835SbmState *)opaque;
    s->available[irq] = level;
    if (!s->mbox_irq_disabled) {
        bcm2835_sbm_update(s);
    }
}

static uint64_t bcm2835_sbm_read(void *opaque, hwaddr offset,
                           unsigned size)
{
    BCM2835SbmState *s = (BCM2835SbmState *)opaque;
    uint32_t res = 0;

    offset &= 0xff;

    switch (offset) {
    case 0x80:  /* MAIL0_READ */
    case 0x84:
    case 0x88:
    case 0x8c:
        if (s->mbox[0].status & ARM_MS_EMPTY) {
            res = MBOX_INVALID_DATA;
        } else {
            res = mbox_pull(&s->mbox[0], 0);
        }
        break;
    case 0x90:  /* MAIL0_PEEK */
        res = s->mbox[0].reg[0];
        break;
    case 0x94:  /* MAIL0_SENDER */
        break;
    case 0x98:  /* MAIL0_STATUS */
        res = s->mbox[0].status;
        break;
    case 0x9c:  /* MAIL0_CONFIG */
        res = s->mbox[0].config;
        break;
    case 0xb8:  /* MAIL1_STATUS */
        res = s->mbox[1].status;
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "bcm2835_sbm_read: Bad offset %x\n", (int)offset);
        return 0;
    }

    bcm2835_sbm_update(s);

    return res;
}

static void bcm2835_sbm_write(void *opaque, hwaddr offset,
                        uint64_t value, unsigned size)
{
    int ch;

    BCM2835SbmState *s = (BCM2835SbmState *)opaque;

    offset &= 0xff;

    switch (offset) {
    case 0x94:  /* MAIL0_SENDER */
        break;
    case 0x9c:  /* MAIL0_CONFIG */
        s->mbox[0].config &= ~ARM_MC_IHAVEDATAIRQEN;
        s->mbox[0].config |= value & ARM_MC_IHAVEDATAIRQEN;
        break;
    case 0xa0:
    case 0xa4:
    case 0xa8:
    case 0xac:
        if (s->mbox[1].status & ARM_MS_FULL) {
            /* Guest error */
        } else {
            ch = value & 0xf;
            if (ch < MBOX_CHAN_COUNT) {
                if (ldl_phys(&s->mbox_as, (ch<<4) + 4)) {
                    /* Push delayed, push it in the arm->vc mbox */
                    mbox_push(&s->mbox[1], value);
                } else {
                    stl_phys(&s->mbox_as, ch<<4, value);
                }
            } else {
                /* Invalid channel number */
            }
        }
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
            "bcm2835_sbm_write: Bad offset %x\n", (int)offset);
        return;
    }

    bcm2835_sbm_update(s);
}

static const MemoryRegionOps bcm2835_sbm_ops = {
    .read = bcm2835_sbm_read,
    .write = bcm2835_sbm_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription vmstate_bcm2835_sbm = {
    .name = TYPE_BCM2835_SBM,
    .version_id = 1,
    .minimum_version_id = 1,
    .minimum_version_id_old = 1,
    .fields      = (VMStateField[]) {
        VMSTATE_END_OF_LIST()
    }
};

static void bcm2835_sbm_init(Object *obj)
{
    BCM2835SbmState *s = BCM2835_SBM(obj);
    memory_region_init_io(&s->iomem, obj, &bcm2835_sbm_ops, s,
                          TYPE_BCM2835_SBM, 0x400);
    sysbus_init_mmio(SYS_BUS_DEVICE(s), &s->iomem);
    sysbus_init_irq(SYS_BUS_DEVICE(s), &s->arm_irq);
    qdev_init_gpio_in(DEVICE(s), bcm2835_sbm_set_irq, MBOX_CHAN_COUNT);
}

static void bcm2835_sbm_realize(DeviceState *dev, Error **errp)
{
    int n;
    BCM2835SbmState *s = BCM2835_SBM(dev);
    Object *obj;
    Error *err = NULL;

    obj = object_property_get_link(OBJECT(dev), "mbox_mr", &err);
    if (err || obj == NULL) {
        error_setg(errp, "bcm2835_sbm: required mbox_mr link not found");
        return;
    }

    s->mbox_mr = MEMORY_REGION(obj);
    address_space_init(&s->mbox_as, s->mbox_mr, NULL);

    mbox_init(&s->mbox[0]);
    mbox_init(&s->mbox[1]);
    s->mbox_irq_disabled = 0;
    for (n = 0; n < MBOX_CHAN_COUNT; n++) {
        s->available[n] = 0;
    }
}

static void bcm2835_sbm_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->realize = bcm2835_sbm_realize;
    dc->vmsd = &vmstate_bcm2835_sbm;
}

static TypeInfo bcm2835_sbm_info = {
    .name          = TYPE_BCM2835_SBM,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(BCM2835SbmState),
    .class_init    = bcm2835_sbm_class_init,
    .instance_init = bcm2835_sbm_init,
};

static void bcm2835_sbm_register_types(void)
{
    type_register_static(&bcm2835_sbm_info);
}

type_init(bcm2835_sbm_register_types)
