#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define JSON_FILE "config/Config.json"

#include <time.h>
#include <stdarg.h>

FILE *log_file = NULL;
char nomedolog[64];

void gerar_nome_log(char *buffer, size_t tamanho) {
    time_t agora = time(NULL);
    struct tm *tm_info = localtime(&agora);
    strftime(buffer, tamanho, "logs/log_Controlador_%Y%m%d_%H%M%S.txt", tm_info);
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

// Fim bloco de log

void executarProcesso(const char *caminho, const char *nome) {
    pid_t pid = fork();

    if (pid == 0) {
        printf("[Controlador] [%s] Iniciando...\n", nome);
        escreverlog("[Controlador] [%s] Iniciando...", nome);
        execl(caminho, caminho, NULL);
        perror("[Controlador] Erro ao executar processo");
        escreverlog("[Controlador] Erro ao executar processo: %s", strerror(errno));
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("[Controlador] Erro ao criar processo");
        escreverlog("[Controlador] Erro ao criar processo: %s", strerror(errno));
        exit(EXIT_FAILURE);
    } else {
        printf("[Controlador] [%s] Executando com PID %d\n", nome, pid);
        escreverlog("[Controlador] [%s] Executando com PID %d", nome, pid);
    }
}

void LimparTela() {
    system("clear");
}

cJSON* loadJSON() {
    FILE *file = fopen(JSON_FILE, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo JSON");
        escreverlog("Erro ao abrir o arquivo JSON: %s", strerror(errno));
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
        escreverlog("Erro ao parsear o JSON.");
    }
    return json;
}

void saveJSON(cJSON *json) {
    FILE *file = fopen(JSON_FILE, "w");
    if (!file) {
        perror("Erro ao abrir o arquivo para salvar o JSON");
        escreverlog("Erro ao abrir o arquivo para salvar o JSON: %s", strerror(errno));
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
        escreverlog("Erro: 'devices' não é um array.");
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

    char gateway_id[256], datr[64], codr[64];
    float freq, lsnr;
    int chan, rfch, rssi;

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

    printf("Digite o gateway_id: "); scanf("%s", gateway_id);
    printf("Digite a frequência (ex: 868.3): "); scanf("%f", &freq);
    printf("Digite o canal (chan): "); scanf("%d", &chan);
    printf("Digite o canal RF (rfch): "); scanf("%d", &rfch);
    printf("Digite o RSSI: "); scanf("%d", &rssi);
    printf("Digite o SNR (lsnr): "); scanf("%f", &lsnr);
    printf("Digite o datr (ex: SF7BW125): "); scanf("%s", datr);
    printf("Digite o codr (ex: 4/5): "); scanf("%s", codr);

    escreverlog("Novo dispositivo adicionado: %s", device_id);

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

    cJSON *gateway = cJSON_CreateObject();
    cJSON_AddStringToObject(gateway, "id", gateway_id);
    cJSON_AddNumberToObject(gateway, "freq", freq);
    cJSON_AddNumberToObject(gateway, "chan", chan);
    cJSON_AddNumberToObject(gateway, "rfch", rfch);
    cJSON_AddNumberToObject(gateway, "rssi", rssi);
    cJSON_AddNumberToObject(gateway, "lsnr", lsnr);
    cJSON_AddStringToObject(gateway, "datr", datr);
    cJSON_AddStringToObject(gateway, "codr", codr);

    cJSON_AddItemToObject(device, "gateway", gateway);
    cJSON_AddItemToArray(devices, device);
}


int JSONMenu(cJSON *json) {
    int choice;

    while (1) {
        escreverlog("[Controlador] MENU JSON Aberto");
        printf("\nMenu de Manipulação do JSON:\n");
        printf("1. Zerar a contagem de todos os dispositivos\n");
        printf("2. Limpar o JSON e adicionar um dispositivo próprio\n");
        printf("3. Manter os dispositivos e adicionar outro\n");
        printf("4. Voltar ao menu inicial\n");
        printf("Escolha uma opção: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                escreverlog("[Controlador] Optou por zerar o fcnt do json");
                resetDeviceCount(json);
                saveJSON(json);
                LimparTela();
                printf("Contagem zerada para todos os dispositivos.\n");
                escreverlog("[Controlador] Contagem zerada para todos os dispositivos.");
                break;
            case 2:
                escreverlog("[Controlador] Optou por limpar e adicionar um novo dispositivo");
                cJSON_ReplaceItemInObject(json, "devices", cJSON_CreateArray());
                addDevice(json);
                saveJSON(json);
                LimparTela();
                printf("JSON limpo e novo dispositivo adicionado.\n");
                escreverlog("JSON limpo e novo dispositivo adicionado.");
                break;
            case 3:
                escreverlog("[Controlador] Optou por adicionar um novo dispositivo sem apagar o json");
                addDevice(json);
                saveJSON(json);
                LimparTela();
                printf("Novo dispositivo adicionado.\n");
                escreverlog("Novo dispositivo adicionado.");
                break;
            case 4:
                escreverlog("[Controlador] Optou por voltar ao menu inicial");
                LimparTela();
                return 0;
            default:
                LimparTela();
                printf("Opção inválida, tente novamente.\n");
                escreverlog("Opção inválida no menu do JSON.");
        }
    }
}

int MenuInicial(cJSON *json) {
    int escolhainicial;
    printf("\nMenu Inicial:\n");
    printf("1. Manipulação do Json\n");
    printf("2. Executar o código\n>");
    escreverlog("[Controlador] Opções de escolha impressas");
    scanf("%d", &escolhainicial);
    escreverlog("[Controlador] Escolha lida");
    escreverlog("[Controlador] Escolha %d", escolhainicial);

    switch (escolhainicial)
    {
    case 1:
        escreverlog("[Controlador] Optou por alterar o Json");
        LimparTela();
        JSONMenu(json);
        break;

    case 2:

        escreverlog("[Controlador] Optou por não alterar o json");
        int escolher;
        LimparTela();
        printf("\nDeseja executar o programa agora? (1 - Sim / 2 - Não, encerrar código. )\n");
        scanf("%d", &escolher);

        if (escolher == 1) {
        LimparTela();
        printf("[Controlador] Executando o gateway\n");
        escreverlog("[Controlador] Executando o gateway");
        executarProcesso("./bin/Gateway.exe", "Gateway");
        sleep(1);

        printf("[Controlador] Executando o Main\n");
        escreverlog("[Controlador] Executando o Main");
        executarProcesso("./bin/Main.exe", "Main");

        sleep(1);
        executarProcesso("./bin/Envio.exe", "Envio");
        int status;
        while (wait(&status) > 0);

        printf("[Controlador] Execução finalizada.\n");
        escreverlog("[Controlador] Execução finalizada.");
        return 1;

        } 
        else {
        LimparTela();
        printf("[Controlador] Encerrando...\n");
        escreverlog("[Controlador] Encerrando...");
        return 1;
        }
        
    default:
        LimparTela();
        printf("Opção inválida, tente novamente.\n");
    }

    return 0;
}

int main() {
    iniciarlog();

    escreverlog("[Controlador] Código iniciado");
    cJSON *json = loadJSON();
    if (!json) return 1;

    while (1) {
        escreverlog("[Controlador] Menu Inicial aberto");
        int sair = MenuInicial(json);
        if (sair) break;
    }

    cJSON_Delete(json);
    fecharlog();
    return 0;
}
