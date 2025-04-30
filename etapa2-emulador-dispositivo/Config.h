#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define MBEDTLS_CIPHER_MODE_CMAC 1

#define MAX_PAYLOAD_SIZE 64

typedef struct {
    char deveui[17];
    uint32_t devaddr;
    uint8_t nwkskey[16];
    uint8_t appskey[16];
    uint16_t fcnt;
    uint8_t fport;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint8_t payload_len;
} Config;

int load_config(const char *filename, Config *config);
int save_config(const char *filename, const Config *config);

#endif // CONFIG_H
