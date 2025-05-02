#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>

int send_data_to_tagoio(const char *device_token, float altura, float temperatura) {
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
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

		res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			fprintf(stderr, "Erro ao enviar dados: %s\n", curl_easy_strerror(res));
			curl_slist_free_all(headers);
			curl_easy_cleanup(curl);
			curl_global_cleanup();
			return 1;
		}

		printf("Dados enviados com sucesso!\n");

		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
	}

	curl_global_cleanup();
	return 0;
}

int main() {
	const char *device_token = "5e01c2ac-e772-42e2-8ff8-214a2151b903";
	float altura = 150.75;
	float temperatura = 23.5;

	if (send_data_to_tagoio(device_token, altura, temperatura) != 0) {
		fprintf(stderr, "Falha ao enviar dados.\n");
		return 1;
	}

	return 0;
}
