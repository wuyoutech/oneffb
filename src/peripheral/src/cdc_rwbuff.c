#include <stdbool.h>
#include <stdint.h>

#include <cdc_rwbuff.h>

// TODO: All fucntions not test yet
bool is_full(cdc_rwbuff *buff) {
    if (buff->p_in + 1 == buff->p_out)
        return false;
    else
        return true;
}

bool is_empty(cdc_rwbuff *buff) {
    if (buff->p_in == buff->p_out)
        return true;
    else
        return false;
}

bool enqueue(cdc_rwbuff *buff, uint8_t data) {
    if (is_full(buff))
        return false;
    buff->data[buff->p_in] = data;
    buff->p_in = (buff->p_in + 1) % CDC_BUFF_MAXLEN;
    return true;
}

bool dequeue(cdc_rwbuff *buff, uint8_t *data) {
    if (is_empty(buff))
        return false;
    *data = buff->data[buff->p_out];
    buff->p_out = buff->p_out + 1 % CDC_BUFF_MAXLEN;
    return true;
}
