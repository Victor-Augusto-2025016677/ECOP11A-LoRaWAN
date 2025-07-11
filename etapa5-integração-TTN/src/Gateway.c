#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <mbedtls/base64.h>
#include <cjson/cJSON.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>

#define PORT 1700
#define BUFFER_SIZE 1024
#define BASE64_BUFFER_SIZE ((BUFFER_SIZE + 2) / 3 * 4 + 1)
#define PROTOCOL_VERSION 0x02
#define PUSH_DATA 0x00

FILE *log_file = NULL;
char nomedolog[64];

void gerar_nome_log(char *buffer, size_t tamanho) {
    time_t agora = time(NULL);
    struct tm *tm_info = localtime(&agora);
    strftime(buffer, tamanho, "logs/log_Gateway_%Y%m%d_%H%M%S.txt", tm_info);
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

int load_devices(const char *filename, cJSON **devices, cJSON **root_json) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo Config.json");
        escreverlog("Erro ao abrir o arquivo Config.json: %s", strerror(errno));
        return 0;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char *file_content = (char *)malloc(file_size + 1);
    if (!file_content) {
        perror("Erro ao alocar memória");
        escreverlog("Erro ao alocar memória: %s", strerror(errno));
        fclose(file);
        return 0;
    }

    fread(file_content, 1, file_size, file);
    file_content[file_size] = '\0';
    fclose(file);

    cJSON *json = cJSON_Parse(file_content);
    free(file_content);

    if (!json) {
        escreverlog("Erro ao analisar o JSON: %s", cJSON_GetErrorPtr());
        return 0;
    }

    cJSON *devs = cJSON_GetObjectItemCaseSensitive(json, "devices");
    if (!cJSON_IsArray(devs)) {
        escreverlog("Erro: 'devices' não é um array no Config.json");
        cJSON_Delete(json);
        return 0;
    }

    *devices = devs;
    *root_json = json;
    return 1;
}

