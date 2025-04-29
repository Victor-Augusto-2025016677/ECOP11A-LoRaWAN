#include "aes_cmac.h"
#include <mbedtls/cmac.h>
#include <string.h>
#include <stdio.h>

int aes128_cmac(const uint8_t *key, const uint8_t *input, size_t length, uint8_t *output) {
    mbedtls_cipher_context_t ctx;
    const mbedtls_cipher_info_t *cipher_info;
    int ret;

    // Inicializar o contexto do CMAC
    mbedtls_cipher_init(&ctx);

    // Obter informações do cipher AES-128
    cipher_info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_ECB);
    if (cipher_info == NULL) {
        printf("Erro: Informações do cipher não encontradas\n");
        mbedtls_cipher_free(&ctx);
        return -1;
    }

    // Configurar o contexto do cipher
    ret = mbedtls_cipher_setup(&ctx, cipher_info);
    if (ret != 0) {
        printf("Erro ao configurar o contexto do cipher: %d\n", ret);
        mbedtls_cipher_free(&ctx);
        return -1;
    }

    // Iniciar o CMAC com a chave fornecida
    ret = mbedtls_cipher_cmac_starts(&ctx, key, 128);
    if (ret != 0) {
        printf("Erro ao iniciar o CMAC: %d\n", ret);
        mbedtls_cipher_free(&ctx);
        return -1;
    }

    // Atualizar o CMAC com os dados de entrada
    ret = mbedtls_cipher_cmac_update(&ctx, input, length);
    if (ret != 0) {
        printf("Erro ao atualizar o CMAC: %d\n", ret);
        mbedtls_cipher_free(&ctx);
        return -1;
    }

    // Finalizar o CMAC e obter o resultado
    ret = mbedtls_cipher_cmac_finish(&ctx, output);
    if (ret != 0) {
        printf("Erro ao finalizar o CMAC: %d\n", ret);
        mbedtls_cipher_free(&ctx);
        return -1;
    }

    // Liberar o contexto do CMAC
    mbedtls_cipher_free(&ctx);

    return 0; // Sucesso
}

int lorawan_append_mic(
    uint8_t *packet,
    int len,
    uint32_t devAddr,
    uint16_t fcnt,
    const uint8_t nwkSKey[16]
) {
    uint8_t b0[16] = {0}; // Bloco B0 para o cálculo do MIC
    uint8_t mic[16] = {0}; // Buffer para armazenar o MIC
    int ret;

    // Preencher o bloco B0 conforme o padrão LoRaWAN
    b0[0] = 0x49; // Prefixo para o MIC
    b0[5] = 0x00; // Direção (0 para uplink)
    b0[6] = (uint8_t)(devAddr & 0xFF);
    b0[7] = (uint8_t)((devAddr >> 8) & 0xFF);
    b0[8] = (uint8_t)((devAddr >> 16) & 0xFF);
    b0[9] = (uint8_t)((devAddr >> 24) & 0xFF);
    b0[10] = (uint8_t)(fcnt & 0xFF);
    b0[11] = (uint8_t)((fcnt >> 8) & 0xFF);
    b0[15] = (uint8_t)len; // Comprimento do pacote

    // Concatenar B0 e o pacote em um buffer temporário
    uint8_t buffer[256];
    if (len + 16 > sizeof(buffer)) {
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

    // Adicionar os 4 primeiros bytes do MIC ao final do pacote
    memcpy(packet + len, mic, 4);

    return 4; // Retorna o comprimento do MIC
}
