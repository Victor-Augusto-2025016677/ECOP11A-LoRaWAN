#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <cjson/cJSON.h>

static void hexstr_to_bytes(const char *hex, uint8_t *out, size_t len) {
    for (size_t i = 0; i < len; i++) {
        sscanf(&hex[i * 2], "%2hhx", &out[i]);
    }
}

int load_config(const char *filename, Config *config) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("Erro ao abrir o arquivo JSON");
        return 0;
    }

    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *data = malloc(length + 1);
    if (!data) {
        fclose(f);
        return 0;
    }
    fread(data, 1, length, f);
    data[length] = '\0';
    fclose(f);

    cJSON *json = cJSON_Parse(data);
    free(data);
    if (!json) {
        printf("Erro ao fazer parse do JSON\n");
        return 0;
    }

    const cJSON *devaddr = cJSON_GetObjectItem(json, "devaddr");
    const cJSON *nwkskey = cJSON_GetObjectItem(json, "nwkskey");
    const cJSON *appskey = cJSON_GetObjectItem(json, "appskey");
    const cJSON *fcnt = cJSON_GetObjectItem(json, "fcnt");
    const cJSON *fport = cJSON_GetObjectItem(json, "fport");
    const cJSON *payload = cJSON_GetObjectItem(json, "payload");

    if (!cJSON_IsString(devaddr) || !cJSON_IsString(nwkskey) || !cJSON_IsString(appskey)
        || !cJSON_IsNumber(fcnt) || !cJSON_IsNumber(fport) || !cJSON_IsArray(payload)) {
        cJSON_Delete(json);
        printf("JSON mal formatado\n");
        return 0;
    }

    sscanf(devaddr->valuestring, "%x", &config->devaddr);
    hexstr_to_bytes(nwkskey->valuestring, config->nwkskey, 16);
    hexstr_to_bytes(appskey->valuestring, config->appskey, 16);
    config->fcnt = (uint16_t)fcnt->valueint;
    config->fport = (uint8_t)fport->valueint;

    int len = cJSON_GetArraySize(payload);
    if (len > MAX_PAYLOAD_SIZE) len = MAX_PAYLOAD_SIZE;
    config->payload_len = len;
    for (int i = 0; i < len; i++) {
        cJSON *item = cJSON_GetArrayItem(payload, i);
        config->payload[i] = (uint8_t)(cJSON_IsNumber(item) ? item->valueint : 0);
    }

    cJSON_Delete(json);
    return 1;
}
