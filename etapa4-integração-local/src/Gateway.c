#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <mbedtls/base64.h>
#include <cjson/cJSON.h>

#define PORT 1700
#define BUFFER_SIZE 1024
#define BASE64_BUFFER_SIZE ((BUFFER_SIZE + 2) / 3 * 4 + 1)
#define PROTOCOL_VERSION 0x02
#define PUSH_DATA 0x00

uint16_t generate_token() {
    return (uint16_t)(rand() & 0xFFFF);
}

int encapsulate_semtech_packet(uint8_t *output_buffer, size_t buffer_size, const char *json_payload, const uint8_t *gateway_eui) {
    if (strlen(json_payload) > buffer_size - 12) {
        printf("Erro: Payload JSON excede o tamanho máximo permitido.\n");
        return -1;
    }

    memset(output_buffer, 0, buffer_size);

    output_buffer[0] = PROTOCOL_VERSION;
    uint16_t token = generate_token();
    output_buffer[1] = (uint8_t)(token >> 8);
    output_buffer[2] = (uint8_t)(token & 0xFF);
    output_buffer[3] = PUSH_DATA;

    memcpy(&output_buffer[4], gateway_eui, 8);

    snprintf((char *)&output_buffer[12], buffer_size - 12, "%s", json_payload);

    return 12 + strlen(json_payload);
}

int load_devices(const char *filename, cJSON **devices) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo Config.json");
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

    *devices = cJSON_GetObjectItemCaseSensitive(json, "devices");
    if (!cJSON_IsArray(*devices)) {
        printf("Erro: 'devices' não é um array no Config.json\n");
        cJSON_Delete(json);
        return 0;
    }

    return 1;
}

void send_ack(int sockfd, struct sockaddr_in *client_addr, socklen_t addr_len) {
    char ack_message[4] = {0};
    strcpy(ack_message, "ACK");

    ssize_t sent_len = sendto(sockfd, ack_message, strlen(ack_message), 0,
                              (struct sockaddr *)client_addr, addr_len);
    if (sent_len < 0) {
        perror("[Gateway] Erro ao enviar ACK");
    } else if (sent_len != (ssize_t)strlen(ack_message)) {
        printf("[Gateway] Erro: ACK enviado parcialmente (%ld bytes).\n", sent_len);
    } else {
        printf("[Gateway] ACK enviado para o dispositivo.\n");
    }
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    unsigned char recv_buffer[BUFFER_SIZE];
    unsigned char send_buffer[BUFFER_SIZE];
    ssize_t recv_len;

    cJSON *devices = NULL;

    if (!load_devices("config/Config.json", &devices)) {
        printf("Erro ao carregar os dispositivos do Config.json\n");
        return 1;
    }

    int device_count = cJSON_GetArraySize(devices);
    printf("[Gateway] Dispositivos encontrados: %d\n", device_count);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao fazer bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("[Gateway] Aguardando pacotes na porta %d...\n", PORT);

    int processed_count = 0;

    uint8_t gateway_eui[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    while (processed_count < device_count) {
        recv_len = recvfrom(sockfd, recv_buffer, BUFFER_SIZE, 0,
                            (struct sockaddr *)&client_addr, &addr_len);

        if (recv_len < 0) {
            perror("[Gateway] Erro ao receber dados");
            continue;
        }

        printf("[Gateway] Pacote recebido (%ld bytes) de %s:%d\n",
               recv_len,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        printf("[Gateway] Conteúdo do pacote (hex): ");
        for (ssize_t i = 0; i < recv_len; ++i) {
            printf("%02X ", recv_buffer[i]);
        }
        printf("\n");

        send_ack(sockfd, &client_addr, addr_len);

        cJSON *root = cJSON_CreateObject();
        if (!root) {
            printf("Erro ao criar objeto JSON\n");
            continue;
        }

        cJSON_AddStringToObject(root, "data", "example_payload");
        char *json_string = cJSON_PrintUnformatted(root);
        if (!json_string) {
            printf("Erro ao criar string JSON\n");
            cJSON_Delete(root);
            continue;
        }

        memset(send_buffer, 0, BUFFER_SIZE);
        int packet_len = encapsulate_semtech_packet(send_buffer, sizeof(send_buffer), json_string, gateway_eui);
        if (packet_len < 0) {
            printf("Erro ao encapsular o pacote no formato Semtech UDP Packet Forwarder\n");
            free(json_string);
            cJSON_Delete(root);
            continue;
        }

        if (sendto(sockfd, send_buffer, packet_len, 0, (struct sockaddr *)&client_addr, addr_len) < 0) {
            perror("Erro ao enviar pacote encapsulado");
        } else {
            printf("[Gateway] Pacote encapsulado enviado com sucesso.\n");
        }

        free(json_string);
        cJSON_Delete(root);

        processed_count++;
        printf("[Gateway] Pacotes processados: %d/%d\n", processed_count, device_count);
    }

    printf("[Gateway] Todos os pacotes foram processados. Encerrando...\n");

    cJSON_Delete(devices);
    close(sockfd);
    return 0;
}
