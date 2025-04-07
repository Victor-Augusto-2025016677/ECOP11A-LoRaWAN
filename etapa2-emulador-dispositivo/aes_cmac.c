#include "aes_cmac.h"
#include <mbedtls/cmac.h>
#include <string.h>

int aes128_cmac(
    const uint8_t key[16],
    const uint8_t *input,
    uint16_t len,
    uint8_t mac[16]
) {
    int ret = mbedtls_cipher_cmac(
        mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_ECB),
        key,
        128,
        input,
        len,
        mac
    );
    return ret;
}

// Função específica LoRaWAN para calcular e adicionar o MIC ao final do pacote
int lorawan_append_mic(
    uint8_t *packet,
    int len,
    uint32_t devAddr,
    uint16_t fcnt,
    const uint8_t nwkSKey[16]
) {
    uint8_t b0[16] = {0};

    b0[0] = 0x49; // bloco de prefixo MIC
    b0[5] = 0x00; // direção uplink = 0
    b0[6] = (uint8_t)(devAddr & 0xFF);
    b0[7] = (uint8_t)((devAddr >> 8) & 0xFF);
    b0[8] = (uint8_t)((devAddr >> 16) & 0xFF);
    b0[9] = (uint8_t)((devAddr >> 24) & 0xFF);
    b0[10] = (uint8_t)(fcnt & 0xFF);
    b0[11] = (uint8_t)((fcnt >> 8) & 0xFF);
    b0[15] = (uint8_t)len;

    // Concatenar b0 || packet
    uint8_t buffer[256];
    memcpy(buffer, b0, 16);
    memcpy(buffer + 16, packet, len);

    // Calcular CMAC
    uint8_t full_mac[16];
    if (aes128_cmac(nwkSKey, buffer, len + 16, full_mac) != 0) {
        return -1;
    }

    // MIC são os primeiros 4 bytes do CMAC
    memcpy(packet + len, full_mac, 4);
    return 4; // bytes de MIC adicionados
}
