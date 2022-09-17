#pragma once

#include <stdbool.h>

void led_init(void);
void led_blinking_task(void);
void led_blinking_timeset(uint32_t time);

enum { blink_not_mounted = 250, blink_mounted = 1000, blink_suspended = 2500 };
