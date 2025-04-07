#ifndef AES_CMAC_H
#define AES_CMAC_H

#include <stdint.h>

// Função genérica: AES-128 CMAC (usada internamente)
int aes128_cmac(
    const uint8_t key[16],
    const uint8_t *input,
    uint16_t len,
    uint8_t mac[16]
);

// Função específica para LoRaWAN: calcula e adiciona o MIC no final do pacote
int lorawan_append_mic(
    uint8_t *packet,
    int len,
    uint32_t devAddr,
    uint16_t fcnt,
    const uint8_t nwkSKey[16]
);

#endif // AES_CMAC_H
