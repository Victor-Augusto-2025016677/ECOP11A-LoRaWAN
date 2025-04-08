#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define MAX_PAYLOAD_SIZE 64

typedef struct {
    uint32_t devaddr;
    uint8_t nwkskey[16];
    uint8_t appskey[16];
    uint16_t fcnt;
    uint8_t fport;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint8_t payload_len;
} Config;

int load_config(const char *filename, Config *config);

#endif // CONFIG_H
