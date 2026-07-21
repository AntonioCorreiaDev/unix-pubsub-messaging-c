#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <pthread.h>

#define FIFO_MANAGER "pipe" 
#define FIFO_FEED "f_%d"
#define TAM 300
#define MAX_TOPICOS 20
#define MAX_MENSAGENS 5
#define MAX_USERS 10
#define TAM_USERNAME 20 

typedef struct{
        char nome[TAM_USERNAME];
        char str[TAM];
        char topico[TAM_USERNAME];
        int tempo_vida;
        pid_t pid;
        int flagVerificado;
}PEDIDO;

typedef struct{
        char nome[TAM_USERNAME];
        char topico[TAM_USERNAME];
        int tempo;
        char str[TAM];
        int flag;
}RESPOSTA;
