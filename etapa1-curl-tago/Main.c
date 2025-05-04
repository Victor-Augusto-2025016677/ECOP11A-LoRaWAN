#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <curl/curl.h>

FILE *log_file = NULL;
char nomedolog[64];

//Incio bloco de log
void gerar_nome_log(char *buffer, size_t tamanho) { //criação de nome com base no tempo
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
void escreverlog(const char *mensagem) {
    if (log_file != NULL) {
        fprintf(log_file, "[%s] %s\n", current_time_str(), mensagem);
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

//Fim bloco de log

int send_data_to_tagoio(const char *device_token, float altura, float temperatura) {
	escreverlog("Iniciando a função send_data");
	CURL *curl;
	CURLcode res;

	const char *url = "https://api.us-e1.tago.io/data";
	char data[1024];

	snprintf(data, sizeof(data),
		"["
		"	{"
		"		\"variable\": \"altura\","
		"		\"value\": %f,"
		"		\"unit\": \"m\""
		"	},"
		"	{"
		"		\"variable\": \"temperatura\","
		"		\"value\": %f,"
		"		\"unit\": \"C\""
		"	}"
		"]", altura, temperatura);

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	char auth_header[256];
	snprintf(auth_header, sizeof(auth_header), "device-token: %s", device_token);
	headers = curl_slist_append(headers, auth_header);

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();

	if (curl) {
		escreverlog("Iniciando a função curl");
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			fprintf(stderr, "Erro ao enviar dados: %s\n", curl_easy_strerror(res));
			escreverlog("Erro ao enviar dados");
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
			curl_global_cleanup();
			return 1;
		}

		escreverlog("Curl finalizado com sucesso");

		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
	}

	curl_global_cleanup();
	return 0;
}

int main() {
	iniciarlog();
    escreverlog("Iniciando o Main");
	const char *device_token = "5e01c2ac-e772-42e2-8ff8-214a2151b903";
	float altura = 150.75;
	float temperatura = 23.5;

	if (send_data_to_tagoio(device_token, altura, temperatura) != 0) {
		escreverlog("Falha ao enviar dados");
		return 1;
	}

	escreverlog("Finalizando o programa");
    fecharlog();

	return 0;
}
