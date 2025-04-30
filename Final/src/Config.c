#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <cjson/cJSON.h>
#include "Config.h"

int load_config(const char *filename, Config *cfg) {
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

    // Carrega o DEVEUI
    cJSON *deveui = cJSON_GetObjectItemCaseSensitive(json, "deveui");
    if (cJSON_IsString(deveui) && (deveui->valuestring != NULL)) {
        strncpy(cfg->deveui, deveui->valuestring, sizeof(cfg->deveui) - 1);
        cfg->deveui[sizeof(cfg->deveui) - 1] = '\0'; // Garante terminação
    } else {
        printf("Erro: Campo 'deveui' ausente ou inválido no Config.json\n");
        cJSON_Delete(json);
        return 0;
    }

    // Carrega os outros campos (exemplo: devaddr, nwkskey, appskey, etc.)
    cJSON *devaddr = cJSON_GetObjectItemCaseSensitive(json, "devaddr");
    if (cJSON_IsString(devaddr) && (devaddr->valuestring != NULL)) {
        cfg->devaddr = strtoul(devaddr->valuestring, NULL, 16);
    }

    cJSON *nwkskey = cJSON_GetObjectItemCaseSensitive(json, "nwkskey");
    if (cJSON_IsString(nwkskey) && (nwkskey->valuestring != NULL)) {
        for (int i = 0; i < 16; i++) {
            sscanf(&nwkskey->valuestring[i * 2], "%2hhx", &cfg->nwkskey[i]);
        }
    }

    cJSON *appskey = cJSON_GetObjectItemCaseSensitive(json, "appskey");
    if (cJSON_IsString(appskey) && (appskey->valuestring != NULL)) {
        for (int i = 0; i < 16; i++) {
            sscanf(&appskey->valuestring[i * 2], "%2hhx", &cfg->appskey[i]);
        }
    }

    cJSON *fcnt = cJSON_GetObjectItemCaseSensitive(json, "fcnt");
    if (cJSON_IsNumber(fcnt)) {
        cfg->fcnt = fcnt->valueint;
    }

    cJSON *fport = cJSON_GetObjectItemCaseSensitive(json, "fport");
    if (cJSON_IsNumber(fport)) {
        cfg->fport = fport->valueint;
    }

    cJSON *payload = cJSON_GetObjectItemCaseSensitive(json, "payload");
    if (cJSON_IsArray(payload)) {
        int payload_len = cJSON_GetArraySize(payload);
        cfg->payload_len = payload_len > 64 ? 64 : payload_len;
        for (int i = 0; i < cfg->payload_len; i++) {
            cJSON *item = cJSON_GetArrayItem(payload, i);
            if (cJSON_IsNumber(item)) {
                cfg->payload[i] = (uint8_t)item->valueint;
            }
        }
    }

    cJSON_Delete(json);
    return 1;
}
