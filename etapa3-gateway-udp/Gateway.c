#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <mbedtls/base64.h>
#include <cjson/cJSON.h>

#define PORT 1700
#define BUFFER_SIZE 1024

int load_config(const char *filename, char *device_id, char *application_id) {
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

	cJSON *device_id_json = cJSON_GetObjectItemCaseSensitive(json, "device_id");
	if (cJSON_IsString(device_id_json) && (device_id_json->valuestring != NULL)) {
		strncpy(device_id, device_id_json->valuestring, 32);
		device_id[32] = '\0';
	} else {
		printf("Erro: Campo 'device_id' ausente ou inválido no Config.json\n");
		cJSON_Delete(json);
		return 0;
	}

	cJSON *application_id_json = cJSON_GetObjectItemCaseSensitive(json, "application_id");
	if (cJSON_IsString(application_id_json) && (application_id_json->valuestring != NULL)) {
		strncpy(application_id, application_id_json->valuestring, 32);
		application_id[32] = '\0';
	} else {
		printf("Erro: Campo 'application_id' ausente ou inválido no Config.json\n");
		cJSON_Delete(json);
		return 0;
	}

	cJSON_Delete(json);
	return 1;
}

int main() {
	int sockfd;
	struct sockaddr_in server_addr, client_addr;
	socklen_t addr_len = sizeof(client_addr);
	unsigned char buffer[BUFFER_SIZE];
	ssize_t recv_len;

	char device_id[33] = {0};
	char application_id[33] = {0};

	if (!load_config("Config.json", device_id, application_id)) {
		printf("Erro ao carregar o arquivo Config.json\n");
		return 1;
	}

	printf("[Gateway] DEVICE_ID: %s\n", device_id);
	printf("[Gateway] APPLICATION_ID: %s\n", application_id);

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

	while (1) {
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

		unsigned char base64_output[BUFFER_SIZE * 2];
		size_t output_len;
		int ret = mbedtls_base64_encode(base64_output, sizeof(base64_output), &output_len, buffer, recv_len);
		if (ret != 0) {
			printf("Erro ao codificar o pacote em Base64\n");
		} else {
			printf("[Gateway] Pacote em Base64:\n%s\n", base64_output);

			cJSON *root = cJSON_CreateObject();
			if (root == NULL) {
				printf("Erro ao criar o objeto JSON\n");
				continue;
			}

			cJSON *end_device_ids = cJSON_CreateObject();
			cJSON_AddStringToObject(end_device_ids, "device_id", device_id);

			cJSON *application_ids = cJSON_CreateObject();
			cJSON_AddStringToObject(application_ids, "application_id", application_id);
			cJSON_AddItemToObject(end_device_ids, "application_ids", application_ids);

			cJSON_AddItemToObject(root, "end_device_ids", end_device_ids);

			cJSON *uplink_message = cJSON_CreateObject();
			cJSON_AddNumberToObject(uplink_message, "f_port", 1);
			cJSON_AddStringToObject(uplink_message, "frm_payload", (char *)base64_output);
			cJSON_AddItemToObject(root, "uplink_message", uplink_message);

			char *json_string = cJSON_Print(root);
			if (json_string == NULL) {
				printf("Erro ao converter JSON para string\n");
			} else {
				printf("[Gateway] JSON gerado:\n%s\n", json_string);

				FILE *file = fopen("uplink.json", "w");
				if (file == NULL) {
					perror("Erro ao abrir o arquivo para salvar o JSON");
				} else {
					fprintf(file, "%s\n", json_string);
					fclose(file);
					printf("[Gateway] JSON salvo no arquivo 'uplink.json'\n");
				}

				free(json_string);
			}

			cJSON_Delete(root);
		}

		printf("[Gateway] Encerrando após receber o pacote.\n");
		break;
	}

	close(sockfd);
	return 0;
}
