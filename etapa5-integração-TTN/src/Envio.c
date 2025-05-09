/*
 * Simple TTN forwarder - ANSI C
 * Lê arquivos JSON contendo pacotes rxpk e envia via Semtech UDP (PUSH_DATA)
 * Inicia PULL_DATA loop para downlink (sem tratar resposta ainda)
 * Dependências: cJSON, pthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <cjson/cJSON.h>
#include <netdb.h>

#define MAX_DEVICES 10
#define GATEWAY_EUI_LEN 8

uint8_t gateway_eui[GATEWAY_EUI_LEN] = {0x75, 0x6E, 0x69, 0x66, 0x65, 0x69, 0x12, 0x09};

char server_address[256] = "router.us.thethings.network";
int server_port = 1700;
int sockfd;
struct sockaddr_in server_addr;

uint16_t token() {
    return (uint16_t)(rand() & 0xFFFF);
}

void build_header(uint8_t *buf, uint16_t token, uint8_t identifier) {
    buf[0] = 0x02;                      // protocol version
    buf[1] = (token >> 8) & 0xFF;       // random token
    buf[2] = token & 0xFF;
    buf[3] = identifier;                // PUSH_DATA = 0x00, PULL_DATA = 0x02
    memcpy(buf + 4, gateway_eui, 8);    // Gateway EUI
}

void *pull_data_loop(void *arg) {
    (void)arg;  // evitar warning de parâmetro não usado
    uint8_t buf[12];
    while (1) {
        uint16_t tk = token();
        build_header(buf, tk, 0x02); // PULL_DATA
        sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        sleep(25);
    }
    return NULL;
}

void send_push_data(cJSON *rxpk_array) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "rxpk", rxpk_array); // transfere propriedade

    char *json_str = cJSON_PrintUnformatted(root);
    size_t json_len = strlen(json_str);

    uint8_t buf[12 + json_len];
    build_header(buf, token(), 0x00); // PUSH_DATA
    memcpy(buf + 12, json_str, json_len);

    sendto(sockfd, buf, 12 + json_len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("Enviado PUSH_DATA (%lu bytes)\n", 12 + json_len);

    free(json_str);
    cJSON_Delete(root);
}

void process_device_file(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) return;

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *data = malloc(len + 1);
    fread(data, 1, len, fp);
    data[len] = '\0';
    fclose(fp);

    cJSON *root = cJSON_Parse(data);
    if (!root) {
        free(data);
        return;
    }

    cJSON *rxpk = cJSON_GetObjectItem(root, "rxpk");
    if (rxpk && cJSON_IsArray(rxpk)) {
        cJSON *rxpk_copy = cJSON_Duplicate(rxpk, 1); // duplicar pois deletamos root depois
        send_push_data(rxpk_copy);
    }

    cJSON_Delete(root);
    free(data);
}

void load_config(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return;
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *data = malloc(len + 1);
    fread(data, 1, len, fp);
    data[len] = '\0';
    fclose(fp);

    cJSON *cfg = cJSON_Parse(data);
    if (!cfg) {
        free(data);
        return;
    }

    cJSON *srv = cJSON_GetObjectItem(cfg, "server");
    cJSON *prt = cJSON_GetObjectItem(cfg, "port");
    if (cJSON_IsString(srv)) strncpy(server_address, srv->valuestring, sizeof(server_address)-1);
    if (cJSON_IsNumber(prt)) server_port = prt->valueint;

    cJSON_Delete(cfg);
    free(data);
}

int main() {
    //srand(time(NULL));
    load_config("config.json");

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    struct hostent *host = gethostbyname(server_address);
    if (!host) {
        fprintf(stderr, "Erro ao resolver o hostname: %s\n", server_address);
        return 1;
    }
    memcpy(&server_addr.sin_addr, host->h_addr_list[0], host->h_length);

    pthread_t pull_thread;
    pthread_create(&pull_thread, NULL, pull_data_loop, NULL);

    for (int i = 1; i <= MAX_DEVICES; i++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "out/Saida_Dispositivo_%d", i);
        process_device_file(filename);
        usleep(500000); // 500ms
    }

    pthread_join(pull_thread, NULL);
    close(sockfd);
    return 0;
}
