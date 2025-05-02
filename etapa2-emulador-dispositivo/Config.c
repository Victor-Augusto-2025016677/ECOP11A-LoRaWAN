#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include "Config.h"

int load_config(const char *filename, Config *configs, int *device_count) {
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

	cJSON *devices = cJSON_GetObjectItemCaseSensitive(json, "devices");
	if (!cJSON_IsArray(devices)) {
		printf("Erro: 'devices' não é um array no Config.json\n");
		cJSON_Delete(json);
		return 0;
	}

	*device_count = cJSON_GetArraySize(devices);
	if (*device_count > MAX_DEVICES) {
		printf("Aviso: Número de dispositivos excede o limite (%d). Apenas os primeiros %d serão carregados.\n", MAX_DEVICES, MAX_DEVICES);
		*device_count = MAX_DEVICES;
	}

	for (int i = 0; i < *device_count; i++) {
		cJSON *device = cJSON_GetArrayItem(devices, i);
		if (!cJSON_IsObject(device)) {
			continue;
		}

		Config *cfg = &configs[i];

		cJSON *deveui = cJSON_GetObjectItemCaseSensitive(device, "deveui");
		if (cJSON_IsString(deveui) && (deveui->valuestring != NULL)) {
			strncpy(cfg->deveui, deveui->valuestring, sizeof(cfg->deveui) - 1);
			cfg->deveui[sizeof(cfg->deveui) - 1] = '\0';
		}

		cJSON *devaddr = cJSON_GetObjectItemCaseSensitive(device, "devaddr");
		if (cJSON_IsString(devaddr) && (devaddr->valuestring != NULL)) {
			cfg->devaddr = strtoul(devaddr->valuestring, NULL, 16);
		}

		cJSON *nwkskey = cJSON_GetObjectItemCaseSensitive(device, "nwkskey");
		if (cJSON_IsString(nwkskey) && (nwkskey->valuestring != NULL)) {
			for (int j = 0; j < 16; j++) {
				sscanf(&nwkskey->valuestring[j * 2], "%2hhx", &cfg->nwkskey[j]);
			}
		}

		cJSON *appskey = cJSON_GetObjectItemCaseSensitive(device, "appskey");
		if (cJSON_IsString(appskey) && (appskey->valuestring != NULL)) {
			for (int j = 0; j < 16; j++) {
				sscanf(&appskey->valuestring[j * 2], "%2hhx", &cfg->appskey[j]);
			}
		}

		cJSON *fcnt = cJSON_GetObjectItemCaseSensitive(device, "fcnt");
		if (cJSON_IsNumber(fcnt)) {
			cfg->fcnt = fcnt->valueint;
		}

		cJSON *fport = cJSON_GetObjectItemCaseSensitive(device, "fport");
		if (cJSON_IsNumber(fport)) {
			cfg->fport = fport->valueint;
		}

		cJSON *payload = cJSON_GetObjectItemCaseSensitive(device, "payload");
		if (cJSON_IsArray(payload)) {
			int payload_len = cJSON_GetArraySize(payload);
			cfg->payload_len = payload_len > MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : payload_len;
			for (int j = 0; j < cfg->payload_len; j++) {
				cJSON *item = cJSON_GetArrayItem(payload, j);
				if (cJSON_IsNumber(item)) {
					cfg->payload[j] = (uint8_t)item->valueint;
				}
			}
		}
	}

	cJSON_Delete(json);
	return 1;
}
