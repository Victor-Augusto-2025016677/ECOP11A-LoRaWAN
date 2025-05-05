#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <mbedtls/base64.h>
#include <cjson/cJSON.h>

#define PORT 1700
#define BUFFER_SIZE 1024

// Início bloco de log

#include <time.h>
#include <stdarg.h>


FILE *log_file = NULL;
char nomedolog[64];

void gerar_nome_log(char *buffer, size_t tamanho) {
    time_t agora = time(NULL);
    struct tm *tm_info = localtime(&agora);
    strftime(buffer, tamanho, "logs/log_%Y%m%d_%H%M%S.txt", tm_info);
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

/*
Comandos de execução:

    iniciarlog();

    escreverlog("texto aqui"); // as timestamp são adicionadas automaticamente na função

    fecharlog();

    escreverlog("texto aqui: %d operador", variavel);

*/

// Fim bloco de log

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
		escreverlog("Erro ao analisar o JSON: %s", cJSON_GetErrorPtr());
		return 0;
	}

	cJSON *device_id_json = cJSON_GetObjectItemCaseSensitive(json, "device_id");
	if (cJSON_IsString(device_id_json) && (device_id_json->valuestring != NULL)) {
		strncpy(device_id, device_id_json->valuestring, 32);
		device_id[32] = '\0';
	} else {
		escreverlog("Erro: Campo 'device_id' ausente ou inválido no Config.json");
		printf("Erro: Campo 'device_id' ausente ou inválido no Config.json\n");
		cJSON_Delete(json);
		return 0;
	}

	cJSON *application_id_json = cJSON_GetObjectItemCaseSensitive(json, "application_id");
	if (cJSON_IsString(application_id_json) && (application_id_json->valuestring != NULL)) {
		strncpy(application_id, application_id_json->valuestring, 32);
		application_id[32] = '\0';
	} else {
		escreverlog("Erro: Campo 'application_id' ausente ou inválido no Config.json");
		printf("Erro: Campo 'application_id' ausente ou inválido no Config.json\n");
		cJSON_Delete(json);
		return 0;
	}

	cJSON_Delete(json);
	return 1;
}

int main() {

	iniciarlog();

	int sockfd;
	struct sockaddr_in server_addr, client_addr;
	socklen_t addr_len = sizeof(client_addr);
	unsigned char buffer[BUFFER_SIZE];
	ssize_t recv_len;

	char device_id[33] = {0};
	char application_id[33] = {0};

	escreverlog("[Gateway] Carregando json");
	printf("[Gateway] Carregando json");

	if (!load_config("Config.json", device_id, application_id)) {
		escreverlog("Erro ao carregar o arquivo Config.json");
		return 1;
	}

	escreverlog("[Gateway] DEVICE_ID: %s", device_id);
	escreverlog("[Gateway] APPLICATION_ID: %s", application_id);
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

	escreverlog("[Gateway] Aguardando pacotes na porta %d...", PORT);
	printf("[Gateway] Aguardando pacotes na porta %d...\n", PORT);

	while (1) {
		recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
							(struct sockaddr *)&client_addr, &addr_len);

		if (recv_len < 0) {
			perror("Erro ao receber dados");
			continue;
		}

		escreverlog("\n[Gateway] Pacote recebido (%ld bytes) de %s:%d\n",
			recv_len,
			inet_ntoa(client_addr.sin_addr),
			ntohs(client_addr.sin_port));

		printf("\n[Gateway] Pacote recebido (%ld bytes) de %s:%d\n",
			recv_len,
			inet_ntoa(client_addr.sin_addr),
			ntohs(client_addr.sin_port));

		escreverlog("[Gateway] Conteúdo (hex):\n");
		printf("[Gateway] Conteúdo (hex):\n");
		for (ssize_t i = 0; i < recv_len; ++i) {
			printf("%02X ", buffer[i]);
		}
		

		unsigned char base64_output[BUFFER_SIZE * 2];
		size_t output_len;
		int ret = mbedtls_base64_encode(base64_output, sizeof(base64_output), &output_len, buffer, recv_len);
		if (ret != 0) {
			escreverlog("Erro ao codificar o pacote em Base64");
			printf("Erro ao codificar o pacote em Base64");
		} else {
			escreverlog("[Gateway] Pacote em Base64:%s", base64_output);
			printf("[Gateway] Pacote em Base64:\n%s", base64_output);

			cJSON *root = cJSON_CreateObject();
			if (root == NULL) {
				escreverlog("Erro ao criar o objeto JSON");
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
				escreverlog("Erro ao converter JSON para string");
				printf("Erro ao converter JSON para string\n");
			} else {
				escreverlog("[Gateway] JSON gerado:\n%s", json_string);
				printf("[Gateway] JSON gerado:\n%s", json_string);

				FILE *file = fopen("uplink.json", "w");
				if (file == NULL) {
					perror("Erro ao abrir o arquivo para salvar o JSON");
				} else {
					fprintf(file, "%s\n", json_string);
					fclose(file);
					printf("[Gateway] JSON salvo no arquivo 'uplink.json'\n");
					escreverlog("[Gateway] JSON salvo no arquivo 'uplink.json'");
				}

				free(json_string);
			}

			cJSON_Delete(root);
		}

		printf("[Gateway] Encerrando após receber o pacote.\n");
		escreverlog("[Gateway] Encerrando após receber o pacote.");
		break;
	}

	close(sockfd);

	fecharlog();
	
	return 0;
}
