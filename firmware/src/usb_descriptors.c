/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "class/net/net_device.h"
#include "tusb.h"

/* A combination of interfaces must have a unique product id, since PC will save
 * device driver after the first plug. Same VID/PID with different interface e.g
 * MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]       NET | VENDOR | MIDI | HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n) ((CFG_TUD_##itf) << (n))
#define USB_PID                                                                \
    (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) |         \
     _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4) | _PID_MAP(ECM_RNDIS, 5) |        \
     _PID_MAP(NCM, 5))

#define EPNUM_NET_NOTIF 0x81
#define EPNUM_NET_OUT 0x02
#define EPNUM_NET_IN 0x82

#define EPNUM_HID_IN 0x83
#define EPNUM_HID_OUT 0x03

#define HID_BINTERVAL 0x01
// 1 = 1000hz, 2 = 500hz, 3 = 333hz 4 = 250hz, 5 = 200hz 6 = 166hz ...

#define USB_HID_FFB_REPORT_DESC_SIZE 1229 - 16 // 1378

// String Descriptor Index
enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
    STRID_INTERFACE_NET,
    STRID_INTERFACE_HID,
    STRID_MAC
};

enum { ITF_NUM_CDC = 0, ITF_NUM_CDC_DATA, ITF_NUM_HID, ITF_NUM_TOTAL };

enum { CONFIG_ID_RNDIS = 0, CONFIG_ID_COUNT };

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,

    // Use Interface Association Descriptor (IAD) device class
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,

    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor = 0xCafe,
    .idProduct = USB_PID,
    .bcdDevice = 0x0101,

    .iManufacturer = STRID_MANUFACTURER,
    .iProduct = STRID_PRODUCT,
    .iSerialNumber = STRID_SERIAL,

    .bNumConfigurations = CONFIG_ID_COUNT // multiple configurations
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)&desc_device;
}

//--------------------------------------------------------------------+
//  HID Desiriptor
//--------------------------------------------------------------------+

enum { REPORT_ID_KEYBOARD = 1, REPORT_ID_COUNT };

// uint8_t const desc_hid_report[] = {
//     TUD_HID_REPORT_DESC_GAMEPAD(HID_REPORT_ID(REPORT_ID_KEYBOARD))};

uint8_t const desc_hid_report[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,                    // USAGE (Game Pad)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x85, 0x01,                    //   REPORT_ID (1)
    0x05, 0x01,                    //   USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //   USAGE (X)
    0x09, 0x31,                    //   USAGE (Y)
    0x09, 0x32,                    //   USAGE (Z)
    0x09, 0x33,                    //   USAGE (Rx)
    0x09, 0x34,                    //   USAGE (Ry)
    0x09, 0x35,                    //   USAGE (Rz)
    0x16, 0x01, 0x80,              //   LOGICAL_MINIMUM (-32767)
    0x26, 0xff, 0x7f,              //   LOGICAL_MAXIMUM (32767)
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x10,                    //   REPORT_SIZE (16)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x05, 0x01,                    //   USAGE_PAGE (Generic Desktop)
    0x09, 0x39,                    //   USAGE (Hat switch)
    0x15, 0x01,                    //   LOGICAL_MINIMUM (1)
    0x25, 0x08,                    //   LOGICAL_MAXIMUM (8)
    0x35, 0x00,                    //   PHYSICAL_MINIMUM (0)
    0x46, 0x3b, 0x01,              //   PHYSICAL_MAXIMUM (315)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x05, 0x09,                    //   USAGE_PAGE (Button)
    0x19, 0x01,                    //   USAGE_MINIMUM (Button 1)
    0x29, 0x20,                    //   USAGE_MAXIMUM (Button 32)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x95, 0x20,                    //   REPORT_COUNT (32)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0xc0                           // END_COLLECTION
};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return desc_hid_report;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+
#define MAIN_CONFIG_TOTAL_LEN                                                  \
    (TUD_CONFIG_DESC_LEN + TUD_RNDIS_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

static uint8_t const rndis_configuration[] = {
    // Config number (index+1), interface count, string index, total length,
    // attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(CONFIG_ID_RNDIS + 1, ITF_NUM_TOTAL, 0,
                          MAIN_CONFIG_TOTAL_LEN, 0, 100),

    // Interface number, string index, EP notification address and size, EP data
    // address (out, in) and size.
    TUD_RNDIS_DESCRIPTOR(ITF_NUM_CDC, STRID_INTERFACE_NET, EPNUM_NET_NOTIF, 8,
                         EPNUM_NET_OUT, EPNUM_NET_IN,
                         CFG_TUD_NET_ENDPOINT_SIZE),
    // Interface number, string index, protocol, report descriptor len,
    // EP In address, size &polling interval
    TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, STRID_INTERFACE_HID,
                             HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report),
                             EPNUM_HID_OUT, EPNUM_HID_IN,
                             CFG_TUD_HID_EP_BUFSIZE, 1)};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    switch (index) {
    case 0:
        return rndis_configuration;
        break;
    default:
        return NULL;
        break;
    }
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
static char const *string_desc_arr[] = {
    [STRID_LANGID] =
        (const char[]){0x09, 0x04},   // supported language is English (0x0409)
    [STRID_MANUFACTURER] = "TinyUSB", // Manufacturer
    [STRID_PRODUCT] = "TinyUSB Device", // Product
    [STRID_SERIAL] = "123456",          // Serial
    [STRID_INTERFACE_NET] =
        "TinyUSB Network Interface", // Interface Description
    [STRID_INTERFACE_HID] = "TinyUSB Joystick"
    // STRID_MAC index is handled separately
};

static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long
// enough for transfer to complete
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;

    unsigned int chr_count = 0;

    if (STRID_LANGID == index) {
        memcpy(&_desc_str[1], string_desc_arr[STRID_LANGID], 2);
        chr_count = 1;
    } else if (STRID_MAC == index) {
        // Convert MAC address into UTF-16

        for (unsigned i = 0; i < sizeof(tud_network_mac_address); i++) {
            _desc_str[1 + chr_count++] =
                "0123456789ABCDEF"[(tud_network_mac_address[i] >> 4) & 0xf];
            _desc_str[1 + chr_count++] =
                "0123456789ABCDEF"[(tud_network_mac_address[i] >> 0) & 0xf];
        }
    } else {
        // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

        if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
            return NULL;

        const char *str = string_desc_arr[index];

        // Cap at max char
        chr_count = strlen(str);
        if (chr_count > (TU_ARRAY_SIZE(_desc_str) - 1))
            chr_count = TU_ARRAY_SIZE(_desc_str) - 1;

        // Convert ASCII string into UTF-16
        for (unsigned int i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return _desc_str;
}
