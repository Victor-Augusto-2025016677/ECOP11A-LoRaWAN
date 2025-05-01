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

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    unsigned char buffer[BUFFER_SIZE];
    ssize_t recv_len;

    cJSON *devices = NULL;

    // Carrega os dispositivos do Config.json
    if (!load_devices("config/Config.json", &devices)) {
        printf("Erro ao carregar os dispositivos do Config.json\n");
        return 1;
    }

    int device_count = cJSON_GetArraySize(devices);
    printf("[Gateway] Dispositivos encontrados: %d\n", device_count);

    // Cria o socket UDP
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    // Configura o endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // aceita qualquer IP
    server_addr.sin_port = htons(PORT);

    // Associa o socket à porta
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao fazer bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("[Gateway] Aguardando pacotes na porta %d...\n", PORT);

    // Variável para contar pacotes processados
    int processed_count = 0;

    // Loop principal para escutar pacotes
    while (processed_count < device_count) {
        recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                            (struct sockaddr *)&client_addr, &addr_len);

        if (recv_len < 0) {
            perror("Erro ao receber dados");
            continue;
        }

        printf("\n[Gateway] Pacote recebido (%ld bytes) de %s:%d\n",
               recv_len,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        printf("[Gateway] Conteúdo (hex):\n");
        for (ssize_t i = 0; i < recv_len; ++i) {
            printf("%02X ", buffer[i]);
        }
        printf("\n");

        // Converte o pacote para Base64
        unsigned char base64_output[BASE64_BUFFER_SIZE]; // Buffer para saída Base64
        size_t output_len;

        // Codificar o payload em Base64
        int ret = mbedtls_base64_encode(base64_output, sizeof(base64_output), &output_len, buffer, recv_len);
        if (ret != 0) {
            printf("Erro ao codificar o pacote em Base64. Código de erro: %d\n", ret);
            continue;
        }

        // Adicionar o caractere nulo ao final da string Base64
        base64_output[output_len] = '\0';

        // Exibir o resultado da codificação para depuração
        printf("[Gateway] Payload codificado em Base64: %s\n", base64_output);

        // Processa cada dispositivo
        for (int i = 0; i < device_count; i++) {
            cJSON *device = cJSON_GetArrayItem(devices, i);
            if (!cJSON_IsObject(device)) {
                continue;
            }

            // Extrai informações do dispositivo
            const cJSON *device_id = cJSON_GetObjectItemCaseSensitive(device, "device_id");
            const cJSON *application_id = cJSON_GetObjectItemCaseSensitive(device, "application_id");
            const cJSON *fport = cJSON_GetObjectItemCaseSensitive(device, "fport");

            if (!cJSON_IsString(device_id) || !cJSON_IsString(application_id) || !cJSON_IsNumber(fport)) {
                printf("Erro: Dispositivo %d com dados inválidos no Config.json\n", i + 1);
                continue;
            }

            // Cria o JSON de saída
            cJSON *root = cJSON_CreateObject();
            cJSON *end_device_ids = cJSON_CreateObject();
            cJSON_AddStringToObject(end_device_ids, "device_id", device_id->valuestring);

            cJSON *application_ids = cJSON_CreateObject();
            cJSON_AddStringToObject(application_ids, "application_id", application_id->valuestring);
            cJSON_AddItemToObject(end_device_ids, "application_ids", application_ids);

            cJSON_AddItemToObject(root, "end_device_ids", end_device_ids);

            cJSON *uplink_message = cJSON_CreateObject();
            cJSON_AddNumberToObject(uplink_message, "f_port", fport->valueint);
            cJSON_AddStringToObject(uplink_message, "frm_payload", (char *)base64_output);
            cJSON_AddItemToObject(root, "uplink_message", uplink_message);

            // Converte o JSON para string
            char *json_string = cJSON_Print(root);
            if (json_string == NULL) {
                printf("Erro ao converter JSON para string\n");
            } else {
                printf("[Gateway] JSON gerado para o dispositivo %d:\n%s\n", i + 1, json_string);

                // Salva o JSON em um arquivo
                char filename[64];
                snprintf(filename, sizeof(filename), "json_out/uplink_device_%d.json", i + 1);
                FILE *file = fopen(filename, "w");
                if (file == NULL) {
                    perror("Erro ao abrir o arquivo para salvar o JSON");
                } else {
                    fprintf(file, "%s\n", json_string);
                    fclose(file);
                    printf("[Gateway] JSON salvo no arquivo '%s'\n", filename);
                }

                free(json_string);
            }

            cJSON_Delete(root);
        }

        // Enviar confirmação de recebimento (ACK)
        const char *ack_message = "ACK";
        if (sendto(sockfd, ack_message, strlen(ack_message), 0,
                   (struct sockaddr *)&client_addr, addr_len) < 0) {
            perror("Erro ao enviar ACK");
        } else {
            printf("[Gateway] ACK enviado para o dispositivo.\n");
        }

        // Incrementa o contador de pacotes processados
        processed_count++;
        printf("[Gateway] Pacotes processados: %d/%d\n", processed_count, device_count);
    }

    printf("[Gateway] Todos os pacotes foram processados. Encerrando...\n");

    cJSON_Delete(devices);
    close(sockfd);
    return 0;
}
