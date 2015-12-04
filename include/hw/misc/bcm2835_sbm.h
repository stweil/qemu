/*
 * Raspberry Pi emulation (c) 2012 Gregory Estrade
 * This code is licensed under the GNU GPLv2 and later.
 */

#ifndef BCM2835_SBM_H
#define BCM2835_SBM_H

#include "hw/sysbus.h"
#include "exec/address-spaces.h"
#include "hw/arm/bcm2835_mbox.h"

#define TYPE_BCM2835_SBM "bcm2835_sbm"
#define BCM2835_SBM(obj) \
        OBJECT_CHECK(BCM2835SbmState, (obj), TYPE_BCM2835_SBM)

typedef struct {
    MemoryRegion *mbox_mr;
    AddressSpace mbox_as;
    uint32_t reg[MBOX_SIZE];
    int count;
    uint32_t status;
    uint32_t config;
} BCM2835Mbox;

typedef struct {
    SysBusDevice busdev;
    MemoryRegion *mbox_mr;
    AddressSpace mbox_as;
    MemoryRegion iomem;
    int mbox_irq_disabled;
    qemu_irq arm_irq;
    int available[MBOX_CHAN_COUNT];
    BCM2835Mbox mbox[2];
} BCM2835SbmState;

#endif
