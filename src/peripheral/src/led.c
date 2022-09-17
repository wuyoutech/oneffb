#include <stdbool.h>
#include <stdint.h>

#include <led.h>
#include <stm32f4xx.h>
#include <systick.h>

void led_init(void) {
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
}

void led_set(bool state) {
    GPIO_WriteBit(GPIOC, GPIO_Pin_13, state ? Bit_SET : Bit_RESET);
}

static uint32_t blink_interval_ms = blink_not_mounted;
void led_blinking_task(void) {
    static uint32_t start_ms = 0;
    static bool led_state = false;
    if (board_millis() - start_ms < blink_interval_ms)
        return;

    led_set(led_state);
    led_state = led_state ? false : true;
}

void led_blinking_timeset(uint32_t time) { blink_interval_ms = time; }
