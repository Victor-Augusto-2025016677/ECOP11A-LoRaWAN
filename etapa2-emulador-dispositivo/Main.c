#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Headers dos nossos módulos
#include "lorawan.h"
#include "aes_cmac.h"

int main() {
    printf("[LoRaWAN] Simulador de dispositivo iniciado\n");

    // Simular payload com 2 variáveis (ex: temperatura e umidade)
    uint8_t payload[2] = { 24, 55 };

    // Parâmetros LoRaWAN (exemplo, substituiremos depois por leitura JSON)
    uint32_t devAddr = 0x26011BDA;
    uint8_t nwkSKey[16] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    };
    uint8_t appSKey[16] = {
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
        0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99
    };
    uint16_t fcnt = 1;
    uint8_t fport = 1;

    // Buffer para montar o pacote
    uint8_t packet[64];
    int packet_len = lorawan_build_uplink(
        packet, devAddr, nwkSKey, appSKey,
        fcnt, fport, payload, sizeof(payload)
    );

    if (packet_len <= 0) {
        printf("Erro ao montar pacote LoRaWAN\n");
        return 1;
    }

    // Adicionar MIC no final do pacote (opcional, se não foi incluso na função acima)
    int mic_len = lorawan_append_mic(packet, packet_len, devAddr, fcnt, nwkSKey);
    if (mic_len != 4) {
        printf("Erro ao calcular MIC\n");
        return 1;
    }
    packet_len += mic_len;

    // Exibir pacote final em hexadecimal
    printf("Pacote LoRaWAN com MIC (hex):\n");
    for (int i = 0; i < packet_len; i++) {
        printf("%02X ", packet[i]);
    }
    printf("\n");

    return 0;
}
