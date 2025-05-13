#include "lorawan.h"
#include <string.h>
#include "aes_cmac.h"
#include <mbedtls/aes.h>

int lorawan_build_uplink(
    uint8_t *out_buffer,
    uint32_t devAddr,
    const uint8_t nwkSKey[16],
    const uint8_t appSKey[16],
    uint16_t fcnt,
    uint8_t fport,
    const uint8_t *payload,
    uint8_t payload_len
) {
    int index = 0;

    out_buffer[index++] = 0x40;

    out_buffer[index++] = (uint8_t)((devAddr >> 24) & 0xFF);
    out_buffer[index++] = (uint8_t)((devAddr >> 16) & 0xFF);
    out_buffer[index++] = (uint8_t)((devAddr >> 8) & 0xFF);
    out_buffer[index++] = (uint8_t)(devAddr & 0xFF);

    out_buffer[index++] = 0x00;
    out_buffer[index++] = (uint8_t)(fcnt & 0xFF);
    out_buffer[index++] = (uint8_t)((fcnt >> 8) & 0xFF);
    out_buffer[index++] = fport;

    uint8_t *encrypted = &out_buffer[index];
    uint8_t block_a[16], s_block[16];
    mbedtls_aes_context aes;

    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, appSKey, 128);

    for (int i = 0; i < payload_len; i += 16) {
        memset(block_a, 0, 16);
        block_a[0] = 0x01;
        block_a[5] = 0x00;

        block_a[6] = (uint8_t)((devAddr >> 0) & 0xFF);
        block_a[7] = (uint8_t)((devAddr >> 8) & 0xFF);
        block_a[8] = (uint8_t)((devAddr >> 16) & 0xFF);
        block_a[9] = (uint8_t)((devAddr >> 24) & 0xFF);

        block_a[10] = (uint8_t)(fcnt & 0xFF);
        block_a[11] = (uint8_t)((fcnt >> 8) & 0xFF);
        block_a[15] = (uint8_t)((i / 16) + 1);

        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, block_a, s_block);

        for (int j = 0; j < 16 && (i + j) < payload_len; j++) {
            encrypted[i + j] = payload[i + j] ^ s_block[j];
        }
    }

    mbedtls_aes_free(&aes);
    index += payload_len;

    uint8_t mic[4];
    uint8_t b0[16];
    memset(b0, 0, 16);

    b0[0] = 0x49;
    b0[5] = 0x00;

    b0[6] = (uint8_t)((devAddr >> 0) & 0xFF);
    b0[7] = (uint8_t)((devAddr >> 8) & 0xFF);
    b0[8] = (uint8_t)((devAddr >> 16) & 0xFF);
    b0[9] = (uint8_t)((devAddr >> 24) & 0xFF);

    b0[10] = (uint8_t)(fcnt & 0xFF);
    b0[11] = (uint8_t)((fcnt >> 8) & 0xFF);
    b0[15] = (uint8_t)(index);

    uint8_t mic_input[256];
    memcpy(mic_input, b0, 16);
    memcpy(mic_input + 16, out_buffer, index);

    aes128_cmac(nwkSKey, mic_input, index + 16, mic);

    out_buffer[index++] = mic[0];
    out_buffer[index++] = mic[1];
    out_buffer[index++] = mic[2];
    out_buffer[index++] = mic[3];

    return index;
}
