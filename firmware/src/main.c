#include <stdint.h>
#include <stdio.h>

#include <stm32f4xx.h>

#include <led.h>

uint32_t get_device_id(void) {
    return ((DBGMCU->IDCODE) & DBGMCU_IDCODE_DEV_ID);
}

const uint32_t device_id = 0x431;

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
    if (get_device_id != device_id) {
        while(1){
            fetal_halt();
        }
    }




    while (1)
        ;
}

void reboot_to_dfu(void) {
    *((unsigned long *)0x2001fff0) = 0xdeadbeef;
    NVIC_SystemReset();
}