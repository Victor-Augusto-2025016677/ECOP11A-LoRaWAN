// gateway.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <mbedtls/base64.h> // Inclui o mbedtls para codificação Base64
#include <cjson/cJSON.h>    // Inclui a biblioteca cJSON

#define PORT 1700
#define BUFFER_SIZE 1024
#define BASE64_BUFFER_SIZE ((BUFFER_SIZE + 2) / 3 * 4 + 1)
#define PROTOCOL_VERSION 0x02
#define PUSH_DATA 0x00

// Função para gerar o token (2 bytes aleatórios)
uint16_t generate_token() {
    return (uint16_t)(rand() & 0xFFFF);
}

// Função para encapsular o pacote no formato Semtech UDP Packet Forwarder
int encapsulate_semtech_packet(uint8_t *output_buffer, size_t buffer_size, const char *json_payload, const uint8_t *gateway_eui) {
    // Verificar se o buffer é suficiente para o cabeçalho e o payload
    if (strlen(json_payload) > buffer_size - 12) {
        printf("Erro: Payload JSON excede o tamanho máximo permitido.\n");
        return -1;
    }

    // Inicializar o buffer
    memset(output_buffer, 0, buffer_size);

    // Cabeçalho do pacote
    output_buffer[0] = PROTOCOL_VERSION; // Versão do protocolo
    uint16_t token = generate_token();
    output_buffer[1] = (uint8_t)(token >> 8); // Token (byte alto)
    output_buffer[2] = (uint8_t)(token & 0xFF); // Token (byte baixo)
    output_buffer[3] = PUSH_DATA; // Tipo de pacote (PUSH_DATA)

    // Identificador único do gateway (EUI-64)
    memcpy(&output_buffer[4], gateway_eui, 8);

    // Payload (JSON)
    snprintf((char *)&output_buffer[12], buffer_size - 12, "%s", json_payload);

    return 12 + strlen(json_payload); // Retorna o tamanho total do pacote
}

// Função para carregar os dispositivos do Config.json
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

// Função para enviar confirmação de recebimento (ACK)
void send_ack(int sockfd, struct sockaddr_in *client_addr, socklen_t addr_len) {
    char ack_message[4] = {0}; // Zera o buffer antes de usá-lo
    strcpy(ack_message, "ACK"); // Copia a mensagem "ACK" para o buffer

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
    unsigned char recv_buffer[BUFFER_SIZE]; // Buffer para recepção
    unsigned char send_buffer[BUFFER_SIZE]; // Buffer dedicado para envio
    ssize_t recv_len;

    cJSON *devices = NULL;

    // Carrega os dispositivos do Config.json
    if (!load_devices("config/Config.json", &devices)) {
        printf("Erro ao carregar os dispositivos do Config.json\n");
        return 1;
    }

    int device_count = cJSON_GetArraySize(devices);
    printf("[Gateway] Dispositivos encontrados: %d\n", device_count);

    // Configuração do socket
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

    // Variável para contar pacotes processados
    int processed_count = 0;

    // Identificador único do gateway (EUI-64)
    uint8_t gateway_eui[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    while (processed_count < device_count) {
        // Receber pacote do dispositivo
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

        // Exibir o conteúdo do pacote recebido
        printf("[Gateway] Conteúdo do pacote (hex): ");
        for (ssize_t i = 0; i < recv_len; ++i) {
            printf("%02X ", recv_buffer[i]);
        }
        printf("\n");

        // Enviar confirmação de recebimento (ACK)
        send_ack(sockfd, &client_addr, addr_len);

        // Criar um objeto JSON para encapsular os dados
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

        // Encapsula o pacote no formato Semtech UDP Packet Forwarder
        memset(send_buffer, 0, BUFFER_SIZE); // Zera o buffer de envio
        int packet_len = encapsulate_semtech_packet(send_buffer, sizeof(send_buffer), json_string, gateway_eui);
        if (packet_len < 0) {
            printf("Erro ao encapsular o pacote no formato Semtech UDP Packet Forwarder\n");
            free(json_string);
            cJSON_Delete(root);
            continue;
        }

        // Envia o pacote encapsulado
        if (sendto(sockfd, send_buffer, packet_len, 0, (struct sockaddr *)&client_addr, addr_len) < 0) {
            perror("Erro ao enviar pacote encapsulado");
        } else {
            printf("[Gateway] Pacote encapsulado enviado com sucesso.\n");
        }

        free(json_string);
        cJSON_Delete(root);

        // Incrementa o contador de pacotes processados
        processed_count++;
        printf("[Gateway] Pacotes processados: %d/%d\n", processed_count, device_count);
    }

    printf("[Gateway] Todos os pacotes foram processados. Encerrando...\n");

    // Liberar memória e fechar o socket
    cJSON_Delete(devices);
    close(sockfd);
    return 0;
}
