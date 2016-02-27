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

#define TYPE_BCM2835_AUX "bcm2835-aux"
#define BCM2835_AUX(obj) OBJECT_CHECK(BCM2835AuxState, (obj), TYPE_BCM2835_AUX)

typedef struct {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    MemoryRegion iomem;
    CharDriverState *chr;
    qemu_irq irq;

    uint32_t read_fifo[8];
    uint8_t read_pos, read_count;
    bool rx_int_enable, tx_int_enable;
} BCM2835AuxState;

#endif
