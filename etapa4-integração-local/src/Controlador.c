#include <stdio.h>
#include <stdlib.h>

void LimparTela() 
    {
        system("clear");
    }

void MenuInicial ()
    {
        int escolhainicial;

        printf("Deseja manipular o arquivo Json? (Y - Sim / N - Não)");

        scanf("%d", &escolhainicial);

        if (escolhainicial == 1) 
            {
                LimparTela();
                printf("Opções de manipulação");
                return 0;
            }

        else 
            {
                LimparTela();
                printf("Opções de execução");
                return 0;

            }
    }











































int main()
{
    MenuInicial();
}