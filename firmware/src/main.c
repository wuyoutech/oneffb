#include <stdint.h>
#include <stdio.h>

#include <stm32f4xx.h>

#include <led.h>

#include <usb_conf.h>
#include <usb_core.h>
#include <usbd_core.h>
#include <usbd_desc.h>
#include <usbd_cdc_vcp.h>
#include <usbd_usr.h>

uint32_t get_device_id(void) {
    return ((DBGMCU->IDCODE) & DBGMCU_IDCODE_DEV_ID);
}

const uint32_t device_id = 0x431;

USB_OTG_CORE_HANDLE usb_otg_device;

int main(void) {
    // change nvic priority grouping
    NVIC_SetPriorityGrouping(NVIC_PriorityGroup_4);
    // init system leds
    led_initialize();
    led_sys_set(true);
    led_clip_set(true);
    led_err_set(true);
    // check chip id
    // this firmware only support stm32f411
    if (get_device_id() != device_id) {
        while (1) {
            fetal_halt();
        }
    }

    USBD_Init(&usb_otg_device, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb,
              &USR_cb);

    while (1)
        ;
}

void reboot_to_dfu(void) {
    *((unsigned long *)0x2001fff0) = 0xdeadbeef;
    NVIC_SystemReset();
}
