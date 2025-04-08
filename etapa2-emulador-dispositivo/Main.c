#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

// Headers dos nossos módulos
#include "lorawan.h"
#include "aes_cmac.h"
#include "config.h"

int main() {
    printf("[LoRaWAN] Simulador de dispositivo iniciado\n");

    Config cfg;
    if (!load_config("Config.json", &cfg)) {
        printf("Erro ao carregar o arquivo Config.json\n");
        return 1;
    }

    // Buffer para montar o pacote
    uint8_t packet[64];
    int packet_len = lorawan_build_uplink(
        packet, cfg.devaddr, cfg.nwkskey, cfg.appskey,
        cfg.fcnt, cfg.fport, cfg.payload, cfg.payload_len
    );

    if (packet_len <= 0) {
        printf("Erro ao montar pacote LoRaWAN\n");
        return 1;
    }

    // Adicionar MIC no final do pacote (opcional, se não foi incluso na função acima)
    int mic_len = lorawan_append_mic(packet, packet_len, cfg.devaddr, cfg.fcnt, cfg.nwkskey);
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
