/*
 * Raspberry Pi emulation (c) 2012 Gregory Estrade
 * This code is licensed under the GNU GPLv2 and later.
 */

#ifndef BCM2835_EMMC_H
#define BCM2835_EMMC_H

#include "hw/sysbus.h"
#include "hw/sd/sd.h"
#include "qemu/timer.h"

#define TYPE_BCM2835_EMMC "bcm2835_emmc"
#define BCM2835_EMMC(obj) \
        OBJECT_CHECK(BCM2835EmmcState, (obj), TYPE_BCM2835_EMMC)

typedef struct {
    SysBusDevice busdev;
    MemoryRegion iomem;

    SDState *card;

    uint32_t arg2;
    uint32_t blksizecnt;
    uint32_t arg1;
    uint32_t cmdtm;
    uint32_t resp0;
    uint32_t resp1;
    uint32_t resp2;
    uint32_t resp3;
    uint32_t data;
    uint32_t status;
    uint32_t control0;
    uint32_t control1;
    uint32_t interrupt;
    uint32_t irpt_mask;
    uint32_t irpt_en;
    uint32_t control2;
    uint32_t force_irpt;
    uint32_t spi_int_spt;
    uint32_t slotisr_ver;
    uint32_t caps;
    uint32_t caps2;
    uint32_t maxcurr;
    uint32_t maxcurr2;

    int acmd;
    int write_op;

    uint32_t bytecnt;

    QEMUTimer *delay_timer;
    qemu_irq irq;
} BCM2835EmmcState;

#endif
