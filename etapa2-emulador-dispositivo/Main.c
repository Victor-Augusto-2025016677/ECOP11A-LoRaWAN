#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

// Headers dos nossos módulos
#include "lorawan.h"
#include "aes_cmac.h"
#include "config.h"

// Declaração da função send_to_ttn
int send_to_ttn(const uint8_t *packet, int packet_len);


int save_config(const char *filename, const Config *cfg) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Erro ao abrir o arquivo para escrita");
        return 0;
    }

    cJSON *json = cJSON_CreateObject();

    cJSON_AddStringToObject(json, "deveui", cfg->deveui);

    char devaddr_str[9]; 
    snprintf(devaddr_str, sizeof(devaddr_str), "%08X", cfg->devaddr);
    cJSON_AddStringToObject(json, "devaddr", devaddr_str);

    char nwkskey_str[33], appskey_str[33];
    for (int i = 0; i < 16; i++) {
        snprintf(&nwkskey_str[i * 2], 3, "%02X", cfg->nwkskey[i]);
        snprintf(&appskey_str[i * 2], 3, "%02X", cfg->appskey[i]);
    }
    cJSON_AddStringToObject(json, "nwkskey", nwkskey_str);
    cJSON_AddStringToObject(json, "appskey", appskey_str);

    cJSON_AddNumberToObject(json, "fcnt", cfg->fcnt);
    cJSON_AddNumberToObject(json, "fport", cfg->fport);

    int payload_int[64];
    for (int i = 0; i < cfg->payload_len; i++) {
        payload_int[i] = (int)cfg->payload[i];
    }
    cJSON *payload = cJSON_CreateIntArray(payload_int, cfg->payload_len);
    if (payload == NULL) {
        perror("Erro ao criar array de payload");
        cJSON_Delete(json);
        fclose(file);
        return 0;
    }
    cJSON_AddItemToObject(json, "payload", payload);

    char *json_string = cJSON_Print(json);
    if (json_string == NULL) {
        perror("Erro ao converter JSON para string");
        cJSON_Delete(json);
        fclose(file);
        return 0;
    }
    fprintf(file, "%s", json_string);

    fclose(file);
    cJSON_Delete(json);
    free(json_string);

    return 1;
}

int send_to_ttn(const uint8_t *packet, int packet_len) {
    int sockfd;
    struct sockaddr_in server_addr;
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP

    printf("Resolvendo o hostname do servidor TTN...\n");
    if (getaddrinfo("nam1.cloud.thethings.network", "1700", &hints, &res) != 0) {
        perror("Erro ao resolver o hostname do servidor TTN");
        return -1;
    }
    printf("Hostname resolvido com sucesso.\n");

    printf("Criando socket UDP...\n");
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        freeaddrinfo(res);
        return -1;
    }
    printf("Socket criado com sucesso.\n");

    printf("Enviando pacote para o TTN...\n");
    if (sendto(sockfd, packet, packet_len, 0, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Erro ao enviar pacote");
        close(sockfd);
        freeaddrinfo(res);
        return -1;
    }
    printf("Pacote enviado para o TTN com sucesso!\n");

    close(sockfd);
    freeaddrinfo(res);
    return 0;
}

int main() {
    printf("[LoRaWAN] Simulador de dispositivo iniciado\n");

    Config cfg;
    printf("Carregando configurações do arquivo Config.json...\n");
    if (!load_config("Config.json", &cfg)) {
        printf("Erro ao carregar o arquivo Config.json\n");
        return 1;
    }
    printf("Configurações carregadas com sucesso.\n");

    uint8_t packet[64];
    printf("Montando o pacote LoRaWAN...\n");
    int packet_len = lorawan_build_uplink(
        packet, cfg.devaddr, cfg.nwkskey, cfg.appskey,
        cfg.fcnt, cfg.fport, cfg.payload, cfg.payload_len
    );

    if (packet_len <= 0) {
        printf("Erro ao montar pacote LoRaWAN\n");
        return 1;
    }
    printf("Pacote montado com sucesso.\n");

    printf("Calculando o MIC...\n");
    int mic_len = lorawan_append_mic(packet, packet_len, cfg.devaddr, cfg.fcnt, cfg.nwkskey);
    if (mic_len != 4) {
        printf("Erro ao calcular MIC\n");
        return 1;
    }
    packet_len += mic_len;
    printf("MIC calculado com sucesso.\n");

    printf("Pacote LoRaWAN com MIC (hex):\n");
    for (int i = 0; i < packet_len; i++) {
        printf("%02X ", packet[i]);
    }
    printf("\n");


    if (send_to_ttn(packet, packet_len) != 0) {
        printf("Erro ao enviar pacote para o TTN\n");
        return 1;
    }

    printf("Incrementando o contador de frames (fcnt)...\n");
    cfg.fcnt += 1;

    printf("Salvando o novo valor de fcnt no Config.json...\n");
    if (!save_config("Config.json", &cfg)) {
        printf("Erro ao salvar o arquivo Config.json\n");
        return 1;
    }
    printf("Config.json atualizado com sucesso.\n");

    return 0;
}
