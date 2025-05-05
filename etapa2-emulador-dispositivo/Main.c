#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "lorawan.h"
#include "aes_cmac.h"
#include "Config.h"

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

char pacote_hex[256] = {0};
char temp[4];

int send_to_ttn(const uint8_t *packet, int packet_len);
int save_config(const char *filename, const Config *configs, int device_count);

int save_config(const char *filename, const Config *configs, int device_count) {
	FILE *file = fopen(filename, "r");
	if (!file) {
		perror("Erro ao abrir o arquivo para leitura");
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

	cJSON *devices = cJSON_GetObjectItemCaseSensitive(json, "devices");
	if (!cJSON_IsArray(devices)) {
		printf("Erro: 'devices' não é um array no Config.json\n");
		cJSON_Delete(json);
		return 0;
	}

	for (int i = 0; i < device_count; i++) {
		cJSON *device = cJSON_GetArrayItem(devices, i);
		if (!cJSON_IsObject(device)) {
			continue;
		}

		cJSON_ReplaceItemInObject(device, "fcnt", cJSON_CreateNumber(configs[i].fcnt));
	}

	file = fopen(filename, "w");
	if (!file) {
		perror("Erro ao abrir o arquivo para escrita");
		cJSON_Delete(json);
		return 0;
	}

	char *json_string = cJSON_Print(json);
	if (!json_string) {
		perror("Erro ao converter JSON para string");
		cJSON_Delete(json);
		fclose(file);
		return 0;
	}

	fprintf(file, "%s", json_string);
	fclose(file);

	cJSON_Delete(json);
	free(json_string);

	return 1;
}

int send_to_ttn(const uint8_t *packet, int packet_len) {
	int sockfd;
	struct sockaddr_in server_addr;

	escreverlog("Criando socket UDP...");
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("Erro ao criar socket");
		return -1;
	}
	escreverlog("Socket criado com sucesso.");

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(1700);
	if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
		perror("Erro ao configurar o endereço do servidor");
		close(sockfd);
		return -1;
	}

	escreverlog("Enviando pacote para o servidor local (127.0.0.1:1700)...");
	if (sendto(sockfd, packet, packet_len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("Erro ao enviar pacote");
		close(sockfd);
		return -1;
	}
	escreverlog("Pacote enviado para o servidor local com sucesso!");

	close(sockfd);
	return 0;
}

int main() {
	iniciarlog();
	escreverlog("Simulador Iniciado");

	printf("[LoRaWAN] Simulador de dispositivo iniciado\n");

	Config configs[MAX_DEVICES];
	int device_count = 0;

	escreverlog("Carregando Json");

	if (!load_config("Config.json", configs, &device_count)) {
		printf("Erro ao carregar o arquivo Config.json\n");
		escreverlog("Erro ao carregar o arquivo Config.json");
		return 1;
	}
	
	escreverlog("Configurações carregadas com sucesso. Dispositivos encontrados: %d\n", device_count);

	for (int i = 0; i < device_count; i++) {
		Config *cfg = &configs[i];

		escreverlog("Montando o pacote para o dispositivo %d (DevEUI: %s)...", i + 1, cfg->deveui);

		uint8_t packet[64];
		int packet_len = lorawan_build_uplink(
			packet, cfg->devaddr, cfg->nwkskey, cfg->appskey,
			cfg->fcnt, cfg->fport, cfg->payload, cfg->payload_len
		);

		if (packet_len <= 0) {
			escreverlog("Erro ao montar pacote para o dispositivo %d", i + 1);
			continue;
		}

		escreverlog("Pacote montado para o dispositivo %d: ", i + 1);
		for (int j = 0; j < packet_len; j++) {
			snprintf(temp, sizeof(temp), "%02X", packet[j]);
			strcat(pacote_hex, temp);
		}
		escreverlog("Pacote montado para o dispositivo %d: %s", i + 1, pacote_hex);
			memset(pacote_hex, 0, sizeof(pacote_hex));
			memset(temp, 0, sizeof(temp));

		escreverlog("Enviando pacote para o dispositivo %d...", i + 1);
		if (send_to_ttn(packet, packet_len) != 0) {
			escreverlog("Erro ao enviar pacote para o dispositivo %d", i + 1);
			continue;
		}

		printf("Pacote enviado com sucesso para o dispositivo %d!\n", i + 1);
		escreverlog("Pacote enviado com sucesso para o dispositivo %d!\n", i + 1);
		cfg->fcnt += 1;

		if (!save_config("Config.json", configs, device_count)) {
			escreverlog("Erro ao salvar o arquivo Config.json para o dispositivo %d", i + 1);
		}
	}

	fecharlog();
	printf("[LoRaWAN] Simulador de dispositivo finalizado\n");
	return 0;
}
