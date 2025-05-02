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
#include <time.h>

#include "lorawan.h"
#include "aes_cmac.h"
#include "Config.h"

int send_to_gateway(int sockfd, const uint8_t *packet, int packet_len, struct sockaddr_in *server_addr);
int wait_for_ack(int sockfd);
int save_config(const char *filename, const Config *configs, int device_count);

int save_config(const char *filename, const Config *configs, int device_count) {
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

    cJSON *devices = cJSON_GetObjectItemCaseSensitive(json, "devices");
    if (!cJSON_IsArray(devices)) {
        printf("Erro: 'devices' não é um array no Config.json\n");
        cJSON_Delete(json);
        return 0;
    }

    for (int i = 0; i < device_count; i++) {
        cJSON *device = cJSON_GetArrayItem(devices, i);
        if (!cJSON_IsObject(device)) {
            continue;
        }

        cJSON_ReplaceItemInObject(device, "fcnt", cJSON_CreateNumber(configs[i].fcnt));
    }

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

int send_to_gateway(int sockfd, const uint8_t *packet, int packet_len, struct sockaddr_in *server_addr) {
    if (sendto(sockfd, packet, packet_len, 0, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("[Main] Erro ao enviar pacote");
        return -1;
    }
    printf("[Main] Pacote enviado para o Gateway.\n");
    return 0;
}

int wait_for_ack(int sockfd) {
    char ack_buffer[64] = {0};
    struct sockaddr_in from_addr;
    socklen_t addr_len = sizeof(from_addr);

    ssize_t ack_len = recvfrom(sockfd, ack_buffer, sizeof(ack_buffer) - 1, 0,
                               (struct sockaddr *)&from_addr, &addr_len);

    if (ack_len < 0) {
        perror("[Main] Erro ao receber ACK do Gateway");
        return 0;
    }

    ack_buffer[ack_len] = '\0';
    printf("[Main] Resposta recebida do Gateway: %s\n", ack_buffer);

    if (strncmp(ack_buffer, "ACK", 3) == 0) {
        printf("[Main] ACK recebido do Gateway.\n");
        return 1;
    }

    printf("[Main] Resposta inesperada do Gateway: %s\n", ack_buffer);
    return 0;
}

int main() {
    printf("[LoRaWAN] Simulador de dispositivo iniciado\n");

    Config configs[MAX_DEVICES];
    int device_count = 0;

    printf("Carregando configurações do arquivo Config.json...\n");
    if (!load_config("config/Config.json", configs, &device_count)) {
        printf("Erro ao carregar o arquivo Config.json\n");
        return 1;
    }
    printf("Configurações carregadas com sucesso. Dispositivos encontrados: %d\n", device_count);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        return 1;
    }
    printf("Socket UDP criado com sucesso.\n");

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1700);
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Erro ao configurar o endereço do servidor");
        close(sockfd);
        return 1;
    }

    for (int i = 0; i < device_count; i++) {
        Config *cfg = &configs[i];

        printf("Montando o pacote para o dispositivo %d (DevEUI: %s)...\n", i + 1, cfg->deveui);

        uint8_t packet[64];
        int packet_len = lorawan_build_uplink(
            packet, cfg->devaddr, cfg->nwkskey, cfg->appskey,
            cfg->fcnt, cfg->fport, cfg->payload, cfg->payload_len
        );

        if (packet_len <= 0) {
            printf("Erro ao montar pacote para o dispositivo %d\n", i + 1);
            continue;
        }

        printf("Pacote montado para o dispositivo %d: ", i + 1);
        for (int j = 0; j < packet_len; j++) {
            printf("%02X ", packet[j]);
        }
        printf("\n");

        int ack_received = 0;
        while (!ack_received) {
            printf("Enviando pacote para o dispositivo %d...\n", i + 1);
            if (send_to_gateway(sockfd, packet, packet_len, &server_addr) != 0) {
                printf("Erro ao enviar pacote para o dispositivo %d\n", i + 1);
                continue;
            }

            printf("Pacote enviado com sucesso para o dispositivo %d! Aguardando ACK...\n", i + 1);

            ack_received = wait_for_ack(sockfd);

            if (!ack_received) {
                struct timespec delay;
                delay.tv_sec = 1;
                delay.tv_nsec = 0;
                nanosleep(&delay, NULL);
            }
        }

        printf("ACK recebido do gateway para o dispositivo %d.\n", i + 1);
        cfg->fcnt += 1;

        if (!save_config("config/Config.json", configs, device_count)) {
            printf("Erro ao salvar o arquivo Config.json para o dispositivo %d\n", i + 1);
        }
    }

    close(sockfd);
    printf("Socket fechado.\n");

    return 0;
}
