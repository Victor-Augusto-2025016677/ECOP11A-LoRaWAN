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















int main() {
    double a, b, resultado;
    char operacao;

    iniciarlog();
    escreverlog("Iniciando o programa");

    printf("Digite o primeiro número: ");
    scanf("%lf", &a);

    escreverlog("Primeiro número lido");

    printf("Digite a operação (+, -, *, /): ");
    scanf(" %c", &operacao); // espaço ignora enter anterior

    escreverlog("operação número lido");

    printf("Digite o segundo número: ");
    scanf("%lf", &b);

    escreverlog("Segundo número lido");

    switch (operacao) {
        case '+':
            resultado = a + b;
            break;
        case '-':
            resultado = a - b;
            break;
        case '*':
            resultado = a * b;
            break;
        case '/':
            if (b == 0) {
                printf("Erro: divisão por zero!\n");
                return 1;
            }
            resultado = a / b;
            break;
        default:
            printf("Operação inválida!\n");
            return 1;
    }

    printf("Resultado: %.2f\n", resultado);

    escreverlog("Resultado calculado");
    escreverlog("Finalizando o programa");
    fecharlog(); // Fecha o log
    printf("Log salvo em: %s\n", nomedolog);
    return 0;
}