#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <unistd.h>
#include <sys/wait.h>

#define JSON_FILE "config/Config.json"

void executarProcesso(const char *caminho, const char *nome) {
    pid_t pid = fork();

    if (pid == 0) {
        printf("[%s] Iniciando...\n", nome);
        execl(caminho, caminho, NULL);
        perror("Erro ao executar processo");
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("Erro ao criar processo");
        exit(EXIT_FAILURE);
    } else {
        printf("[%s] Executando com PID %d\n", nome, pid);
    }
}

void LimparTela() {
    system("clear");
}

cJSON* loadJSON() {
    FILE *file = fopen(JSON_FILE, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo JSON");
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(fileSize + 1);
    fread(buffer, 1, fileSize, file);
    buffer[fileSize] = '\0';
    fclose(file);

    cJSON *json = cJSON_Parse(buffer);
    free(buffer);

    if (!json) {
        printf("Erro ao parsear o JSON.\n");
    }
    return json;
}

void saveJSON(cJSON *json) {
    FILE *file = fopen(JSON_FILE, "w");
    if (!file) {
        perror("Erro ao abrir o arquivo para salvar o JSON");
        return;
    }

    char *jsonString = cJSON_Print(json);
    fprintf(file, "%s", jsonString);
    fclose(file);
    free(jsonString);
}

void resetDeviceCount(cJSON *json) {
    cJSON *devices = cJSON_GetObjectItem(json, "devices");
    if (devices && cJSON_IsArray(devices)) {
        cJSON *device = NULL;
        cJSON_ArrayForEach(device, devices) {
            cJSON *fcnt = cJSON_GetObjectItem(device, "fcnt");
            if (fcnt) {
                cJSON_SetNumberValue(fcnt, 0);
            }
        }
    } else {
        printf("Erro: 'devices' não é um array.\n");
    }
}

void addDevice(cJSON *json) {
    cJSON *devices = cJSON_GetObjectItem(json, "devices");
    if (!devices || !cJSON_IsArray(devices)) {
        devices = cJSON_AddArrayToObject(json, "devices");
    }

    char device_id[256], application_id[256], deveui[256], devaddr[256];
    char nwkskey[256], appskey[256];
    int fcnt, fport, payload[2];

    printf("Digite o device_id: "); scanf("%s", device_id);
    printf("Digite o application_id: "); scanf("%s", application_id);
    printf("Digite o deveui: "); scanf("%s", deveui);
    printf("Digite o devaddr: "); scanf("%s", devaddr);
    printf("Digite o nwkskey: "); scanf("%s", nwkskey);
    printf("Digite o appskey: "); scanf("%s", appskey);
    printf("Digite o fcnt: "); scanf("%d", &fcnt);
    printf("Digite o fport: "); scanf("%d", &fport);
    printf("Digite o primeiro valor da payload: "); scanf("%d", &payload[0]);
    printf("Digite o segundo valor da payload: "); scanf("%d", &payload[1]);

    cJSON *device = cJSON_CreateObject();
    cJSON_AddStringToObject(device, "device_id", device_id);
    cJSON_AddStringToObject(device, "application_id", application_id);
    cJSON_AddStringToObject(device, "deveui", deveui);
    cJSON_AddStringToObject(device, "devaddr", devaddr);
    cJSON_AddStringToObject(device, "nwkskey", nwkskey);
    cJSON_AddStringToObject(device, "appskey", appskey);
    cJSON_AddNumberToObject(device, "fcnt", fcnt);
    cJSON_AddNumberToObject(device, "fport", fport);

    cJSON *payloadArray = cJSON_AddArrayToObject(device, "payload");
    cJSON_AddItemToArray(payloadArray, cJSON_CreateNumber(payload[0]));
    cJSON_AddItemToArray(payloadArray, cJSON_CreateNumber(payload[1]));

    cJSON_AddItemToArray(devices, device);
}

int JSONMenu(cJSON *json) {
    int choice;

    while (1) {
        printf("\nMenu de Manipulação do JSON:\n");
        printf("1. Zerar a contagem de todos os dispositivos\n");
        printf("2. Limpar o JSON e adicionar um dispositivo próprio\n");
        printf("3. Manter os dispositivos e adicionar outro\n");
        printf("4. Voltar ao menu inicial\n");
        printf("Escolha uma opção: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                resetDeviceCount(json);
                saveJSON(json);
                LimparTela();
                printf("Contagem zerada para todos os dispositivos.\n");
                break;
            case 2:
                cJSON_ReplaceItemInObject(json, "devices", cJSON_CreateArray());
                addDevice(json);
                saveJSON(json);
                LimparTela();
                printf("JSON limpo e novo dispositivo adicionado.\n");
                break;
            case 3:
                addDevice(json);
                saveJSON(json);
                LimparTela();
                printf("Novo dispositivo adicionado.\n");
                break;
            case 4:
                LimparTela();
                return 0;
            default:
                LimparTela();
                printf("Opção inválida, tente novamente.\n");
        }
    }
}

int MenuInicial(cJSON *json) {
    int escolhainicial;
    printf("\nDeseja manipular o arquivo Json? (1 - Sim / 2 - Não)\n");
    scanf("%d", &escolhainicial);

    if (escolhainicial == 1) {
        LimparTela();
        JSONMenu(json);
    } else {
        int escolher;
        LimparTela();
        printf("\nDeseja executar o programa agora? (1 - Sim / 2 - Não)\n");
        scanf("%d", &escolher);

        if (escolher == 1) {
            LimparTela();
            printf("[Controlador] Executando o gateway\n");
            executarProcesso("./bin/Gateway.exe", "Gateway");
            sleep(1);

            printf("[Controlador] Executando o Main\n");
            executarProcesso("./bin/Main.exe", "Main");

            int status;
            while (wait(&status) > 0);

            printf("[Controlador] Execução finalizada.\n");
            return 1;

        } else {
            LimparTela();
            printf("[Controlador] Encerrando...\n");
            return 1;
        }
    }

    return 0;
}

int main() {
    cJSON *json = loadJSON();
    if (!json) return 1;

    while (1) {
        int sair = MenuInicial(json);
        if (sair) break;
    }

    cJSON_Delete(json);
    return 0;
}
