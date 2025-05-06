#define _POSIX_C_SOURCE 199309L]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
<<<<<<< HEAD
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <netdb.h>
#include <cjson/cJSON.h>
=======
#include <arpa/inet.h>
#include <mbedtls/base64.h>
#include <cjson/cJSON.h>
#include <time.h>
#include <stdarg.h>
>>>>>>> parent of 1bb18fa (Aperfeiçoando Gateway antes de programar o envio)

#define BUFFER_SIZE 4096
#define PORTA_GATEWAY 1700
#define PROTOCOL_VERSION 2
#define PUSH_DATA 0x00
<<<<<<< HEAD
#define PUSH_ACK 0x01
#define BASE64_BUFFER_SIZE ((BUFFER_SIZE + 2) / 3 * 4 + 1)
=======

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
>>>>>>> parent of 1bb18fa (Aperfeiçoando Gateway antes de programar o envio)

void escreverlog(const char *formato, ...) {
    va_list args;
    va_start(args, formato);
    vprintf(formato, args);
    printf("\n");
    va_end(args);
}

void send_ack(int sockfd, struct sockaddr_in *cliaddr, socklen_t len, uint8_t *data) {
    uint8_t ack[4];
    ack[0] = PROTOCOL_VERSION;
    ack[1] = data[1];
    ack[2] = data[2];
    ack[3] = PUSH_ACK;
    sendto(sockfd, ack, sizeof(ack), 0, (struct sockaddr *)cliaddr, len);
    escreverlog("ACK enviado para %s:%d", inet_ntoa(cliaddr->sin_addr), ntohs(cliaddr->sin_port));
}
// Fim bloco de log

<<<<<<< HEAD
char* carregar_config_dispositivos(const char *caminho) {
    FILE *arquivo = fopen(caminho, "r");
    if (!arquivo) {
        escreverlog("Erro ao abrir arquivo de configuração: %s", strerror(errno));
        return NULL;
=======
int load_devices(const char *filename, cJSON **devices) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo Config.json");
        escreverlog("Erro ao abrir o arquivo Config.json");
        return 0;
>>>>>>> parent of 1bb18fa (Aperfeiçoando Gateway antes de programar o envio)
    }

    fseek(arquivo, 0, SEEK_END);
    long tamanho = ftell(arquivo);
    rewind(arquivo);

<<<<<<< HEAD
    char *conteudo = malloc(tamanho + 1);
    fread(conteudo, 1, tamanho, arquivo);
    conteudo[tamanho] = '\0';

    fclose(arquivo);
    return conteudo;
}

cJSON* buscar_dispositivo_por_devaddr(cJSON *devices, const char *devaddr_hex) {
    cJSON *item = NULL;
    cJSON_ArrayForEach(item, devices) {
        cJSON *devaddr = cJSON_GetObjectItem(item, "devaddr");
        if (devaddr && strcmp(devaddr->valuestring, devaddr_hex) == 0) {
            return item;
        }
=======
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
>>>>>>> parent of 1bb18fa (Aperfeiçoando Gateway antes de programar o envio)
    }
    return NULL;
}

void base64_encode(const unsigned char *input, size_t input_len, char *output) {
    static const char *base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i = 0, j = 0;
    unsigned char a3[3];
    unsigned char a4[4];

    while (input_len--) {
        a3[i++] = *(input++);
        if (i == 3) {
            a4[0] = (a3[0] & 0xfc) >> 2;
            a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
            a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
            a4[3] = a3[2] & 0x3f;

            for (i = 0; i < 4; i++) *output++ = base64_chars[a4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) a3[j] = '\0';

        a4[0] = (a3[0] & 0xfc) >> 2;
        a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
        a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
        a4[3] = a3[2] & 0x3f;

        for (j = 0; j < i + 1; j++) *output++ = base64_chars[a4[j]];
        while ((i++ < 3)) *output++ = '=';
    }

    *output = '\0';
}

void enviar_para_servidor(const char *json_str) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        escreverlog("Erro ao criar socket para envio: %s", strerror(errno));
        return;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORTA_GATEWAY);

    struct hostent *host = gethostbyname("eu1.cloud.thethings.network");
    if (!host) {
        escreverlog("Erro ao resolver host do TTN");
        close(sockfd);
        return;
    }

    memcpy(&server_addr.sin_addr, host->h_addr_list[0], host->h_length);

    unsigned char message[BUFFER_SIZE];
    memset(message, 0, sizeof(message));

    message[0] = PROTOCOL_VERSION;
    message[1] = rand() % 256;
    message[2] = rand() % 256;
    message[3] = PUSH_DATA;

    for (int i = 0; i < 8; ++i) message[4 + i] = (unsigned char)(i * 11 + 33);

    size_t header_len = 12;
    size_t body_len = strlen(json_str);
    memcpy(message + header_len, json_str, body_len);

    sendto(sockfd, message, header_len + body_len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    escreverlog("Pacote enviado para o TTN (%ld bytes)", header_len + body_len);
    close(sockfd);
}

