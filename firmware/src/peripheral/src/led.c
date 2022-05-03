#include <stdbool.h>
#include <stdint.h>

#include <led.h>
#include <stm32f4xx.h>

void led_initialize(void) {
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void led_sys_set(bool new_state) {
    GPIO_WriteBit(GPIOA, GPIO_Pin_0, new_state ? Bit_RESET : Bit_SET);
}

void led_clip_set(bool new_state) {
    GPIO_WriteBit(GPIOA, GPIO_Pin_1, new_state ? Bit_RESET : Bit_SET);
}

void led_err_set(bool new_state) {
    GPIO_WriteBit(GPIOA, GPIO_Pin_2, new_state ? Bit_RESET : Bit_SET);
}

void fetal_halt(void) {
    while (1) {
        led_sys_set(true);
        led_clip_set(true);
        led_err_set(true);
        for (uint32_t i = 0; i < 0xfffff; i++)
            ;
        led_sys_set(false);
        led_err_set(false);
        led_clip_set(false);
        for (uint32_t i = 0; i < 0xfffff; i++)
            ;
    }
}