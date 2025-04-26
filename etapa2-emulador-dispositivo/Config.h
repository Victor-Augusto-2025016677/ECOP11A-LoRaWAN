#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define MAX_PAYLOAD_SIZE 64

typedef struct {
    char deveui[17];          // DeveUI como string (16 caracteres + '\0')
    uint32_t devaddr;         // DevAddr como uint32_t
    uint8_t nwkskey[16];      // NwkSKey como array de 16 bytes
    uint8_t appskey[16];      // AppSKey como array de 16 bytes
    uint16_t fcnt;            // Frame counter
    uint8_t fport;            // Frame port
    uint8_t payload[MAX_PAYLOAD_SIZE]; // Payload
    uint8_t payload_len;      // Tamanho do payload
} Config;

int load_config(const char *filename, Config *config);
int save_config(const char *filename, const Config *config);

#endif // CONFIG_H
