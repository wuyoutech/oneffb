#include <stdint.h>

#include <stm32f4xx.h>

/* get unique deivce id */
uint8_t *get_unique_device_id() { return (uint32_t *)(0x1FFF7A10); }
/* generate mac address */
uint8_t tud_network_mac_address[6] = {0x02, 0x02, 0x84, 0x6A, 0x96, 0x00};

void network_mac_address_generate(uint8_t *mac_address) {
    mac_address[0] = 0x02;
    uint8_t *unique_device_id = get_unique_device_id();
    for (uint8_t i = 0; i < 5; i++) {
        mac_address[i + 1] = unique_device_id[i];
    }
}

void