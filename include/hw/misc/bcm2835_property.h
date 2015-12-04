/*
 * Raspberry Pi emulation (c) 2012 Gregory Estrade
 * This code is licensed under the GNU GPLv2 and later.
 */

#ifndef BCM2835_PROPERTY_H
#define BCM2835_PROPERTY_H

#include "hw/sysbus.h"
#include "exec/address-spaces.h"
#include "hw/display/bcm2835_fb.h"

#define TYPE_BCM2835_PROPERTY "bcm2835_property"
#define BCM2835_PROPERTY(obj) \
        OBJECT_CHECK(BCM2835PropertyState, (obj), TYPE_BCM2835_PROPERTY)

typedef struct {
    SysBusDevice busdev;
    MemoryRegion *dma_mr;
    AddressSpace dma_as;
    BCM2835FbState *fbdev;
    MemoryRegion iomem;
    uint32_t addr;
    int pending;
    qemu_irq mbox_irq;
} BCM2835PropertyState;

#endif