int main() {
<<<<<<< HEAD
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        escreverlog("Erro ao criar socket UDP: %s", strerror(errno));
        return 1;
    }

    struct sockaddr_in gateway_addr, client_addr;
    socklen_t len = sizeof(client_addr);
    memset(&gateway_addr, 0, sizeof(gateway_addr));
    gateway_addr.sin_family = AF_INET;
    gateway_addr.sin_port = htons(PORTA_GATEWAY);
    gateway_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&gateway_addr, sizeof(gateway_addr)) < 0) {
        escreverlog("Erro ao fazer bind: %s", strerror(errno));
=======
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
>>>>>>> parent of 1bb18fa (Aperfeiçoando Gateway antes de programar o envio)
        close(sockfd);
        return 1;
    }

<<<<<<< HEAD
    char *json_raw = carregar_config_dispositivos("config/Config.json");
    if (!json_raw) return 1;

    cJSON *config_json = cJSON_Parse(json_raw);
    free(json_raw);
    cJSON *devices = cJSON_GetObjectItem(config_json, "devices");

    unsigned char buffer[BUFFER_SIZE];
    while (1) {
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &len);
        if (n < 0) continue;

        send_ack(sockfd, &client_addr, len, buffer);

        if (n < 5) continue;  // mínimo necessário para ter DevAddr

        char devaddr_hex[9];
        snprintf(devaddr_hex, sizeof(devaddr_hex), "%02X%02X%02X%02X", buffer[1], buffer[2], buffer[3], buffer[4]);

        cJSON *dispositivo = buscar_dispositivo_por_devaddr(devices, devaddr_hex);
        if (!dispositivo) {
            escreverlog("Dispositivo %s não encontrado", devaddr_hex);
            continue;
        }

        cJSON *gw = cJSON_GetObjectItem(dispositivo, "gateway");

        char base64_data[BASE64_BUFFER_SIZE];
        base64_encode(buffer, n, base64_data);

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        unsigned long long timestamp = ts.tv_sec * 1000ULL + ts.tv_nsec / 1000000;

        char iso_time[32];
        struct tm *gmt = gmtime(&ts.tv_sec);
        strftime(iso_time, sizeof(iso_time), "%Y-%m-%dT%H:%M:%SZ", gmt);

        cJSON *rxpk = cJSON_CreateObject();
        cJSON_AddNumberToObject(rxpk, "tmst", (unsigned int)timestamp);
        cJSON_AddStringToObject(rxpk, "time", iso_time);
        cJSON_AddNumberToObject(rxpk, "chan", cJSON_GetObjectItem(gw, "chan")->valueint);
        cJSON_AddNumberToObject(rxpk, "rfch", cJSON_GetObjectItem(gw, "rfch")->valueint);
        cJSON_AddNumberToObject(rxpk, "freq", cJSON_GetObjectItem(gw, "freq")->valuedouble);
        cJSON_AddNumberToObject(rxpk, "stat", 1);
        cJSON_AddStringToObject(rxpk, "modu", "LORA");
        cJSON_AddStringToObject(rxpk, "datr", cJSON_GetObjectItem(gw, "datr")->valuestring);
        cJSON_AddStringToObject(rxpk, "codr", cJSON_GetObjectItem(gw, "codr")->valuestring);
        cJSON_AddNumberToObject(rxpk, "rssi", cJSON_GetObjectItem(gw, "rssi")->valueint);
        cJSON_AddNumberToObject(rxpk, "lsnr", cJSON_GetObjectItem(gw, "lsnr")->valuedouble);
        cJSON_AddNumberToObject(rxpk, "size", n);
        cJSON_AddStringToObject(rxpk, "data", base64_data);

        cJSON *rxpk_array = cJSON_CreateArray();
        cJSON_AddItemToArray(rxpk_array, rxpk);

        cJSON *root = cJSON_CreateObject();
        cJSON_AddItemToObject(root, "rxpk", rxpk_array);

        char *json_envio = cJSON_PrintUnformatted(root);
        enviar_para_servidor(json_envio);

        cJSON_Delete(root);
        free(json_envio);
    }

=======
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
    
            // Obter timestamp real em milissegundos
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            unsigned long long timestamp_ms = (unsigned long long)(ts.tv_sec) * 1000 + (ts.tv_nsec / 1000000);
    
            // Formatar time ISO 8601
            char time_iso8601[40];
            struct tm *tm_info = gmtime(&ts.tv_sec);
            strftime(time_iso8601, sizeof(time_iso8601), "%Y-%m-%dT%H:%M:%SZ", tm_info);
    
            // Acessar informações do gateway
            cJSON *gateway = cJSON_GetObjectItem(device, "gateway");
    
            // Criar objeto rxpk
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
>>>>>>> parent of 1bb18fa (Aperfeiçoando Gateway antes de programar o envio)
    close(sockfd);
    cJSON_Delete(config_json);
    return 0;
}