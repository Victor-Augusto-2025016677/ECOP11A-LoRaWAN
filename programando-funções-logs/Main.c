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

