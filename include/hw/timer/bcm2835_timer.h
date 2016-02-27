/*
 * Raspberry Pi emulation (c) 2012 Gregory Estrade
 * This code is licensed under the GNU GPLv2 and later.
 */

#ifndef BCM2835_TIMER_H
#define BCM2835_TIMER_H

#include "hw/sysbus.h"
#include "hw/ptimer.h"

#define TYPE_BCM2835_TIMER "bcm2835_timer"
#define BCM2835_TIMER(obj) \
        OBJECT_CHECK(BCM2835TimerState, (obj), TYPE_BCM2835_TIMER)

typedef struct {
    SysBusDevice busdev;
    MemoryRegion iomem;

    qemu_irq irq;

    uint32_t load;
    uint32_t control;
    uint32_t raw_irq;
    uint32_t prediv;
    uint32_t frc_value;

    ptimer_state *timer;
    ptimer_state *frc_timer;
} BCM2835TimerState;

#endif
