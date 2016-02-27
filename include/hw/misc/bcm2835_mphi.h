/*
 * Raspberry Pi emulation (c) 2012 Gregory Estrade
 * This code is licensed under the GNU GPLv2 and later.
 */

#ifndef BCM2835_MPHI_H
#define BCM2835_MPHI_H

#include "hw/sysbus.h"

#define TYPE_BCM2835_MPHI "bcm2835-mphi"
#define BCM2835_MPHI(obj) \
        OBJECT_CHECK(BCM2835MphiState, (obj), TYPE_BCM2835_MPHI)

typedef struct {
    SysBusDevice busdev;
    MemoryRegion iomem;

    uint32_t mphi_base;
    uint32_t mphi_ctrl;
    uint32_t mphi_outdda;
    uint32_t mphi_outddb;
    uint32_t mphi_intstat;

    qemu_irq irq;
} BCM2835MphiState;

#endif
