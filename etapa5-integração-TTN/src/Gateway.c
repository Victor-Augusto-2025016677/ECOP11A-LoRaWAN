#define _POSIX_C_SOURCE 199309L]
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <netdb.h>
#include <cjson/cJSON.h>

#define BUFFER_SIZE 4096
#define PORTA_GATEWAY 1700
#define PROTOCOL_VERSION 2
#define PUSH_DATA 0x00
#define PUSH_ACK 0x01
#define BASE64_BUFFER_SIZE ((BUFFER_SIZE + 2) / 3 * 4 + 1)

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

char* carregar_config_dispositivos(const char *caminho) {
    FILE *arquivo = fopen(caminho, "r");
    if (!arquivo) {
        escreverlog("Erro ao abrir arquivo de configuração: %s", strerror(errno));
        return NULL;
    }

    fseek(arquivo, 0, SEEK_END);
    long tamanho = ftell(arquivo);
    rewind(arquivo);

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
        close(sockfd);
        return 1;
    }

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

    close(sockfd);
    cJSON_Delete(config_json);
    return 0;
}
