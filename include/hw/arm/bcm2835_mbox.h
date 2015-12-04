#ifndef BCM2835_MBOX_H
#define BCM2835_MBOX_H

/* Constants shared with the ARM identifying separate mailbox channels */
#define MBOX_CHAN_POWER    0 /* for use by the power management interface */
#define MBOX_CHAN_FB       1 /* for use by the frame buffer */
#define MBOX_CHAN_VCHIQ    3 /* for use by the VCHIQ interface */
#define MBOX_CHAN_PROPERTY 8 /* for use by the property channel */
#define MBOX_CHAN_COUNT    9

#define MBOX_SIZE          32
#define MBOX_INVALID_DATA  0x0f

#endif /* BCM2835_MBOX_H */
