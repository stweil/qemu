/*
 * Raspberry Pi emulation (c) 2012 Gregory Estrade
 * This code is licensed under the GNU GPLv2 and later.
 */

#ifndef BCM2835_POWER_H
#define BCM2835_POWER_H

#include "hw/sysbus.h"

#define TYPE_BCM2835_POWER "bcm2835-power"
#define BCM2835_POWER(obj) \
        OBJECT_CHECK(BCM2835PowerState, (obj), TYPE_BCM2835_POWER)

typedef struct {
    SysBusDevice busdev;
    MemoryRegion iomem;
    int pending;
    qemu_irq mbox_irq;
} BCM2835PowerState;

#endif
