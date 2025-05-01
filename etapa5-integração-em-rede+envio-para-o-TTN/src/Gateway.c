// gateway.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <mbedtls/base64.h>
#include <cjson/cJSON.h>
#include <libwebsockets.h>

#define PORT 1700
#define BUFFER_SIZE 1024
#define BASE64_BUFFER_SIZE ((BUFFER_SIZE + 2) / 3 * 4 + 1)
#define PROTOCOL_VERSION 0x02
#define PUSH_DATA 0x00

typedef struct {
    char gateway_id[64];
    uint8_t gateway_eui[8];
    char server[256];
} DeviceConfig;

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
        perror("Erro ao alocar memória");
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
        printf("Erro: 'devices' não é um array.\n");
        cJSON_Delete(json);
        return 0;
    }

    return 1;
}

int load_device_info(cJSON *device, DeviceConfig *config) {
    const char *gateway_id = cJSON_GetObjectItem(device, "gateway_id")->valuestring;
    const char *gateway_eui_str = cJSON_GetObjectItem(device, "gateway_eui")->valuestring;
    const char *ws_url = cJSON_GetObjectItem(device, "ws_url")->valuestring;

    if (!gateway_id || !gateway_eui_str || !ws_url) {
        printf("Erro: gateway_id, gateway_eui ou ws_url ausente no dispositivo.\n");
        return 0;
    }

    strncpy(config->gateway_id, gateway_id, sizeof(config->gateway_id) - 1);

    for (int i = 0; i < 8; i++) {
        sscanf(&gateway_eui_str[i * 2], "%2hhx", &config->gateway_eui[i]);
    }

    strncpy(config->server, ws_url, sizeof(config->server) - 1);
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

void extract_host_from_url(const char *url, char *host, size_t host_size) {
    const char *start = strstr(url, "://");
    start = start ? start + 3 : url;
    const char *end = strchr(start, ':');
    if (!end) end = strchr(start, '/');
    if (!end) end = start + strlen(start);

    size_t len = end - start;
    if (len >= host_size) len = host_size - 1;
    strncpy(host, start, len);
    host[len] = '\0';
}

static int callback_ttn(struct lws *wsi __attribute__((unused)),
                        enum lws_callback_reasons reason,
                        void *user __attribute__((unused)),
                        void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("[Gateway] Conexão WebSocket estabelecida com o TTN.\n");
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            printf("[Gateway] Mensagem recebida do TTN: %.*s\n", (int)len, (char *)in);
            break;
        case LWS_CALLBACK_CLIENT_WRITEABLE:
            printf("[Gateway] Pronto para enviar dados ao TTN.\n");
            break;
        case LWS_CALLBACK_CLOSED:
            printf("[Gateway] Conexão WebSocket fechada.\n");
            break;
        default:
            break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    { "ttn-protocol", callback_ttn, 0, BUFFER_SIZE, 0, NULL, 0 },
    { NULL, NULL, 0, 0, 0, NULL, 0 }
};

int main() {
    struct lws_context_creation_info info;
    struct lws_client_connect_info ccinfo;
    struct lws_context *context;
    struct lws *wsi;

    cJSON *devices = NULL;
    if (!load_devices("config/Config.json", &devices)) {
        printf("Erro ao carregar dispositivos\n");
        return -1;
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Erro ao criar socket");
        return -1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao fazer bind");
        close(sockfd);
        return -1;
    }

    printf("[Gateway] Aguardando pacotes...\n");

    int device_count = cJSON_GetArraySize(devices);
    for (int i = 0; i < device_count; i++) {
        cJSON *device = cJSON_GetArrayItem(devices, i);
        DeviceConfig config;

        if (!load_device_info(device, &config)) {
            printf("Erro ao carregar info do dispositivo %d.\n", i + 1);
            continue;
        }

        printf("[Gateway] Dispositivo %d: %s\n", i + 1, config.gateway_id);
        printf("  EUI: ");
        for (int j = 0; j < 8; j++) {
            printf("%02X%s", config.gateway_eui[j], (j < 7) ? ":" : "\n");
        }
        printf("  URL: %s\n", config.server);

        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        unsigned char recv_buffer[BUFFER_SIZE];
        ssize_t recv_len;

        recv_len = recvfrom(sockfd, recv_buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (recv_len < 0) {
            perror("[Gateway] Erro ao receber pacote");
            continue;
        }

        send_ack(sockfd, &client_addr, addr_len);

        memset(&info, 0, sizeof(info));
        info.port = CONTEXT_PORT_NO_LISTEN;
        info.protocols = protocols;

        context = lws_create_context(&info);
        if (!context) {
            printf("Erro ao criar contexto WebSocket\n");
            continue;
        }

        char host[256];
        extract_host_from_url(config.server, host, sizeof(host));

        memset(&ccinfo, 0, sizeof(ccinfo));
        ccinfo.context = context;
        ccinfo.address = host;
        ccinfo.port = 8887;
        ccinfo.path = "/";
        ccinfo.host = host;
        ccinfo.origin = "origin";
        ccinfo.protocol = protocols[0].name;
        ccinfo.ssl_connection = LCCSCF_USE_SSL;

        wsi = lws_client_connect_via_info(&ccinfo);
        if (!wsi) {
            printf("Erro ao conectar WebSocket\n");
            lws_context_destroy(context);
            continue;
        }

        printf("[Gateway] Conectando ao TTN...\n");

        while (lws_service(context, 1000) >= 0) {}

        lws_context_destroy(context);
    }

    cJSON_Delete(devices);
    close(sockfd);
    return 0;
}
