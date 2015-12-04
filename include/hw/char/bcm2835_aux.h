/*
 * Rasperry Pi 2 emulation and refactoring Copyright (c) 2015, Microsoft
 * Written by Andrew Baumann
 *
 * This code is licensed under the GNU GPLv2 and later.
 */

#ifndef BCM2835_AUX_H
#define BCM2835_AUX_H

#include "hw/sysbus.h"
#include "sysemu/char.h"

#define TYPE_BCM2835_AUX "bcm2835_aux"
#define BCM2835_AUX(obj) OBJECT_CHECK(BCM2835AuxState, (obj), TYPE_BCM2835_AUX)

typedef struct {
    SysBusDevice parent_obj;

    MemoryRegion iomem;
    uint32_t read_fifo[8];
    bool rx_int_enable, tx_int_enable;
    int read_pos;
    int read_count;
    CharDriverState *chr;
    qemu_irq irq;
    const unsigned char *id;
} BCM2835AuxState;

#endif