void send_ack(int sockfd, struct sockaddr_in *client_addr, socklen_t addr_len) {
    char ack_message[4] = "ACK";
    ssize_t sent_len = sendto(sockfd, ack_message, strlen(ack_message), 0,
                              (struct sockaddr *)client_addr, addr_len);

    if (sent_len < 0) {
        perror("[Gateway] Erro ao enviar ACK");
        escreverlog("[Gateway] Erro ao enviar ACK: %s", strerror(errno));
    } else if (sent_len != (ssize_t)strlen(ack_message)) {
        escreverlog("[Gateway] Erro: ACK enviado parcialmente (%ld bytes)", sent_len);
    } else {
        escreverlog("[Gateway] ACK enviado com sucesso para %s:%d",
                    inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
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
    ssize_t recv_len;
    cJSON *devices = NULL;
    cJSON *root_json = NULL;

    if (!load_devices("config/Config.json", &devices, &root_json)) {
        escreverlog("Falha ao carregar dispositivos. Encerrando...");
        return 1;
    }

    int device_count = cJSON_GetArraySize(devices);
    escreverlog("[Gateway] Dispositivos encontrados: %d", device_count);

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket");
        escreverlog("Erro ao criar socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao fazer bind");
        escreverlog("Erro ao fazer bind: %s", strerror(errno));
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    escreverlog("[Gateway] Aguardando pacotes na porta %d", PORT);
    int processed_count = 0;

    while (processed_count < device_count) {
        recv_len = recvfrom(sockfd, recv_buffer, BUFFER_SIZE, 0,
                            (struct sockaddr *)&client_addr, &addr_len);

        if (recv_len < 0) {
            perror("[Gateway] Erro ao receber dados");
            escreverlog("[Gateway] Erro ao receber dados: %s", strerror(errno));
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

        if (recv_len < 4) {
            escreverlog("Pacote muito curto para conter DevAddr");
            continue;
        }

        char devaddr_hex[9] = {0};
        snprintf(devaddr_hex, sizeof(devaddr_hex), "%02X%02X%02X%02X", 
        recv_buffer[1], recv_buffer[2], recv_buffer[3], recv_buffer[4]);

        escreverlog("[Gateway] DevAddr extraído (hex): %s", devaddr_hex);

        cJSON *device = NULL;
        cJSON *item = NULL;
        cJSON_ArrayForEach(item, devices) {
            cJSON *devaddr = cJSON_GetObjectItem(item, "devaddr");
            if (devaddr && strcmp(devaddr->valuestring, devaddr_hex) == 0) {
                device = item;
                break;
            }
        }

        if (!device) {
            escreverlog("Dispositivo com DevAddr %s não encontrado", devaddr_hex);
            continue;
        }

        unsigned char base64_output[BASE64_BUFFER_SIZE];
        size_t base64_len = 0;
        mbedtls_base64_encode(base64_output, sizeof(base64_output), &base64_len, recv_buffer, recv_len);

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        unsigned long long timestamp_ms = (unsigned long long)(ts.tv_sec) * 1000 + (ts.tv_nsec / 1000000);

        char time_iso8601[40];
        struct tm *tm_info = gmtime(&ts.tv_sec);
        strftime(time_iso8601, sizeof(time_iso8601), "%Y-%m-%dT%H:%M:%SZ", tm_info);

        cJSON *gateway = cJSON_GetObjectItem(device, "gateway");

        cJSON *rxpk = cJSON_CreateObject();
        cJSON_AddNumberToObject(rxpk, "tmst", (unsigned int)timestamp_ms);
        cJSON_AddStringToObject(rxpk, "time", time_iso8601);
        cJSON_AddNumberToObject(rxpk, "chan", cJSON_GetObjectItem(gateway, "chan")->valueint);
        cJSON_AddNumberToObject(rxpk, "rfch", cJSON_GetObjectItem(gateway, "rfch")->valueint);
        cJSON_AddNumberToObject(rxpk, "freq", cJSON_GetObjectItem(gateway, "freq")->valuedouble);
        cJSON_AddNumberToObject(rxpk, "stat", 1);
        cJSON_AddStringToObject(rxpk, "modu", "LORA");
        cJSON_AddStringToObject(rxpk, "datr", cJSON_GetObjectItem(gateway, "datr")->valuestring);
        cJSON_AddStringToObject(rxpk, "codr", cJSON_GetObjectItem(gateway, "codr")->valuestring);
        cJSON_AddNumberToObject(rxpk, "rssi", cJSON_GetObjectItem(gateway, "rssi")->valueint);
        cJSON_AddNumberToObject(rxpk, "lsnr", cJSON_GetObjectItem(gateway, "lsnr")->valuedouble);
        cJSON_AddNumberToObject(rxpk, "size", recv_len);
        cJSON_AddStringToObject(rxpk, "data", (char *)base64_output);

        cJSON *rxpk_array = cJSON_CreateArray();
        cJSON_AddItemToArray(rxpk_array, rxpk);
        cJSON *root = cJSON_CreateObject();
        cJSON_AddItemToObject(root, "rxpk", rxpk_array);

        char output_filename[128];
        snprintf(output_filename, sizeof(output_filename), "out/Saida_Dispositivo_%d.json", processed_count + 1);

        FILE *saida = fopen(output_filename, "w");
        if (!saida) {
            escreverlog("Erro ao abrir %s para escrita: %s", output_filename, strerror(errno));
        } else {
            char *saida_str = cJSON_Print(root);
            if (saida_str) {
                fputs(saida_str, saida);
                escreverlog("Pacote JSON Semtech salvo em %s", output_filename);
                free(saida_str);
            }
            fclose(saida);
        }

        cJSON_Delete(root);
        processed_count++;
        escreverlog("[Gateway] Pacotes processados: %d/%d", processed_count, device_count);
    }

    printf("[Gateway] Todos os pacotes foram processados. Encerrando...\n");
    escreverlog("[Gateway] Todos os pacotes foram processados. Encerrando");
    cJSON_Delete(root_json);
    close(sockfd);
    fecharlog();
    return 0;
}
