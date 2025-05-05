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
// Fim bloco de log

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
    } else if (sent_len != (ssize_t)strlen(ack_message)) {
        printf("[Gateway] Erro: ACK enviado parcialmente (%ld bytes).\n", sent_len);
    } else {
        printf("[Gateway] ACK enviado com sucesso para %s:%d.\n",
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

    while (processed_count < device_count) {
        // Recebe pacotes
        recv_len = recvfrom(sockfd, recv_buffer, BUFFER_SIZE, 0,
                            (struct sockaddr *)&client_addr, &addr_len);
    
        if (recv_len < 0) {
            perror("[Gateway] Erro ao receber dados");
            escreverlog("[Gateway] Erro ao receber dados");
            continue;
        }
    
        // Log do pacote recebido
        escreverlog("[Gateway] Pacote recebido de %s:%d com %ld bytes",
                    inet_ntoa(client_addr.sin_addr),
                    ntohs(client_addr.sin_port),
                    recv_len);
    
        // Converte para hexadecimal para log
        char hex[3 * BUFFER_SIZE] = {0};
        for (ssize_t i = 0; i < recv_len; ++i) {
            char temp[4];
            snprintf(temp, sizeof(temp), "%02X ", recv_buffer[i]);
            strcat(hex, temp);
        }
        escreverlog("[Gateway] Conteúdo (hex): %s", hex);
    
        // Envia ACK para o cliente
        send_ack(sockfd, &client_addr, addr_len);
    
        // Extrair os primeiros 4 bytes como devaddr
        if (recv_len < 4) {
            escreverlog("Pacote muito curto para conter DevAddr");
            continue;
        }
        
        // Corrigido: Extrai DevAddr corretamente do buffer
        char devaddr_hex[9] = {0};
        snprintf(devaddr_hex, sizeof(devaddr_hex), "%02X%02X%02X%02X", 
        recv_buffer[1], recv_buffer[2], recv_buffer[3], recv_buffer[4]);

        // Log do DevAddr extraído
        escreverlog("[Gateway] DevAddr extraído (hex): %s", devaddr_hex);
    
        // Procurar dispositivo correspondente
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
    
        // Codificar payload em Base64
        unsigned char base64_output[BASE64_BUFFER_SIZE];
        size_t base64_len = 0;
        mbedtls_base64_encode(base64_output, sizeof(base64_output), &base64_len, recv_buffer, recv_len);
    
        // Criar objeto rxpk
        cJSON *rxpk = cJSON_CreateObject();
        cJSON_AddNumberToObject(rxpk, "tmst", (unsigned)time(NULL)); // timestamp fictício
        cJSON_AddStringToObject(rxpk, "time", "2025-05-05T18:00:00Z");
        cJSON_AddNumberToObject(rxpk, "chan", 0);
        cJSON_AddNumberToObject(rxpk, "rfch", 0);
        cJSON_AddNumberToObject(rxpk, "freq", 868.3);
        cJSON_AddNumberToObject(rxpk, "stat", 1);
        cJSON_AddStringToObject(rxpk, "modu", "LORA");
        cJSON_AddStringToObject(rxpk, "datr", "SF7BW125");
        cJSON_AddStringToObject(rxpk, "codr", "4/5");
        cJSON_AddNumberToObject(rxpk, "rssi", -42);
        cJSON_AddNumberToObject(rxpk, "lsnr", 5.5);
        cJSON_AddNumberToObject(rxpk, "size", recv_len);
        cJSON_AddStringToObject(rxpk, "data", (char *)base64_output);
    
        // Criar array rxpk e raiz
        cJSON *rxpk_array = cJSON_CreateArray();
        cJSON_AddItemToArray(rxpk_array, rxpk);
        cJSON *root = cJSON_CreateObject();
        cJSON_AddItemToObject(root, "rxpk", rxpk_array);
    
        // Salvar em um arquivo específico para cada dispositivo
        char output_filename[128];
        snprintf(output_filename, sizeof(output_filename), "out/Saida_Dispositivo_%d.json", processed_count + 1);
        
        FILE *saida = fopen(output_filename, "w");
        if (!saida) {
            escreverlog("Erro ao abrir %s para escrita", output_filename);
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

    cJSON_Delete(devices);
    close(sockfd);
    fecharlog();
    return 0;
}
