#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// Headers dos nossos módulos
#include "lorawan.h"
#include "aes_cmac.h"
#include "Config.h"

// Declaração da função send_to_ttn
int send_to_ttn(const uint8_t *packet, int packet_len);


// Função para salvar o Config.json atualizado
int save_config(const char *filename, const Config *cfg) {
    // Carregar o JSON original
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo para leitura");
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *file_content = (char *)malloc(file_size + 1);
    if (!file_content) {
        perror("Erro ao alocar memória para o conteúdo do arquivo");
        fclose(file);
        return 0;
    }

    fread(file_content, 1, file_size, file);
    file_content[file_size] = '\0';
    fclose(file);

    cJSON *json = cJSON_Parse(file_content);
    free(file_content);

    if (!json) {
        printf("Erro ao analisar o JSON: %s\n", cJSON_GetErrorPtr());
        return 0;
    }

    // Atualizar os campos necessários
    cJSON_ReplaceItemInObject(json, "fcnt", cJSON_CreateNumber(cfg->fcnt));

    // Salvar o JSON atualizado
    file = fopen(filename, "w");
    if (!file) {
        perror("Erro ao abrir o arquivo para escrita");
        cJSON_Delete(json);
        return 0;
    }

    char *json_string = cJSON_Print(json);
    if (!json_string) {
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

    printf("Criando socket UDP...\n");
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        return -1;
    }
    printf("Socket criado com sucesso.\n");

    // Configurando o endereço do servidor (127.0.0.1:1700)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1700);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Erro ao configurar o endereço do servidor");
        close(sockfd);
        return -1;
    }

    printf("Enviando pacote para o servidor local (127.0.0.1:1700)...\n");
    if (sendto(sockfd, packet, packet_len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao enviar pacote");
        close(sockfd);
        return -1;
    }
    printf("Pacote enviado para o servidor local com sucesso!\n");

    close(sockfd);
    return 0;
}

int main() {
    printf("[LoRaWAN] Simulador de dispositivo iniciado\n");

    Config cfg;
    printf("Carregando configurações do arquivo config/Config.json...\n");
    if (!load_config("config/Config.json", &cfg)) {
        printf("Erro ao carregar o arquivo config/Config.json\n");
        return 1;
    }
    printf("Configurações carregadas com sucesso.\n");

    // Depuração: Exibindo os valores carregados do Config.json
    printf("Debug: DEVEUI: %s\n", cfg.deveui);
    printf("Debug: DEVADDR: %08X\n", cfg.devaddr);
    printf("Debug: NWKSKEY: ");
    for (int i = 0; i < 16; i++) {
        printf("%02X", cfg.nwkskey[i]);
    }
    printf("\n");
    printf("Debug: APPSKEY: ");
    for (int i = 0; i < 16; i++) {
        printf("%02X", cfg.appskey[i]);
    }
    printf("\n");
    printf("Debug: FCNT: %d\n", cfg.fcnt);
    printf("Debug: FPORT: %d\n", cfg.fport);
    printf("Debug: PAYLOAD: ");
    for (int i = 0; i < cfg.payload_len; i++) {
        printf("%02X ", cfg.payload[i]);
    }
    printf("\n");

    // Proteção: Verifica se o DEVEUI foi alterado
    const char *expected_deveui = "70B3D57ED007035A";
    if (strcmp(cfg.deveui, expected_deveui) != 0) {
        printf("Erro: DEVEUI foi alterado inesperadamente! Valor atual: %s\n", cfg.deveui);
        return 1;
    }

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

    // Depuração: Exibindo o pacote montado
    printf("Debug: Pacote montado (hex): ");
    for (int i = 0; i < packet_len; i++) {
        printf("%02X ", packet[i]);
    }
    printf("\n");

    printf("Calculando o MIC...\n");
    printf("Debug: Antes de calcular o MIC (packet_len=%d): ", packet_len);
    for (int i = 0; i < packet_len; i++) {
        printf("%02X ", packet[i]);
    }
    printf("\n");

    int mic_len = lorawan_append_mic(packet, packet_len, cfg.devaddr, cfg.fcnt, cfg.nwkskey);
    if (mic_len != 4) {
        printf("Erro ao calcular MIC\n");
        return 1;
    }
    packet_len += mic_len;
    printf("MIC calculado com sucesso.\n");

    // Depuração: Exibindo o pacote com o MIC
    printf("Debug: Pacote com MIC (hex): ");
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

    printf("Salvando o novo valor de fcnt no config/Config.json...\n");
    if (!save_config("config/Config.json", &cfg)) {
        printf("Erro ao salvar o arquivo config/Config.json\n");
        return 1;
    }
    printf("Config.json atualizado com sucesso.\n");

    return 0;
}
