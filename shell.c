/*
Implementação shell em C

811773 - Jakson Huang Zheng
811716 - Gustavo Lamin Honda

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

int verificaSaida(char command[1024]) {

    return !(strcmp(command, "exit"));
}

void separarTokens(char *input, char **args, int *qtdTokens) {
    char *token = strtok(input, " ");
    int i = 0;

    while (token != NULL)
    {
        args[i] = token;
        i++;

        token = strtok(NULL, " ");
    }

    args[i] = NULL;
    *qtdTokens = i;
}

int verificaArquivoSaida(char **args) {
    int i;
    for (i = 0; args[i] != NULL; i++)
    {

        if (strcmp(args[i], ">") == 0)
        {
            args[i] = NULL;
            return i + 1;
        }
    }
    return 0;
}

int main() {
    char command[1024];
    char *args[64];
    int qtdTokens;
    int indiceSaida;

    // Configura o tratador de sinal SIGCHLD para SIG_IGN(ignora o sinal), tratando assim os zombies
    signal(SIGCHLD, SIG_IGN);

    while (1) {
        printf("$ ");
        fgets(command, 1024, stdin);

        // retira o \n da linha de comando lida do teclado
        command[strcspn(command, "\n")] = '\0';
        
        // verifica se é "exit"
        if (verificaSaida(command))
            break;

        // Verifica se tem &
        int background = (strchr(command, '&') != NULL);

        // Tira o & do final se background == 1
        if (background) {
            command[strlen(command) - 1] = '\0';
        }

        // separa o comando em partes
        separarTokens(command, args, &qtdTokens);

        // vê onde está o nome do redirecionamento da saída na linha de comando 
        indiceSaida = verificaArquivoSaida(args);

        // gera o processo filho
        pid_t son = fork();

        if (son == 0) {
            // processo filho
            // Verifica se tem arquivo de saida
            if (indiceSaida > 0) {

                // Abre o arquivo de saída
                int saida = open(args[indiceSaida], O_WRONLY | O_CREAT | O_TRUNC, 0644);

                if (saida < 0) {
                    perror("Erro ao abrir o arquivo de saída");
                    exit(EXIT_FAILURE);
                }

                // Redireciona a saída padrão para o arquivo
                if (dup2(saida, STDOUT_FILENO) < 0) {
                    perror("Erro ao redirecionar a saída padrão");
                    exit(EXIT_FAILURE);
                }
            }
            
            // Verifica se o comando é o cd
            if (!(strcmp(args[0], "cd"))) {
                if (args[1] == NULL) { 
                    // Verifica se o cd vem sem o parâmetro
                    printf("Informe o diretório\n");
                } else {
                    // Muda o diretório corrente para o indicado pelo cd 
                    if (chdir(args[1])) {
                        perror("Erro ao mudar de diretório");
                    }
                }
            } else {
                // Executa a instrução
                execvp(args[0], args);

                // o que está a baixo não é executado a menos que dê um erro no execvp()
                perror("Erro ao executar o comando");
                exit(EXIT_FAILURE);
            }
        } else if (son < 0) {
            // erro no fork
            perror("Erro ao criar processo filho");
        } else {
            // processo pai
            int status;
            if (!background)
                // suspende o processo até que o sistema capte o status do filho que tem o pid == son
                wait(NULL);
            if (!strcmp(args[0], "cd")) 
                // ao encerrar o processo filho o procesos pai também é encerrado, por ser um "cd"
                exit(1);
        }
    }

    return 0;
}