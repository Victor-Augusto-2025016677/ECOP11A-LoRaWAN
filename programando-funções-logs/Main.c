//Iniciando escrita da função de criação e manipulação de logs


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

FILE *log_file = NULL;
char nomedolog[64];

void gerar_nome_log(char *buffer, size_t tamanho) { //criação de nome com base no tempo
    time_t agora = time(NULL);
    struct tm *tm_info = localtime(&agora);
    strftime(buffer, tamanho, "log_%Y%m%d_%H%M%S.txt", tm_info);
}



//função para adicionar tempo na escrita do log
const char* current_time_str() {
    static char buffer[32];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", t);
    return buffer;
}

void init_log() {
    gerar_nome_log(nomedolog, sizeof(nomedolog));
    log_file = fopen(nomedolog, "a");
    if (log_file == NULL) {
        perror("Erro ao abrir o arquivo de log");
        exit(EXIT_FAILURE);
    }
    fprintf(log_file, "Inicio\n", current_time_str());
    fflush(log_file);
}

void escreverlog(const char *mensagem) {
    if (log_file != NULL) {
        fprintf(log_file, "[%s] %s\n", current_time_str(), mensagem);
        fflush(log_file);
    }
}

void close_log() {
    if (log_file != NULL) {
        escreverlog("Final-Log");
        fclose(log_file);
        log_file = NULL;
    }
}

