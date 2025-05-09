#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <regex.h>
#include <netdb.h>
#include <sys/select.h>
#include <cjson/cJSON.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
#include <limits.h>

#define MAX_DEVICES 10
#define GATEWAY_EUI_LEN 8

uint8_t gateway_eui[GATEWAY_EUI_LEN] = {0x75, 0x6E, 0x69, 0x66, 0x65, 0x69, 0x12, 0x09};
char server_address[256] = "nam1.cloud.thethings.network";
int server_port = 1700;
int sockfd;
struct sockaddr_in server_addr;

uint16_t token() {
    return (uint16_t)(rand() & 0xFFFF);
}

void build_header(uint8_t *buf, uint16_t tk, uint8_t identifier) {
    buf[0] = 0x02;
    buf[1] = (tk >> 8) & 0xFF;
    buf[2] = tk & 0xFF;
    buf[3] = identifier;
    memcpy(buf + 4, gateway_eui, GATEWAY_EUI_LEN);
}

void wait_for_ack(uint16_t expected_token) {
    struct timeval timeout = {1, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sockfd, &fds);

    int ret = select(sockfd + 1, &fds, NULL, NULL, &timeout);
    if (ret > 0) {
        uint8_t buffer[1024];
        socklen_t addr_len = sizeof(server_addr);
        ssize_t len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, &addr_len);

        if (len >= 4 && buffer[3] == 0x01) {
            uint16_t received_token = (buffer[1] << 8) | buffer[2];
            if (received_token == expected_token) {
                printf("✔ PUSH_ACK recebido!\n");
                return;
            }
        }

        printf("⚠ Recebido pacote desconhecido ou token incorreto\n");
    } else {
        printf("✖ Timeout esperando PUSH_ACK\n");
    }
}

void send_push_data(cJSON *rxpk_array) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return;

    cJSON_AddItemToObject(root, "rxpk", rxpk_array);

    char *json_str = cJSON_PrintUnformatted(root);
    if (!json_str) {
        cJSON_Delete(root);
        return;
    }

    size_t json_len = strlen(json_str);
    size_t packet_size = 12 + json_len;
    uint8_t *buf = malloc(packet_size);
    if (!buf) {
        free(json_str);
        cJSON_Delete(root);
        return;
    }

    uint16_t tk = token();
    build_header(buf, tk, 0x00);
    memcpy(buf + 12, json_str, json_len);

    sendto(sockfd, buf, packet_size, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("Enviado PUSH_DATA (%lu bytes), aguardando ACK...\n", packet_size);

    wait_for_ack(tk);

    free(json_str);
    free(buf);
    cJSON_Delete(root);
}

void process_device_file(const char *path) {
    printf("Processando arquivo: %s\n", path);
    FILE *fp = fopen(path, "r");
    if (!fp) {
        perror("Erro ao abrir arquivo");
        return;
    }

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    rewind(fp);

    char *data = malloc(len + 1);
    if (!data) {
        fclose(fp);
        printf("Erro de alocação de memória\n");
        return;
    }

    fread(data, 1, len, fp);
    data[len] = '\0';
    fclose(fp);

    cJSON *root = cJSON_Parse(data);
    free(data);
    if (!root) {
        printf("Erro ao interpretar JSON em: %s\n", path);
        return;
    }

    cJSON *rxpk = cJSON_GetObjectItem(root, "rxpk");
    if (rxpk && cJSON_IsArray(rxpk)) {
        cJSON *rxpk_copy = cJSON_Duplicate(rxpk, 1);
        if (rxpk_copy) {
            send_push_data(rxpk_copy);
        }
    } else {
        printf("JSON em %s não contém um array 'rxpk' válido\n", path);
    }

    cJSON_Delete(root);
}

void load_config(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Erro ao abrir config.json");
        return;
    }

    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    rewind(fp);

    char *data = malloc(len + 1);
    if (!data) {
        fclose(fp);
        return;
    }

    fread(data, 1, len, fp);
    data[len] = '\0';
    fclose(fp);

    cJSON *cfg = cJSON_Parse(data);
    free(data);
    if (!cfg) return;

    cJSON *srv = cJSON_GetObjectItem(cfg, "server");
    cJSON *prt = cJSON_GetObjectItem(cfg, "port");
    if (cJSON_IsString(srv)) strncpy(server_address, srv->valuestring, sizeof(server_address) - 1);
    if (cJSON_IsNumber(prt)) server_port = prt->valueint;

    cJSON_Delete(cfg);
}

int main() {
    printf("Iniciando Envio.exe\n");

    srand(time(NULL));
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

    DIR *d;
    struct dirent *dir;
    // Caminho correto da pasta
    d = opendir("/workspaces/ECOP11A-LoRaWAN/etapa5-integração-TTN/out");
    if (d) {
        regex_t regex;
        regcomp(&regex, "^Saida_Dispositivo_[0-9]+\\.json$", REG_EXTENDED);

        while ((dir = readdir(d)) != NULL) {
            if (regexec(&regex, dir->d_name, 0, NULL, 0) == 0) {
                // Construir o caminho completo do arquivo
                char filepath[512];
                snprintf(filepath, sizeof(filepath), "/workspaces/ECOP11A-LoRaWAN/etapa5-integração-TTN/out/%s", dir->d_name);
                printf("Processando arquivo: %s\n", filepath);
                process_device_file(filepath);
                usleep(500000); // 500ms entre envios
            }
        }
        regfree(&regex);
        closedir(d);
    } else {
        perror("Erro ao abrir diretório out/");
    }

    close(sockfd);
    printf("Envio concluído.\n");
    return 0;
}
