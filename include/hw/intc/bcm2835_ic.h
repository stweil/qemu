/*
 * Raspberry Pi emulation (c) 2012 Gregory Estrade
 * This code is licensed under the GNU GPLv2 and later.
 */

#ifndef BCM2835_IC_H
#define BCM2835_IC_H

#include "hw/sysbus.h"

#define TYPE_BCM2835_IC "bcm2835_ic"
#define BCM2835_IC(obj) OBJECT_CHECK(BCM2835IcState, (obj), TYPE_BCM2835_IC)

typedef struct BCM2835IcState {
    SysBusDevice busdev;
    MemoryRegion iomem;

    uint32_t level[3];
    uint32_t irq_enable[3];
    int fiq_enable;
    int fiq_select;
    qemu_irq irq;
    qemu_irq fiq;
} BCM2835IcState;

#endif
