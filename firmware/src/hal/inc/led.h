#ifndef __LED_H_
#define __LED_H_

#include <stdbool.h>

void led_init(void);
void led_sys_write(bool new_state);

#endif
