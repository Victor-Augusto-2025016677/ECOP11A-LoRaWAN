#ifndef AES_CMAC_H
#define AES_CMAC_H

#include <stdint.h>
#include <stddef.h> // Para o tipo size_t

// Função genérica: AES-128 CMAC (usada internamente)
int aes128_cmac(
    const uint8_t *key,
    const uint8_t *input,
    size_t length,
    uint8_t *output
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
