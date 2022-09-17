#include <stdbool.h>
#include <stdint.h>

#include <stm32f4xx.h>

#include <led.h>
#include <systick.h>
#include <usb.h>

void cdc_task(void);
void board_init(void);

int main() {
    board_init();
    while (1) {
        led_blinking_task();
        usb_task();
    }
}
void board_init(void) {
    systick_init();
    led_init();
    usb_init();
}

void HardFault_Handler(void) { asm("bkpt"); }
