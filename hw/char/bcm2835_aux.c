/*
 * BCM2835 (Raspberry Pi / Pi 2) Aux block (mini UART and SPI).
 * Copyright (c) 2015, Microsoft
 * Written by Andrew Baumann
 *
 * Fairly hacky. Based on gutted code for pl011 emulation (copyright below).
 */

/*
 * Arm PrimeCell PL011 UART
 *
 * Copyright (c) 2006 CodeSourcery.
 * Written by Paul Brook
 *
 * This code is licensed under the GPL.
 */

#include "hw/char/bcm2835_aux.h"

static void bcm2835_aux_update(BCM2835AuxState *s)
{
    bool status = (s->rx_int_enable && s->read_count != 0) || s->tx_int_enable;
    qemu_set_irq(s->irq, status);
}

static uint64_t bcm2835_aux_read(void *opaque, hwaddr offset,
                           unsigned size)
{
    BCM2835AuxState *s = (BCM2835AuxState *)opaque;
    uint32_t c, res;

    switch (offset >> 2) {
    case 1: /* AUXENB */
        return 1; /* mini UART enabled */

    case 16: /* AUX_MU_IO_REG */
        c = s->read_fifo[s->read_pos];
        if (s->read_count > 0) {
            s->read_count--;
            if (++s->read_pos == 8) {
                s->read_pos = 0;
            }
        }
        if (s->chr) {
            qemu_chr_accept_input(s->chr);
        }
        bcm2835_aux_update(s);
        return c;

    case 17: /* AUX_MU_IIR_REG */
        res = 0;
        if (s->rx_int_enable) {
            res |= 0x2;
        }
        if (s->tx_int_enable) {
            res |= 0x1;
        }
        return res;

    case 18: /* AUX_MU_IER_REG */
        res = 0xc0;
        if (s->tx_int_enable) {
            res |= 0x1;
        } else if (s->rx_int_enable && s->read_count != 0) {
            res |= 0x2;
        }
        return res;

    case 21: /* AUX_MU_LSR_REG */
        res = 0x60; /* tx idle, empty */
        if (s->read_count != 0) {
            res |= 0x1;
        }
        return res;

    case 25: /* AUX_MU_STAT_REG */
        res = 0x302; /* space in the output buffer, empty tx fifo */
        if (s->read_count > 0) {
            res |= 0x1; /* data in input buffer */
            assert(s->read_count < 8);
            res |= ((uint32_t)s->read_count) << 16; /* rx fifo fill level */
        }
        return res;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "bcm2835_aux_read: Bad offset %x\n", (int)offset);
        return 0;
    }
}

static void bcm2835_aux_write(void *opaque, hwaddr offset,
                              uint64_t value, unsigned size)
{
    BCM2835AuxState *s = (BCM2835AuxState *)opaque;
    unsigned char ch;

    switch (offset >> 2) {
    case 1: /* AUXENB */
        if (value != 1) {
            qemu_log_mask(LOG_GUEST_ERROR,
                          "bcm2835_aux_write: Trying to enable SPI or"
                          " disable UART. Not supported!\n");
        }
        break;

    case 16: /* AUX_MU_IO_REG */
        ch = value;
        if (s->chr) {
            qemu_chr_fe_write(s->chr, &ch, 1);
        }
        break;

    case 17: /* AUX_MU_IIR_REG */
        s->rx_int_enable = (value & 0x2) != 0;
        s->tx_int_enable = (value & 0x1) != 0;
        break;

    case 18: /* AUX_MU_IER_REG */
        if (value & 0x1) {
            s->read_count = 0;
        }
        break;

    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "bcm2835_aux_write: Bad offset %x\n", (int)offset);
    }

    bcm2835_aux_update(s);
}

static int bcm2835_aux_can_receive(void *opaque)
{
    BCM2835AuxState *s = (BCM2835AuxState *)opaque;

    return s->read_count < 8;
}

static void bcm2835_aux_put_fifo(void *opaque, uint32_t value)
{
    BCM2835AuxState *s = (BCM2835AuxState *)opaque;
    int slot;

    slot = s->read_pos + s->read_count;
    if (slot >= 8) {
        slot -= 8;
    }
    s->read_fifo[slot] = value;
    s->read_count++;
    if (s->read_count == 8) {
        /* buffer full */
    }
    bcm2835_aux_update(s);
}

static void bcm2835_aux_receive(void *opaque, const uint8_t *buf, int size)
{
    bcm2835_aux_put_fifo(opaque, *buf);
}

static void bcm2835_aux_event(void *opaque, int event)
{
    if (event == CHR_EVENT_BREAK) {
        bcm2835_aux_put_fifo(opaque, 0x400);
    }
}

static const MemoryRegionOps bcm2835_aux_ops = {
    .read = bcm2835_aux_read,
    .write = bcm2835_aux_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};

static const VMStateDescription vmstate_bcm2835_aux = {
    .name = "bcm2835_aux",
    .version_id = 2,
    .minimum_version_id = 2,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32_ARRAY(read_fifo, BCM2835AuxState, 8),
        VMSTATE_INT32(read_pos, BCM2835AuxState),
        VMSTATE_INT32(read_count, BCM2835AuxState),
        VMSTATE_END_OF_LIST()
    }
};

static void bcm2835_aux_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    BCM2835AuxState *s = BCM2835_AUX(obj);

    memory_region_init_io(&s->iomem, OBJECT(s), &bcm2835_aux_ops, s,
                          "bcm2835_aux", 0x100);
    sysbus_init_mmio(sbd, &s->iomem);
    sysbus_init_irq(sbd, &s->irq);
}

static void bcm2835_aux_realize(DeviceState *dev, Error **errp)
{
    BCM2835AuxState *s = BCM2835_AUX(dev);

    /* FIXME use a qdev chardev prop instead of qemu_char_get_next_serial() */
    s->chr = qemu_char_get_next_serial();

    if (s->chr) {
        qemu_chr_add_handlers(s->chr, bcm2835_aux_can_receive,
                              bcm2835_aux_receive, bcm2835_aux_event, s);
    }
}

static void bcm2835_aux_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->realize = bcm2835_aux_realize;
    dc->vmsd = &vmstate_bcm2835_aux;
    /* Reason: realize() method uses qemu_char_get_next_serial() */
    dc->cannot_instantiate_with_device_add_yet = true;
}

static const TypeInfo bcm2835_aux_info = {
    .name          = TYPE_BCM2835_AUX,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(BCM2835AuxState),
    .instance_init = bcm2835_aux_init,
    .class_init    = bcm2835_aux_class_init,
};

static void bcm2835_aux_register_types(void)
{
    type_register_static(&bcm2835_aux_info);
}

type_init(bcm2835_aux_register_types)
