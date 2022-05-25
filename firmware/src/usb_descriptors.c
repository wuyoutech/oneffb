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

#include "ffb_defs.h"

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

    /* joystick definitions */

    0x05, 0x01,       // USAGE_PAGE (Generic Desktop)
    0x09, 0x04,       // USAGE (Joystick)
    0xa1, 0x01,       // COLLECTION (Application)
    0xa1, 0x01,       // COLLECTION (Application)
    0x85, 0x01,       //   REPORT_ID (1)
    0x05, 0x02,       //   USAGE_PAGE (Simulation Controls)
    0x09, 0xc4,       //   USAGE (Accelerator)
    0x09, 0xc5,       //   USAGE (Brake)
    0x09, 0xc6,       //   USAGE (Clutch)
    0x09, 0xc8,       //   USAGE (Steering)
    0x16, 0x01, 0x80, //   LOGICAL_MINIMUM (-32767)
    0x26, 0xff, 0x7f, //   LOGICAL_MAXIMUM (32767)
    0x95, 0x04,       //   REPORT_COUNT (4)
    0x75, 0x10,       //   REPORT_SIZE (16)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)
    0x05, 0x09,       //   USAGE_PAGE (Button)
    0x19, 0x01,       //   USAGE_MINIMUM (Button 1)
    0x29, 0x20,       //   USAGE_MAXIMUM (Button 32)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x25, 0x01,       //   LOGICAL_MAXIMUM (1)
    0x95, 0x20,       //   REPORT_COUNT (32)
    0x75, 0x01,       //   REPORT_SIZE (1)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)
    0xc0,             // END_COLLECTION

    // /* start force feedback command definitions */

    // 0x05, 0x0f, // USAGE_PAGE (Physical Interface)
    // 0x09, 0x21, // USAGE (Set Effect Report)
    // 0xa1, 0x02, // COLLECTION (Logical)
    // 0x09, 0x22, //   USAGE (Effect Block Index)
    // 0x25, 0x7f, //   LOGICAL_MAXIMUM (127)
    // 0x75, 0x07, //   REPORT_SIZE (7)
    // 0x95, 0x01, //   REPORT_COUNT (1)
    // 0x91, 0x02, //   OUTPUT (Data, Var, Abs)
    // 0x09, 0x24, //   USAGE (ROM FLAG)
    // 0x25, 0x01, //   Logical_MAXIMUM (1)
    // 0x75, 0x01, //   REPORT_SIZE (1)
    // 0x91, 0x02, //   OUTPUT (Data, Var, Abs)
    // /* Deficne the available effect types. */
    // 0x09, 0x25, //   USAGE (Effect Type)
    // 0xa1, 0x02, //   COLLECTION (Logical)
    // 0x09, 0x26, //     USAGE (ET Constant Force)
    // 0x09, 0x27, //     USAGE (ET Ramp)
    // 0x09, 0x30, //     USAGE (ET Square)
    // 0x09, 0x31, //     USAGE (ET Sine)
    // 0x09, 0x32, //     USAGE (ET Triangle)
    // 0x09, 0x33, //     USAGE (ET Sawtooth Up)
    // 0x09, 0x34, //     USAGE (ET Sawtooth Down)
    // 0x09, 0x40, //     USAGE (ET Spring)
    // 0x09, 0x41, //     USAGE (ET Damper)
    // 0x09, 0x42, //     USAGE (ET Inertia)
    // 0x15, 0x01, //     LOGICAL_MINIMUM (1)
    // 0x25, 0x0A, //     LOGICAL_MAXMINUM (10)
    // 0x75, 0x08, //     REPORT_SIZE (8)
    // 0x91, 0x00, //     OUTPUT (Data, Var, Abs)
    // 0xc0,       //   END_COLLECTION

    // 0x09, 0x50, //   USAGE (Duration)
    // 0x09, 0x54, //   USAGE (Trigger Repeat Interval)
    // 0x15, 0x00, //   LOGICAL_MINIMUM (0)
    // 0x26,0x10,0x27, //  LOGICAL_MAXIMUM (10000)
    // 0x46,0x10,0x27,// PHYSICAL_MAXIMUM (10000)
    // 0x75,0x10,// REPORT_SIZE (16)
    // 0x66,0x03,0x10,// UNIT (Eng Lin: Time)
    // 0x55,0x0d,// UNIT_EXPONENT (-3)
    // 0x95,0x02,// REPORT_COUNT (2)
    // 0x91,0x02, //
    // 0x55,0x0a,
    // 0x09,0x51,
    // 0x95,0x01,
    // 0x91,0x02,
    // 0x45,0x00,
    // 0x55,0x00,
    // 0x65,0x00,
    // 0x09,0x52,
    // 0x09,0x53,
    // 0x25,0x7f,
    // 0x75,0x08,
    // 0x95,0x02,
    // 0x91,0x02,
    // 0x09,0x55,
    // 0xa1,0x02,
    // 0x05,0x01,
    // 0x09,0x01,
    // 0xa1,0x00,
    // 0x09,0x30,
    // 0x09,0x31,
    // 0x25,0x01,
    // 0x75,0x01,
    // 0x95,0x02,
    // 0x91,0x02,
    // 0xc0,
    // 0xc0,

    /* BEGIN PID effects */
    0x05, 0x0F,                          //    Usage Page Physical Interface
    0x09, 0x92,                          //    Usage PID State report
    0xA1, 0x02,                          //    Collection Datalink (logical)
    0x85, 0x02,                          //      Report ID 2
    0x09, 0x9F,                          //      Usage Device is Pause
    0x09, 0xA0,                          //      Usage Actuators Enabled
    0x09, 0xA4,                          //      Usage Safety Switch
    0x09, 0xA6,                          //      Usage Actuator Power
    0x09, 0x94,                          //      Usage Effect Playing
    0x15, 0x00,                          //      Logical Minimum 0
    0x25, 0x01,                          //      Logical Maximum 1
    0x35, 0x00,                          //      Physical Minimum 0
    0x45, 0x01,                          //      Physical Maximum 1
    0x75, 0x01,                          //      Report Size 1
    0x95, 0x05,                          //      Report Count 4
    0x81, 0x02,                          //      Input (Variable)
    0x95, 0x03,                          //      Report Count 3
    0x81, 0x03,                          //      Input (Constant, Variable)
    0xC0,                                //    End Collection
    0x09, 0x21,                          //    Usage Set Effect Report
    0xA1, 0x02,                          //    Collection Datalink (Logical)
    0x85, HID_ID_EFFREP + FFB_ID_OFFSET, //      Report ID 1
    0x09, 0x22,                          //      Usage Effect Block Index
    0x15, 0x01,                          //      Logical Minimum 1
    0x25, MAX_EFFECTS,                   //      Logical Maximum 28h (40d)
    0x35, 0x01,                          //      Physical Minimum 1
    0x45, MAX_EFFECTS,                   //      Physical Maximum 28h (40d)
    0x75, 0x08,                          //      Report Size 8
    0x95, 0x01,                          //      Report Count 1
    0x91, 0x02,                          //      Output (Variable)
    0x09, 0x25,                          //      Usage Effect Type
    0xA1, 0x02,                          //      Collection Datalink
    0x09, HID_USAGE_CONST,               //      Usage ET Constant Force
    0x09, HID_USAGE_RAMP,                //      Usage ET Ramp
    0x09, HID_USAGE_SQUR,                //      Usage ET Square
    0x09, HID_USAGE_SINE,                //      Usage ET Sine
    0x09, HID_USAGE_TRNG,                //      Usage ET Triangle
    0x09, HID_USAGE_STUP,                //      Usage ET Sawtooth Up
    0x09, HID_USAGE_STDN,                //      Usage ET Sawtooth Down
    0x09, HID_USAGE_SPRNG,               //      Usage ET Spring
    0x09, HID_USAGE_DMPR,                //      Usage ET Damper
    0x09, HID_USAGE_INRT,                //      Usage ET Inertia
    0x09, HID_USAGE_FRIC,                //      Usage ET Friction
    0x09, 0x28,                          //    Usage ET Custom Force Data
    0x25, 0x0B,                          //    Logical Maximum Ch (11d)
    0x15, 0x01,                          //    Logical Minimum 1
    0x35, 0x01,                          //    Physical Minimum 1
    0x45, 0x0B,                          //    Physical Maximum Ch (11d)
    0x75, 0x08,                          //    Report Size 8
    0x95, 0x01,                          //    Report Count 1
    0x91, 0x00,                          //    Output
    0xC0,                                //    End Collection
    0x09, 0x50,                          //    Usage Duration
    0x09, 0x54,                          //    Usage Trigger Repeat Interval
    0x09, 0x51,                          //    Usage Sample Period
    0x09, 0xA7,                          //    Usage Start Delay
    0x15, 0x00,                          //    Logical Minimum 0
    0x26, 0xFF, 0x7F,                    //    Logical Maximum 7FFFh (32767d)
    0x35, 0x00,                          //    Physical Minimum 0
    0x46, 0xFF, 0x7F,                    //    Physical Maximum 7FFFh (32767d)
    0x66, 0x03, 0x10,                    //    Unit 1003h (4099d)
    0x55, 0xFD,                          //    Unit Exponent FDh (253d)
    0x75, 0x10,                          //    Report Size 10h (16d)
    0x95, 0x04,                          //    Report Count 4
    0x91, 0x02,                          //    Output (Variable)
    0x55, 0x00,                          //    Unit Exponent 0
    0x66, 0x00, 0x00,                    //    Unit 0
    0x09, 0x52,                          //    Usage Gain
    0x15, 0x00,                          //    Logical Minimum 0
    0x26, 0xFF, 0x00, //    Logical Maximum FFh (255d) // TODO scaling?
    0x35, 0x00,       //    Physical Minimum 0
    0x46, 0x10, 0x27, //    Physical Maximum 2710h (10000d)
    0x75, 0x08,       //    Report Size 8
    0x95, 0x01,       //    Report Count 1
    0x91, 0x02,       //    Output (Variable)
    0x09, 0x53,       //    Usage Trigger Button
    0x15, 0x01,       //    Logical Minimum 1
    0x25, 0x08,       //    Logical Maximum 8
    0x35, 0x01,       //    Physical Minimum 1
    0x45, 0x08,       //    Physical Maximum 8
    0x75, 0x08,       //    Report Size 8
    0x95, 0x01,       //    Report Count 1
    0x91, 0x02,       //    Output (Variable)

    0x09, 0x55, //    Usage Axes Enable TODO multi axis
    0xA1, 0x02, //    Collection Datalink
    0x05, 0x01, //    Usage Page Generic Desktop
    0x09, 0x30, //    Usage X
    0x09, 0x31, //    Usage Y
    0x15, 0x00, //    Logical Minimum 0
    0x25, 0x01, //    Logical Maximum 1
    0x75, 0x01, //    Report Size 1
    0x95, 0x02, //    Report Count 2
    0x91, 0x02, //    Output (Variable)
    0xC0,       // End Collection
    0x05, 0x0F, //    Usage Page Physical Interface
    0x09, 0x56, //    Usage Direction Enable
    0x95, 0x01, //    Report Count 1
    0x91, 0x02, //    Output (Variable)
    0x95, 0x05, //    Report Count 5
    0x91, 0x03, //    Output (Constant, Variable)

    0x09, 0x57,                   //    Usage Direction
    0xA1, 0x02,                   //    Collection Datalink
    0x0B, 0x01, 0x00, 0x0A, 0x00, //    Usage Ordinals: Instance 1
    0x0B, 0x02, 0x00, 0x0A, 0x00, //    Usage Ordinals: Instance 2
    0x66, 0x14, 0x00,             //    Unit 14h (20d)
    //			      0x55,0xFE,                   //    Unit Exponent FEh
    //(254d) 			      0x15,0x00,                   //    Logical Minimum
    // 0 			      0x26,0xFF,0x00,              //    Logical Maximum FFh
    // (255d)
    0x15, 0x00,                   //    Logical Minimum 0
    0x27, 0xA0, 0x8C, 0x00, 0x00, //    Logical Maximum 8CA0h (36000d)
    0x35, 0x00,                   //    Physical Minimum 0
    0x47, 0xA0, 0x8C, 0x00, 0x00, //    Physical Maximum 8CA0h (36000d)
    0x66, 0x00, 0x00,             //    Unit 0
    0x75, 0x10,                   //    Report Size 16
    0x95, 0x02,                   //    Report Count 2
    0x91, 0x02,                   //    Output (Variable)
    0x55, 0x00,                   //    Unit Exponent 0
    0x66, 0x00, 0x00,             //    Unit 0
    0xC0,                         //    End Collection

    0x05, 0x0F,                   //     USAGE_PAGE (Physical Interface)
    0x09, 0x58,                   //     USAGE (Type Specific Block Offset)
    0xA1, 0x02,                   //     COLLECTION (Logical)
    0x0B, 0x01, 0x00, 0x0A, 0x00, // USAGE (Ordinals:Instance 1
    0x0B, 0x02, 0x00, 0x0A, 0x00, // USAGE (Ordinals:Instance 2)
    0x26, 0xFD, 0x7F, //   LOGICAL_MAXIMUM (32765) ; 32K RAM or ROM max.
    0x75, 0x10,       //     REPORT_SIZE (16)
    0x95, 0x02,       //     REPORT_COUNT (2)
    0x91, 0x02,       //     OUTPUT (Data,Var,Abs)
    0xC0,             //     END_COLLECTION
    0xC0,             //     END_COLLECTION

    // Envelope Report Definition
    0x09, 0x5A,                          //    Usage Set Envelope Report
    0xA1, 0x02,                          //    Collection Datalink
    0x85, HID_ID_ENVREP + FFB_ID_OFFSET, //    Report ID 2
    0x09, 0x22,                          //    Usage Effect Block Index
    0x15, 0x01,                          //    Logical Minimum 1
    0x25, MAX_EFFECTS,                   //    Logical Maximum 28h (40d)
    0x35, 0x01,                          //    Physical Minimum 1
    0x45, MAX_EFFECTS,                   //    Physical Maximum 28h (40d)
    0x75, 0x08,                          //    Report Size 8
    0x95, 0x01,                          //    Report Count 1
    0x91, 0x02,                          //    Output (Variable)
    0x09, 0x5B,                          //    Usage Attack Level
    0x09, 0x5D,                          //    Usage Fade Level
    0x16, 0x00, 0x00,                    //    Logical Minimum 0
    0x26, 0xFF, 0x7F,                    //    Logical Maximum 7FFFh (32767d)
    0x36, 0x00, 0x00,                    //    Physical Minimum 0
    0x46, 0xFF, 0x7F,                    //    Physical Maximum 7FFFh (32767d)
    0x75, 0x10,                          //    Report Size 16
    0x95, 0x02,                          //    Report Count 2
    0x91, 0x02,                          //    Output (Variable)
    0x09, 0x5C,                          //    Usage Attack Time
    0x09, 0x5E,                          //    Usage Fade Time
    0x66, 0x03, 0x10, //    Unit 1003h (English Linear, Seconds)
    0x55, 0xFD,       //    Unit Exponent FDh (X10^-3 ==> Milisecond)
    0x27, 0xFF, 0x7F, 0x00, 0x00, //    Logical Maximum FFFFFFFFh (4294967295)
    0x47, 0xFF, 0x7F, 0x00, 0x00, //    Physical Maximum FFFFFFFFh (4294967295)
    0x75, 0x20,                   //    Report Size 20h (32d)
    0x91, 0x02,                   //    Output (Variable)
    0x45, 0x00,                   //    Physical Maximum 0
    0x66, 0x00, 0x00,             //    Unit 0
    0x55, 0x00,                   //    Unit Exponent 0
    0xC0,                         //    End Collection
    0x09, 0x5F,                   //    Usage Set Condition Report
    0xA1, 0x02,                   //    Collection Datalink
    0x85, HID_ID_CONDREP + FFB_ID_OFFSET, //    Report ID 3
    0x09, 0x22,                           //    Usage Effect Block Index
    0x15, 0x01,                           //    Logical Minimum 1
    0x25, MAX_EFFECTS,                    //    Logical Maximum 28h (40d)
    0x35, 0x01,                           //    Physical Minimum 1
    0x45, MAX_EFFECTS,                    //    Physical Maximum 28h (40d)
    0x75, 0x08,                           //    Report Size 8
    0x95, 0x01,                           //    Report Count 1
    0x91, 0x02,                           //    Output (Variable)
    0x09, 0x23,                           //    Usage Parameter Block Offset
    0x15, 0x00,                           //    Logical Minimum 0
    0x25, 0x03,                           //    Logical Maximum 3
    0x35, 0x00,                           //    Physical Minimum 0
    0x45, 0x03,                           //    Physical Maximum 3
    0x75, 0x04,                           //    Report Size 4
    0x95, 0x01,                           //    Report Count 1
    0x91, 0x02,                           //    Output (Variable)
    0x09, 0x58,                           //    Usage Type Specific Block Off...
    0xA1, 0x02,                           //    Collection Datalink
    0x0B, 0x01, 0x00, 0x0A, 0x00,         //    Usage Ordinals: Instance 1
    0x0B, 0x02, 0x00, 0x0A, 0x00,         //    Usage Ordinals: Instance 2
    0x75, 0x02,                           //    Report Size 2
    0x95, 0x02,                           //    Report Count 2
    0x91, 0x02,                           //    Output (Variable)
    0xC0,                                 //    End Collection
    0x16, 0x00, 0x80,                     //    Logical Minimum 7FFFh (-32767d)
    0x26, 0xff, 0x7f,                     //    Logical Maximum 7FFFh (32767d)
    0x36, 0x00, 0x80,                     //    Physical Minimum 7FFFh (-32767d)
    0x46, 0xff, 0x7f,                     //    Physical Maximum 7FFFh (32767d)

    0x09, 0x60,                           //    Usage CP Offset
    0x75, 0x10,                           //    Report Size 16
    0x95, 0x01,                           //    Report Count 1
    0x91, 0x02,                           //    Output (Variable)
    0x36, 0x00, 0x80,                     //    Physical Minimum  (-32768)
    0x46, 0xff, 0x7f,                     //    Physical Maximum  (32767)
    0x09, 0x61,                           //    Usage Positive Coefficient
    0x09, 0x62,                           //    Usage Negative Coefficient
    0x95, 0x02,                           //    Report Count 2
    0x91, 0x02,                           //    Output (Variable)
    0x16, 0x00, 0x00,                     //    Logical Minimum 0
    0x26, 0xff, 0x7f,                     //    Logical Maximum  (32767)
    0x36, 0x00, 0x00,                     //    Physical Minimum 0
    0x46, 0xff, 0x7f,                     //    Physical Maximum  (32767)
    0x09, 0x63,                           //    Usage Positive Saturation
    0x09, 0x64,                           //    Usage Negative Saturation
    0x75, 0x10,                           //    Report Size 16
    0x95, 0x02,                           //    Report Count 2
    0x91, 0x02,                           //    Output (Variable)
    0x09, 0x65,                           //    Usage Dead Band
    0x46, 0xff, 0x7f,                     //    Physical Maximum (32767)
    0x95, 0x01,                           //    Report Count 1
    0x91, 0x02,                           //    Output (Variable)
    0xC0,                                 //    End Collection
    0x09, 0x6E,                           //    Usage Set Periodic Report
    0xA1, 0x02,                           //    Collection Datalink
    0x85, HID_ID_PRIDREP + FFB_ID_OFFSET, //    Report ID 4
    0x09, 0x22,                           //    Usage Effect Block Index
    0x15, 0x01,                           //    Logical Minimum 1
    0x25, MAX_EFFECTS,                    //    Logical Maximum 28h (40d)
    0x35, 0x01,                           //    Physical Minimum 1
    0x45, MAX_EFFECTS,                    //    Physical Maximum 28h (40d)
    0x75, 0x08,                           //    Report Size 8
    0x95, 0x01,                           //    Report Count 1
    0x91, 0x02,                           //    Output (Variable)
    0x09, 0x70,                           //    Usage Magnitude
    0x16, 0x00, 0x00,                     //    Logical Minimum 0
    0x26, 0xff, 0x7f,                     //    Logical Maximum 7FFFh (32767d)
    0x36, 0x00, 0x00,                     //    Physical Minimum 0
    0x26, 0xff, 0x7f,                     //    Logical Maximum 7FFFh (32767d)
    0x75, 0x10,                           //    Report Size 16
    0x95, 0x01,                           //    Report Count 1
    0x91, 0x02,                           //    Output (Variable)
    0x09, 0x6F,                           //    Usage Offset
    0x16, 0x00, 0x80,                     //    Logical Minimum 7FFFh (-32767d)
    0x26, 0xff, 0x7f,                     //    Logical Maximum 7FFFh (32767d)
    0x36, 0x00, 0x80,                     //    Physical Minimum 7FFFh (-32767d)
    0x46, 0xff, 0x7f,                     //    Physical Maximum 7FFFh (32767d)
    0x95, 0x01,                           //    Report Count 1
    0x75, 0x10,                           //    Report Size 16
    0x91, 0x02,                           //    Output (Variable)
    0x09, 0x71,                           //    Usage Phase
    0x66, 0x14, 0x00,                     //    Unit 14h (Eng Rotation, Degrees)
    0x55, 0xFE,                           //    Unit Exponent FEh (X10^-2)
    0x15, 0x00,                           //    Logical Minimum 0
    0x27, 0x9F, 0x8C, 0x00, 0x00,         //    Logical Maximum 8C9Fh (35999d)
    0x35, 0x00,                           //    Physical Minimum 0
    0x47, 0x9F, 0x8C, 0x00, 0x00,         //    Physical Maximum 8C9Fh (35999d)
    0x75, 0x10,                           //    Report Size 16
    0x95, 0x01,                           //    Report Count 1
    0x91, 0x02,                           //    Output (Variable)
    0x09, 0x72,                           //    Usage Period
    0x15, 0x01,                           //    Logical Minimum 1
    0x27, 0xFF, 0x7F, 0x00, 0x00,         //    Logical Maximum 7FFFh (32K)
    0x35, 0x01,                           //    Physical Minimum 1
    0x47, 0xFF, 0x7F, 0x00, 0x00,         //    Physical Maximum 7FFFh (32K)
    0x66, 0x03, 0x10, //    Unit 1003h (English Linear, Seconds)
    0x55, 0xFD,       //    Unit Exponent FDh (X10^-3 ==> Milisecond)
    0x75, 0x20,       //    Report Size 20h (32)
    0x95, 0x01,       //    Report Count 1
    0x91, 0x02,       //    Output (Variable)
    0x66, 0x00, 0x00, //    Unit 0
    0x55, 0x00,       //    Unit Exponent 0
    0xC0,             // End Collection
    0x09, 0x73,       //    Usage Set Constant Force Report
    0xA1, 0x02,       //    Collection Datalink
    0x85, HID_ID_CONSTREP + FFB_ID_OFFSET, //    Report ID 5
    0x09, 0x22,                            //    Usage Effect Block Index
    0x15, 0x01,                            //    Logical Minimum 1
    0x25, MAX_EFFECTS,                     //    Logical Maximum 28h (40d)
    0x35, 0x01,                            //    Physical Minimum 1
    0x45, MAX_EFFECTS,                     //    Physical Maximum 28h (40d)
    0x75, 0x08,                            //    Report Size 8
    0x95, 0x01,                            //    Report Count 1
    0x91, 0x02,                            //    Output (Variable)
    0x09, 0x70,                            //    Usage Magnitude
    0x16, 0x00, 0x80,                      //    Logical Minimum 7FFFh (-32767d)
    0x26, 0xff, 0x7f,                      //    Logical Maximum 7FFFh (32767d)
    0x36, 0x00, 0x80,                     //    Physical Minimum 7FFFh (-32767d)
    0x46, 0xff, 0x7f,                     //    Physical Maximum 7FFFh (32767d)
    0x75, 0x10,                           //    Report Size 10h (16d)
    0x95, 0x01,                           //    Report Count 1
    0x91, 0x02,                           //    Output (Variable)
    0xC0,                                 //    End Collection
    0x09, 0x74,                           //    Usage Set Ramp Force Report
    0xA1, 0x02,                           //    Collection Datalink
    0x85, HID_ID_RAMPREP + FFB_ID_OFFSET, //    Report ID 6
    0x09, 0x22,                           //    Usage Effect Block Index
    0x15, 0x01,                           //    Logical Minimum 1
    0x25, MAX_EFFECTS,                    //    Logical Maximum 28h (40d)
    0x35, 0x01,                           //    Physical Minimum 1
    0x45, MAX_EFFECTS,                    //    Physical Maximum 28h (40d)
    0x75, 0x08,                           //    Report Size 8
    0x95, 0x01,                           //    Report Count 1
    0x91, 0x02,                           //    Output (Variable)
    0x09, 0x75,                           //    Usage Ramp Start
    0x09, 0x76,                           //    Usage Ramp End
    0x16, 0x00, 0x80,                     //    Logical Minimum 7FFFh (-32767d)
    0x26, 0xff, 0x7f,                     //    Logical Maximum 7FFFh (32767d)
    0x36, 0x00, 0x80,                     //    Physical Minimum 7FFFh (-32767d)
    0x46, 0xff, 0x7f,                     //    Physical Maximum 7FFFh (32767d)
    0x75, 0x10,                           //    Report Size 16
    0x95, 0x02,                           //    Report Count 2
    0x91, 0x02,                           //    Output (Variable)
    0xC0,                                 //    End Collection

    //			0x09,0x68,    //    Usage Custom Force Data Report
    //			0xA1,0x02,    //    Collection Datalink
    //			   0x85,HID_ID_CSTMREP+FFB_ID_OFFSET,         //    Report ID 7
    //			   0x09,0x22,         //    Usage Effect Block Index
    //			   0x15,0x01,         //    Logical Minimum 1
    //			   0x25,MAX_EFFECTS,         //    Logical Maximum 28h (40d)
    //			   0x35,0x01,         //    Physical Minimum 1
    //			   0x45,MAX_EFFECTS,         //    Physical Maximum 28h (40d)
    //			   0x75,0x08,         //    Report Size 8
    //			   0x95,0x01,         //    Report Count 1
    //			   0x91,0x02,         //    Output (Variable)
    //			   0x09,0x6C,         //    Usage Custom Force Data Offset
    //			   0x15,0x00,         //    Logical Minimum 0
    //			   0x26,0x10,0x27,    //    Logical Maximum 2710h (10000d)
    //			   0x35,0x00,         //    Physical Minimum 0
    //			   0x46,0x10,0x27,    //    Physical Maximum 2710h (10000d)
    //			   0x75,0x10,         //    Report Size 10h (16d)
    //			   0x95,0x01,         //    Report Count 1
    //			   0x91,0x02,         //    Output (Variable)
    //			   0x09,0x69,         //    Usage Custom Force Data
    //			   0x15,0x81,         //    Logical Minimum 81h (-127d)
    //			   0x25,0x7F,         //    Logical Maximum 7Fh (127d)
    //			   0x35,0x00,         //    Physical Minimum 0
    //			   0x46,0xFF,0x00,    //    Physical Maximum FFh (255d)
    //			   0x75,0x08,         //    Report Size 8
    //			   0x95,0x0C,         //    Report Count Ch (12d)
    //			   0x92,0x02,0x01,    //       Output (Variable, Buffered)
    //			0xC0     ,    //    End Collection
    //			0x09,0x66,    //    Usage Download Force Sample
    //			0xA1,0x02,    //    Collection Datalink
    //			   0x85,HID_ID_SMPLREP+FFB_ID_OFFSET,         //    Report ID 8
    //			   0x05,0x01,         //    Usage Page Generic Desktop
    //			   0x09,0x30,         //    Usage X
    //			   0x09,0x31,         //    Usage Y
    //			   0x15,0x81,         //    Logical Minimum 81h (-127d)
    //			   0x25,0x7F,         //    Logical Maximum 7Fh (127d)
    //			   0x35,0x00,         //    Physical Minimum 0
    //			   0x46,0xFF,0x00,    //    Physical Maximum FFh (255d)
    //			   0x75,0x08,         //    Report Size 8
    //			   0x95,0x02,         //    Report Count 2
    //			   0x91,0x02,         //    Output (Variable)
    //			0xC0     ,   //    End Collection

    0x05, 0x0F,                            //    Usage Page Physical Interface
    0x09, 0x77,                            //    Usage Effect Operation Report
    0xA1, 0x02,                            //    Collection Datalink
    0x85, HID_ID_EFOPREP + FFB_ID_OFFSET,  //    Report ID Ah (10d)
    0x09, 0x22,                            //    Usage Effect Block Index
    0x15, 0x01,                            //    Logical Minimum 1
    0x25, MAX_EFFECTS,                     //    Logical Maximum 28h (40d)
    0x35, 0x01,                            //    Physical Minimum 1
    0x45, MAX_EFFECTS,                     //    Physical Maximum 28h (40d)
    0x75, 0x08,                            //    Report Size 8
    0x95, 0x01,                            //    Report Count 1
    0x91, 0x02,                            //    Output (Variable)
    0x09, 0x78,                            //    Usage Effect Operation
    0xA1, 0x02,                            //    Collection Datalink
    0x09, 0x79,                            //    Usage Op Effect Start
    0x09, 0x7A,                            //    Usage Op Effect Start Solo
    0x09, 0x7B,                            //    Usage Op Effect Stop
    0x15, 0x01,                            //    Logical Minimum 1
    0x25, 0x03,                            //    Logical Maximum 3
    0x75, 0x08,                            //    Report Size 8
    0x95, 0x01,                            //    Report Count 1
    0x91, 0x00,                            //    Output
    0xC0,                                  //    End Collection
    0x09, 0x7C,                            //    Usage Loop Count
    0x15, 0x00,                            //    Logical Minimum 0
    0x26, 0xFF, 0x00,                      //    Logical Maximum FFh (255d)
    0x35, 0x00,                            //    Physical Minimum 0
    0x46, 0xFF, 0x00,                      //    Physical Maximum FFh (255d)
    0x91, 0x02,                            //    Output (Variable)
    0xC0,                                  //    End Collection
    0x09, 0x90,                            //    Usage PID Block Free Report
    0xA1, 0x02,                            //    Collection Datalink
    0x85, HID_ID_BLKFRREP + FFB_ID_OFFSET, //    Report ID Bh (11d)
    0x09, 0x22,                            //    Usage Effect Block Index
    0x15, 0x01,                            //    Logical Minimum 1
    0x25, MAX_EFFECTS,                     //    Logical Maximum 28h (40d)
    0x35, 0x01,                            //    Physical Minimum 1
    0x45, MAX_EFFECTS,                     //    Physical Maximum 28h (40d)
    0x75, 0x08,                            //    Report Size 8
    0x95, 0x01,                            //    Report Count 1
    0x91, 0x02,                            //    Output (Variable)
    0xC0,                                  //    End Collection

    0x09, 0x95,                           //    Usage PID Device Control (0x96?)
    0xA1, 0x02,                           //    Collection Datalink
    0x85, HID_ID_CTRLREP + FFB_ID_OFFSET, //    Report ID Ch (12d)
    0x09, 0x96,                           //    Usage PID Device Control (0x96?)
    0xA1, 0x02,                           //    Collection Datalink

    0x09, 0x97, //    Usage DC Enable Actuators
    0x09, 0x98, //    Usage DC Disable Actuators
    0x09, 0x99, //    Usage DC Stop All Effects
    0x09, 0x9A, //    Usage DC Device Reset
    0x09, 0x9B, //    Usage DC Device Pause
    0x09, 0x9C, //    Usage DC Device Continue

    0x15, 0x01, //    Logical Minimum 1
    0x25, 0x06, //    Logical Maximum 6
    0x75, 0x01, //    Report Size 1
    0x95, 0x08, //    Report Count 8
    0x91, 0x02, //    Output

    0xC0,                                 //    End Collection
    0xC0,                                 //    End Collection
    0x09, 0x7D,                           //    Usage Device Gain Report
    0xA1, 0x02,                           //    Collection Datalink
    0x85, HID_ID_GAINREP + FFB_ID_OFFSET, //    Report ID Dh (13d)
    0x09, 0x7E,                           //    Usage Device Gain
    0x15, 0x00,                           //    Logical Minimum 0
    0x26, 0xFF, 0x00,                     //    Logical Maximum FFh (255d)
    0x35, 0x00,                           //    Physical Minimum 0
    0x46, 0x10, 0x27,                     //    Physical Maximum 2710h (10000d)
    0x75, 0x08,                           //    Report Size 8
    0x95, 0x01,                           //    Report Count 1
    0x91, 0x02,                           //    Output (Variable)
    0xC0,                                 //    End Collection
    //			0x09,0x6B,    //    Usage Set Custom Force Report
    //			0xA1,0x02,    //    Collection Datalink
    //			   0x85,HID_ID_SETCREP+FFB_ID_OFFSET,         //    Report ID Eh
    //(14d) 			   0x09,0x22,         //    Usage Effect Block Index
    // 0x15,0x01, // Logical Minimum 1 			   0x25,MAX_EFFECTS, // Logical
    // Maximum 28h
    //(40d) 			   0x35,0x01,         //    Physical Minimum 1
    // 0x45,MAX_EFFECTS, // Physical Maximum 28h (40d) 			   0x75,0x08, //
    // Report Size 8 			   0x95,0x01,         //    Report Count 1
    // 0x91,0x02, //
    // Output (Variable) 			   0x09,0x6D,         //    Usage Sample
    // Count 0x15,0x00,
    ////    Logical Minimum 0 			   0x26,0xFF,0x00,    //    Logical
    /// Maximum FFh
    //(255d) 			   0x35,0x00,         //    Physical Minimum 0
    // 0x46,0xFF,0x00,    // Physical Maximum FFh (255d)
    // 0x75,0x08,         //    Report Size 8 			   0x95,0x01,         //
    // Report
    // Count 1 			   0x91,0x02,         //    Output (Variable)
    // 0x09,0x51,
    // // Usage Sample Period 			   0x66,0x03,0x10,    //    Unit 1003h
    // (4099d) 			   0x55,0xFD,
    ////    Unit Exponent FDh (253d) 			   0x15,0x00,         // Logical
    /// Minimum 0
    //			   0x26,0xFF,0x7F,    //    Logical Maximum 7FFFh (32767d)
    //			   0x35,0x00,         //    Physical Minimum 0
    //			   0x46,0xFF,0x7F,    //    Physical Maximum 7FFFh (32767d)
    //			   0x75,0x10,         //    Report Size 10h (16d)
    //			   0x95,0x01,         //    Report Count 1
    //			   0x91,0x02,         //    Output (Variable)
    //			   0x55,0x00,         //    Unit Exponent 0
    //			   0x66,0x00,0x00,    //    Unit 0
    //			0xC0     ,    //    End Collection
    0x09, 0xAB,                            //    Usage Create New Effect Report
    0xA1, 0x02,                            //    Collection Datalink
    0x85, HID_ID_NEWEFREP + FFB_ID_OFFSET, //    Report ID 1
    0x09, 0x25,                            //    Usage Effect Type
    0xA1, 0x02,                            //    Collection Datalink
    0x09, HID_USAGE_CONST,                 //    Usage ET Constant Force
    0x09, HID_USAGE_RAMP,                  //    Usage ET Ramp
    0x09, HID_USAGE_SQUR,                  //    Usage ET Square
    0x09, HID_USAGE_SINE,                  //    Usage ET Sine
    0x09, HID_USAGE_TRNG,                  //    Usage ET Triangle
    0x09, HID_USAGE_STUP,                  //    Usage ET Sawtooth Up
    0x09, HID_USAGE_STDN,                  //    Usage ET Sawtooth Down
    0x09, HID_USAGE_SPRNG,                 //    Usage ET Spring
    0x09, HID_USAGE_DMPR,                  //    Usage ET Damper
    0x09, HID_USAGE_INRT,                  //    Usage ET Inertia
    0x09, HID_USAGE_FRIC,                  //    Usage ET Friction
    //			 0x09, 0x28,    //    Usage ET Custom Force Data
    0x25, 0x0B,                            //    Logical Maximum Ch (11d)
    0x15, 0x01,                            //    Logical Minimum 1
    0x35, 0x01,                            //    Physical Minimum 1
    0x45, 0x0B,                            //    Physical Maximum Ch (11d)
    0x75, 0x08,                            //    Report Size 8
    0x95, 0x01,                            //    Report Count 1
    0xB1, 0x00,                            //    Feature
    0xC0,                                  // End Collection
    0x05, 0x01,                            //    Usage Page Generic Desktop
    0x09, 0x3B,                            //    Usage Reserved (Byte count)
    0x15, 0x00,                            //    Logical Minimum 0
    0x26, 0xFF, 0x01,                      //    Logical Maximum 1FFh (511d)
    0x35, 0x00,                            //    Physical Minimum 0
    0x46, 0xFF, 0x01,                      //    Physical Maximum 1FFh (511d)
    0x75, 0x0A,                            //    Report Size Ah (10d)
    0x95, 0x01,                            //    Report Count 1
    0xB1, 0x02,                            //    Feature (Variable)
    0x75, 0x06,                            //    Report Size 6
    0xB1, 0x01,                            //    Feature (Constant)
    0xC0,                                  //    End Collection
    0x05, 0x0F,                            //    Usage Page Physical Interface
    0x09, 0x89,                            //    Usage Block Load Report
    0xA1, 0x02,                            //    Collection Datalink
    0x85, HID_ID_BLKLDREP + FFB_ID_OFFSET, //    Report ID 0x12
    0x09, 0x22,                            //    Usage Effect Block Index
    0x25, MAX_EFFECTS,                     //    Logical Maximum 28h (40d)
    0x15, 0x01,                            //    Logical Minimum 1
    0x35, 0x01,                            //    Physical Minimum 1
    0x45, MAX_EFFECTS,                     //    Physical Maximum 28h (40d)
    0x75, 0x08,                            //    Report Size 8
    0x95, 0x01,                            //    Report Count 1
    0xB1, 0x02,                            //    Feature (Variable)
    0x09, 0x8B,                            //    Usage Block Load Status
    0xA1, 0x02,                            //    Collection Datalink
    0x09, 0x8C,                            //    Usage Block Load Success
    0x09, 0x8D,                            //    Usage Block Load Full
    0x09, 0x8E,                            //    Usage Block Load Error
    0x15, 0x01,                            //    Logical Minimum 1
    0x25, 0x03,                            //    Logical Maximum 3
    0x35, 0x01,                            //    Physical Minimum 1
    0x45, 0x03,                            //    Physical Maximum 3
    0x75, 0x08,                            //    Report Size 8
    0x95, 0x01,                            //    Report Count 1
    0xB1, 0x00,                            //    Feature
    0xC0,                                  // End Collection
    0x09, 0xAC,                            //    Usage Pool available
    0x15, 0x00,                            //    Logical Minimum 0
    0x27, 0xFF, 0xFF, 0x00, 0x00,          //    Logical Maximum FFFFh (65535d)
    0x35, 0x00,                            //    Physical Minimum 0
    0x47, 0xFF, 0xFF, 0x00, 0x00,          //    Physical Maximum FFFFh (65535d)
    0x75, 0x10,                            //    Report Size 10h (16d)
    0x95, 0x01,                            //    Report Count 1
    0xB1, 0x00,                            //    Feature
    0xC0,                                  //    End Collection

    0x09, 0x7F,                           //    Usage PID Pool Report
    0xA1, 0x02,                           //    Collection Datalink
    0x85, HID_ID_POOLREP + FFB_ID_OFFSET, //    Report ID 0x13
    0x09, 0x80,                           //    Usage RAM Pool size
    0x75, 0x10,                           //    Report Size 10h (16d)
    0x95, 0x01,                           //    Report Count 1
    0x15, 0x00,                           //    Logical Minimum 0
    0x35, 0x00,                           //    Physical Minimum 0
    0x27, 0xFF, 0xFF, 0x00, 0x00,         //    Logical Maximum FFFFh (65535d)
    0x47, 0xFF, 0xFF, 0x00, 0x00,         //    Physical Maximum FFFFh (65535d)
    0xB1, 0x02,                           //    Feature (Variable)
    0x09, 0x83,                           //    Usage Simultaneous Effects Max
    0x26, 0xFF, 0x00,                     //    Logical Maximum FFh (255d)
    0x46, 0xFF, 0x00,                     //    Physical Maximum FFh (255d)
    0x75, 0x08,                           //    Report Size 8
    0x95, 0x01,                           //    Report Count 1
    0xB1, 0x02,                           //    Feature (Variable)
    0x09, 0xA9,                           //    Usage Device Managed Pool
    0x09, 0xAA,                           //    Usage Shared Parameter Blocks
    0x75, 0x01,                           //    Report Size 1
    0x95, 0x02,                           //    Report Count 2
    0x15, 0x00,                           //    Logical Minimum 0
    0x25, 0x01,                           //    Logical Maximum 1
    0x35, 0x00,                           //    Physical Minimum 0
    0x45, 0x01,                           //    Physical Maximum 1
    0xB1, 0x02,                           //    Feature (Variable)
    0x75, 0x06,                           //    Report Size 6
    0x95, 0x01,                           //    Report Count 1
    0xB1, 0x03,                           //    Feature (Constant, Variable)
    0xC0,                                 //    End Collection

    0xC0 /*     END_COLLECTION	             */

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
        (const char[]){0x09, 0x04}, // supported language is English (0x0409)
    [STRID_MANUFACTURER] = "wuyoutech Limited",         // Manufacturer
    [STRID_PRODUCT] = "OneFFB Device",                  // Product
    [STRID_SERIAL] = "123456",                          // TODO: Generate Serial
    [STRID_INTERFACE_NET] = "OneFFB Network Interface", // Interface Description
    [STRID_INTERFACE_HID] = "OneFFB Force Feedback Wheel"
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
