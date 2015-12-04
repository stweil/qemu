/*
 * Raspberry Pi emulation (c) 2012 Gregory Estrade
 * This code is licensed under the GNU GPLv2 and later.
 */

#ifndef BCM2835_VCHIQ_H
#define BCM2835_VCHIQ_H

#include "hw/sysbus.h"

#define TYPE_BCM2835_VCHIQ "bcm2835_vchiq"
#define BCM2835_VCHIQ(obj) \
        OBJECT_CHECK(BCM2835VchiqState, (obj), TYPE_BCM2835_VCHIQ)

typedef struct {
    SysBusDevice busdev;
    MemoryRegion iomem;
    int pending;
    qemu_irq mbox_irq;
} BCM2835VchiqState;

#endif
