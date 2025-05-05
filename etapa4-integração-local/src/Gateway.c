#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <mbedtls/base64.h>
#include <cjson/cJSON.h>
#include <time.h>
#include <stdarg.h>

#define PORT 1700
#define BUFFER_SIZE 1024
#define BASE64_BUFFER_SIZE ((BUFFER_SIZE + 2) / 3 * 4 + 1)
#define PROTOCOL_VERSION 0x02
#define PUSH_DATA 0x00

// Início bloco de log
FILE *log_file = NULL;
char nomedolog[64];

void gerar_nome_log(char *buffer, size_t tamanho) {
    time_t agora = time(NULL);
    struct tm *tm_info = localtime(&agora);
    strftime(buffer, tamanho, "logs/log_gateway_%Y%m%d_%H%M%S.txt", tm_info);
}

const char* current_time_str() {
    static char buffer[32];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", t);
    return buffer;
}

void iniciarlog() {
    gerar_nome_log(nomedolog, sizeof(nomedolog));
    log_file = fopen(nomedolog, "a");
    if (log_file == NULL) {
        perror("Erro ao abrir o arquivo de log");
        exit(EXIT_FAILURE);
    }
    fprintf(log_file, "========== NOVA EXECUÇÃO (%s) ==========\n\n", current_time_str());
    fflush(log_file);
}

void escreverlog(const char *formato, ...) {
    if (log_file != NULL) {
        va_list args;
        va_start(args, formato);
        fprintf(log_file, "[%s] ", current_time_str());
        vfprintf(log_file, formato, args);
        fprintf(log_file, "\n");
        va_end(args);
        fflush(log_file);
    }
}

void fecharlog() {
    if (log_file != NULL) {
        fprintf(log_file, "\n========== FINAL EXECUÇÃO (%s) ==========\n", current_time_str());
        fclose(log_file);
        log_file = NULL;
    }
}
// Fim bloco de log

uint16_t generate_token() {
    return (uint16_t)(rand() & 0xFFFF);
}

int encapsulate_semtech_packet(uint8_t *output_buffer, size_t buffer_size, const char *json_payload, const uint8_t *gateway_eui) {
    if (strlen(json_payload) > buffer_size - 12) {
        printf("Erro: Payload JSON excede o tamanho máximo permitido.\n");
        escreverlog("Erro: Payload JSON excede o tamanho máximo permitido");
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
        escreverlog("Erro ao abrir o arquivo Config.json");
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *file_content = (char *)malloc(file_size + 1);
    if (!file_content) {
        perror("Erro ao alocar memória");
        escreverlog("Erro ao alocar memória para conteúdo do arquivo");
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
        escreverlog("Erro ao analisar o JSON: %s", cJSON_GetErrorPtr());
        return 0;
    }

    *devices = cJSON_GetObjectItemCaseSensitive(json, "devices");
    if (!cJSON_IsArray(*devices)) {
        printf("Erro: 'devices' não é um array no Config.json\n");
        escreverlog("Erro: 'devices' não é um array no Config.json");
        cJSON_Delete(json);
        return 0;
    }

    return 1;
}

void send_ack(int sockfd, struct sockaddr_in *client_addr, socklen_t addr_len) {
    char ack_message[4] = "ACK";

    ssize_t sent_len = sendto(sockfd, ack_message, strlen(ack_message), 0,
                              (struct sockaddr *)client_addr, addr_len);
    if (sent_len < 0) {
        perror("[Gateway] Erro ao enviar ACK");
        escreverlog("[Gateway] Erro ao enviar ACK");
    } else if (sent_len != (ssize_t)strlen(ack_message)) {
        printf("[Gateway] Erro: ACK enviado parcialmente (%ld bytes).\n", sent_len);
        escreverlog("[Gateway] ACK enviado parcialmente (%ld bytes)", sent_len);
    } else {
        printf("[Gateway] ACK enviado para o dispositivo.\n");
        escreverlog("[Gateway] ACK enviado com sucesso");
    }
}

int main() {
    iniciarlog();
    printf("[Gateway] Iniciando gateway UDP...\n");
    escreverlog("[Gateway] Iniciando gateway UDP");

    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    unsigned char recv_buffer[BUFFER_SIZE];
    unsigned char send_buffer[BUFFER_SIZE];
    ssize_t recv_len;

    cJSON *devices = NULL;

    if (!load_devices("config/Config.json", &devices)) {
        printf("Erro ao carregar os dispositivos do Config.json\n");
        escreverlog("Erro ao carregar os dispositivos do Config.json");
        return 1;
    }

    int device_count = cJSON_GetArraySize(devices);
    printf("[Gateway] Dispositivos encontrados: %d\n", device_count);
    escreverlog("[Gateway] Dispositivos encontrados: %d", device_count);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket");
        escreverlog("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao fazer bind");
        escreverlog("Erro ao fazer bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("[Gateway] Aguardando pacotes na porta %d...\n", PORT);
    escreverlog("[Gateway] Aguardando pacotes na porta %d", PORT);

    int processed_count = 0;
    uint8_t gateway_eui[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

    while (processed_count < device_count) {
        recv_len = recvfrom(sockfd, recv_buffer, BUFFER_SIZE, 0,
                            (struct sockaddr *)&client_addr, &addr_len);

        if (recv_len < 0) {
            perror("[Gateway] Erro ao receber dados");
            escreverlog("[Gateway] Erro ao receber dados");
            continue;
        }

        escreverlog("[Gateway] Pacote recebido de %s:%d com %ld bytes",
                    inet_ntoa(client_addr.sin_addr),
                    ntohs(client_addr.sin_port),
                    recv_len);

        char hex[3 * BUFFER_SIZE] = {0};
        for (ssize_t i = 0; i < recv_len; ++i) {
            char temp[4];
            snprintf(temp, sizeof(temp), "%02X ", recv_buffer[i]);
            strcat(hex, temp);
        }
        escreverlog("[Gateway] Conteúdo (hex): %s", hex);

        send_ack(sockfd, &client_addr, addr_len);

        cJSON *root = cJSON_CreateObject();
        if (!root) {
            escreverlog("Erro ao criar objeto JSON");
            continue;
        }

        cJSON_AddStringToObject(root, "data", "example_payload");
        char *json_string = cJSON_PrintUnformatted(root);
        if (!json_string) {
            escreverlog("Erro ao criar string JSON");
            cJSON_Delete(root);
            continue;
        }

        memset(send_buffer, 0, BUFFER_SIZE);
        int packet_len = encapsulate_semtech_packet(send_buffer, sizeof(send_buffer), json_string, gateway_eui);
        if (packet_len < 0) {
            escreverlog("Erro ao encapsular pacote no formato Semtech");
            free(json_string);
            cJSON_Delete(root);
            continue;
        }

        if (sendto(sockfd, send_buffer, packet_len, 0, (struct sockaddr *)&client_addr, addr_len) < 0) {
            perror("Erro ao enviar pacote encapsulado");
            escreverlog("Erro ao enviar pacote encapsulado");
        } else {
            printf("[Gateway] Pacote encapsulado enviado com sucesso.\n");
            escreverlog("[Gateway] Pacote encapsulado enviado com sucesso");
        }

        free(json_string);
        cJSON_Delete(root);

        processed_count++;
        escreverlog("[Gateway] Pacotes processados: %d/%d", processed_count, device_count);
    }

    printf("[Gateway] Todos os pacotes foram processados. Encerrando...\n");
    escreverlog("[Gateway] Todos os pacotes foram processados. Encerrando");

    cJSON_Delete(devices);
    close(sockfd);
    fecharlog();
    return 0;
}
