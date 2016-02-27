/*
 * Raspberry Pi emulation (c) 2012 Gregory Estrade
 * This code is licensed under the GNU GPLv2 and later.
 */

#ifndef BCM2835_USB_H
#define BCM2835_USB_H

#include "qemu/timer.h"
#include "hw/sysbus.h"
#include "hw/usb.h"

#define BCM2835_USB_HCHANS 8

#define TYPE_BCM2835_USB "bcm2835_usb"
#define BCM2835_USB(obj) \
        OBJECT_CHECK(BCM2835UsbState, (obj), TYPE_BCM2835_USB)

typedef struct BCM2835UsbState BCM2835UsbState;

typedef struct {
    BCM2835UsbState *parent;
    int index;

    uint32_t hcchar;
    uint32_t hcsplt;
    uint32_t hcint;
    uint32_t hcintmsk;
    uint32_t hctsiz;
    uint32_t hcdma;
    uint32_t reserved;
    uint32_t hcdmab;

    USBPacket packet;
    uint8_t buffer[8192];
} BCM2835UsbHcState;

struct BCM2835UsbState {
    SysBusDevice busdev;
    MemoryRegion iomem;
    MemoryRegion *dma_mr;
    AddressSpace dma_as;

    USBBus bus;
    USBPort port;
    int attached;
    int reset_done;
    QEMUTimer *sof_timer;

    uint32_t gusbcfg;
    uint32_t hptxfsiz;
    uint32_t hcfg;
    uint32_t dcfg;
    uint32_t grxfsiz;
    uint32_t gnptxfsiz;
    uint32_t dtxfsiz[15];
    uint32_t gahbcfg;
    uint32_t grstctl;
    uint32_t gotgctl;
    uint32_t gotgint;
    uint32_t gintsts;
    uint32_t gintmsk;
    uint32_t gdfifocfg;
    uint32_t hprt0;
    uint32_t haint;
    uint32_t haintmsk;
    uint32_t gnptxsts;
    uint32_t hfnum;
    uint32_t hptxsts;
    uint32_t guid;

    BCM2835UsbHcState hchan[BCM2835_USB_HCHANS];

    qemu_irq irq;
};

#endif
