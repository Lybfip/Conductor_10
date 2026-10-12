#ifndef TUSB_CONFIG_H_
#define TUSB_CONFIG_H_

#include "tusb_option.h"

#define CFG_TUSB_MCU               OPT_MCU_RP2040
#define CFG_TUSB_OS                OPT_OS_PICO

#define CFG_TUSB_RHPORT0_MODE      OPT_MODE_DEVICE
#define CFG_TUSB_RHPORT1_MODE      OPT_MODE_HOST

#define CFG_TUD_ENABLED            1
#define CFG_TUD_ENDPOINT0_SIZE     64
#define CFG_TUD_HID                1
#define CFG_TUD_HID_EP_BUFSIZE     16

#define CFG_TUH_ENABLED            1
#define CFG_TUH_RPI_PIO_USB        1
#define CFG_TUH_DEVICE_MAX         1
#define CFG_TUH_ENUMERATION_BUFSIZE 512

#define CFG_TUH_HUB                0
#define CFG_TUH_CDC                0
#define CFG_TUH_HID                0
#define CFG_TUH_MSC                0
#define CFG_TUH_VENDOR             0
#define CFG_TUH_CUSTOM_CLASS       0

#define CFG_TUH_API_EDPT_XFER      1

#endif /* TUSB_CONFIG_H_ */
