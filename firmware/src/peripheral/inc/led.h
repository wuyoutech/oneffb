#pragma once

#include <stdbool.h>

void led_initialize(void);
void led_sys_set(bool new_state);
void led_clip_set(bool new_state);
void led_err_set(bool new_state);

void fetal_halt(void);