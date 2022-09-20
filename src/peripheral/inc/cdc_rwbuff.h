#pragma once

#include <stdint.h>

#define CDC_BUFF_MAXLEN (64)

typedef struct cdc_rwbuff {
    uint32_t p_in;
    uint32_t p_out;
    uint8_t data[CDC_BUFF_MAXLEN];
} cdc_rwbuff;

bool enqueue(cdc_rwbuff *buff, uint8_t data);
bool dequeue(cdc_rwbuff *buff, uint8_t *data);
