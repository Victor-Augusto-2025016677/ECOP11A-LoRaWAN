#include "aes_cmac.h"
#include <mbedtls/cmac.h>
#include <string.h>
#include <stdio.h>

int aes128_cmac(const uint8_t *key, const uint8_t *input, size_t length, uint8_t *output) {
    mbedtls_cipher_context_t ctx;
    const mbedtls_cipher_info_t *cipher_info;
    int ret;

    mbedtls_cipher_init(&ctx);

    cipher_info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_ECB);
    if (cipher_info == NULL) {
        printf("Erro: Informações do cipher não encontradas\n");
        mbedtls_cipher_free(&ctx);
        return -1;
    }

    ret = mbedtls_cipher_setup(&ctx, cipher_info);
    if (ret != 0) {
        printf("Erro ao configurar o contexto do cipher: %d\n", ret);
        mbedtls_cipher_free(&ctx);
        return -1;
    }

    ret = mbedtls_cipher_cmac_starts(&ctx, key, 128);
    if (ret != 0) {
        printf("Erro ao iniciar o CMAC: %d\n", ret);
        mbedtls_cipher_free(&ctx);
        return -1;
    }

    ret = mbedtls_cipher_cmac_update(&ctx, input, length);
    if (ret != 0) {
        printf("Erro ao atualizar o CMAC: %d\n", ret);
        mbedtls_cipher_free(&ctx);
        return -1;
    }

    ret = mbedtls_cipher_cmac_finish(&ctx, output);
    if (ret != 0) {
        printf("Erro ao finalizar o CMAC: %d\n", ret);
        mbedtls_cipher_free(&ctx);
        return -1;
    }

    mbedtls_cipher_free(&ctx);

    return 0;
}

int lorawan_append_mic(
    uint8_t *packet,
    int len,
    uint32_t devAddr,
    uint16_t fcnt,
    const uint8_t nwkSKey[16]
) {
    uint8_t b0[16] = {0};
    uint8_t mic[16] = {0};
    int ret;

    b0[0] = 0x49;
    b0[5] = 0x00;
    b0[6] = (uint8_t)(devAddr & 0xFF);
    b0[7] = (uint8_t)((devAddr >> 8) & 0xFF);
    b0[8] = (uint8_t)((devAddr >> 16) & 0xFF);
    b0[9] = (uint8_t)((devAddr >> 24) & 0xFF);
    b0[10] = (uint8_t)(fcnt & 0xFF);
    b0[11] = (uint8_t)((fcnt >> 8) & 0xFF);
    b0[15] = (uint8_t)len;

    uint8_t buffer[256];
    if ((size_t)(len + 16) > sizeof(buffer)) { // Correção aqui
        printf("Erro: Tamanho do pacote excede o limite do buffer\n");
        return -1;
    }
    memcpy(buffer, b0, 16);
    memcpy(buffer + 16, packet, len);

    // Calcular o MIC usando a função genérica AES-128 CMAC
    ret = aes128_cmac(nwkSKey, buffer, len + 16, mic);
    if (ret != 0) {
        printf("Erro ao calcular o MIC\n");
        return -1;
    }

    memcpy(packet + len, mic, 4);

    return 4;
}
