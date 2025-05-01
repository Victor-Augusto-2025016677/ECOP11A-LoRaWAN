#ifndef LORAWAN_H
#define LORAWAN_H

#include <stdint.h>

// Monta um pacote LoRaWAN (sem MIC ainda)
int lorawan_build_uplink(
    uint8_t *out_buffer,
    uint32_t devAddr,
    const uint8_t nwkSKey[16],
    const uint8_t appSKey[16],
    uint16_t fcnt,
    uint8_t fport,
    const uint8_t *payload,
    uint8_t payload_len
);

// Calcula e adiciona o MIC ao final do pacote
int lorawan_append_mic(
    uint8_t *buffer,
    int len,
    uint32_t devAddr,
    uint16_t fcnt,
    const uint8_t nwkSKey[16]
);

#endif // LORAWAN_H
