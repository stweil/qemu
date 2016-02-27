/*
 * Raspberry Pi emulation (c) 2012 Gregory Estrade
 * This code is licensed under the GNU GPLv2 and later.
 */

#ifndef BCM2835_ST_H
#define BCM2835_ST_H

#include "hw/sysbus.h"
#include "qemu/timer.h"

#define TYPE_BCM2835_ST "bcm2835_st"
#define BCM2835_ST(obj) OBJECT_CHECK(BCM2835StState, (obj), TYPE_BCM2835_ST)

typedef struct BCM2835StState {
    SysBusDevice busdev;
    MemoryRegion iomem;
    QEMUTimer *timer;
    uint32_t compare[4];
    uint32_t match;
    uint32_t next;
    qemu_irq irq[4];
} BCM2835StState;

#endif
