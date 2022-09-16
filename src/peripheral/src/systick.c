#include <stm32f4xx.h>
#include <stdint.h>
#include <stdbool.h>

volatile uint32_t system_ticks = 0;
void SysTick_Handler(void) { system_ticks++; }
uint32_t board_millis(void) { return system_ticks; }
void systick_init(void)
{
    SysTick_Config(SystemCoreClock / 1000);
}
