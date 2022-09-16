#ifndef __SYSTICK_H_
#define __SYSTICK_H_

#include <stdint.h>
void systick_init(void);
uint32_t board_millis(void);

#endif
