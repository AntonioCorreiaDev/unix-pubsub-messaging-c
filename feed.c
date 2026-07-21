#include "util.h"

int fd = -1;          // Descritor do FIFO do servidor
int fd_feed = -1;     // Descritor do FIFO do cliente
char fifo_feed[20];   // Nome do FIFO do cliente
int fifo_created = 0; // Flag para saber se o FIFO foi criado


int escreve_mensagem_manager(PEDIDO p){
    int res;
    res = write(fd, &p, sizeof(PEDIDO));
    if (res == sizeof(PEDIDO)) {
        if(strlen(p.topico) > 0 && p.tempo_vida > 0){
            printf("MENSAGEM PERSISTENTE ENVIADA: TOPICO:%s TEMPO:%d -> '%s' (%d)\n", p.topico, p.tempo_vida, p.str, res);
            return res;
        }else if(strlen(p.topico) > 0 && p.tempo_vida == 0){
            printf("MENSAGEM ENVIADA: TOPICO:%s -> '%s' (%d)\n", p.topico, p.str, res);
            return res;
        }else if(strlen(p.topico) <= 0){
            printf("MENSAGEM ENVIADA: '%s' -> MANAGER(%d)\n", p.str, res);
            return res;
        }else{
            printf("MENSAGEM ENVIADA: '%s' ->MANAGER (PID:%d) (%d)\n", p.str, p.pid, res);
            return res;
        }
    }else{
        printf("[ERRO] Nao foi possivel enviar mensagem\n");
        return 0;
    }
}

void handle_exit(int sig) {
    
    printf("\nA encerrar o feed...\n");

    if (fd != -1) {
        close(fd);
    }
    if (fd_feed != -1) {
        close(fd_feed);
    }
    if (fifo_created) {
        unlink(fifo_feed);
    }
    
    exit(0);
}

void *recebe_resposta(void *arg){
    RESPOSTA r; 
    int res;

    while(1){
        res = read(fd_feed, &r, sizeof(RESPOSTA));
        if(res == sizeof(RESPOSTA)){
            if(r.flag == 0){
                printf("\nMENSAGEM RECEBIDA: '%s' -> %s (%d)\n", r.str, r.nome, res);
                handle_exit(0);
            }else if(r.flag == 2){
                printf("\nMENSAGEM RECEBIDA: TOPICO:%s USER:%s -> '%s'(%d)\n", r.topico, r.nome, r.str, res);
            }else if(r.flag == 3){
                printf("\nMENSAGEM PERSISTENTE RECEBIDA: TOPICO:%s USER:%s TEMPO:%d -> '%s'(%d)\n", r.topico, r.nome, r.tempo, r.str, res);
            }else{
                printf("\nMENSAGEM RECEBIDA: USER:%s -> '%s' (%d)\n", r.nome, r.str, res);
            }
        }else{
            printf("[ERRO] Impossivel ler mensagem!\n");
        }
    }
}

int main(int argc, char *argv[]) {
    int res;
    RESPOSTA r;
    PEDIDO login;

    // Registar manipulador para SIGINT
    signal(SIGINT, handle_exit);

    if (argc <= 1) {
        perror("[ERRO] Precisa de especificar um nome!!\n");
        exit(1);
    }

    if (access(FIFO_MANAGER, F_OK) != 0) {
        perror("[ERRO] O servidor não está a funcionar!!\n");
        exit(1);
    }

    // Criar nome para o FIFO do cliente
    sprintf(fifo_feed, FIFO_FEED, getpid());

    // Criar FIFO do cliente
    if (mkfifo(fifo_feed, 0600) == -1) {
        perror("[ERRO] Não foi possível criar o FIFO do cliente");
        exit(2);
    }
    fifo_created = 1; // FIFO foi criado com sucesso

    // Abrir FIFO do cliente
    fd_feed = open(fifo_feed, O_RDWR);
    if (fd_feed == -1) {
        perror("[ERRO] Não foi possível abrir o FIFO do cliente");
        unlink(fifo_feed);
        exit(3);
    }

    // Abrir FIFO do servidor
    fd = open(FIFO_MANAGER, O_WRONLY);
    if (fd == -1) {
        perror("[ERRO] Não foi possível abrir o FIFO do servidor");
        close(fd_feed);
        unlink(fifo_feed);
        exit(4);
    }

    // Enviar login
    strcpy(login.nome, argv[1]);
    login.pid = getpid();
    login.flagVerificado = 0;

    res = write(fd, &login, sizeof(PEDIDO));
    if (res == sizeof(PEDIDO)) {
        printf("TENTATIVA DE LOGIN ENVIADA COM SUCESSO\n");
        res = read(fd_feed, &r, sizeof(RESPOSTA));   
        if(res > 0){ 
            if (r.flag == 0) {
                printf("MENSAGEM RECEBIDA: '%s' - %s\n", r.str, r.nome);
                printf("[ERRO] Tentativa de login falhou\n");
                close(fd);
                close(fd_feed);
                unlink(fifo_feed);
                exit(3);
            } else {
                printf("MENSAGEM RECEBIDA: '%s' - %s\n", r.str, r.nome);
                printf("LOGIN SUCESSO\n");
                login.flagVerificado = 1;
            }
        }else{
            printf("[ERRO] Impossível ler resposta do login\n");
        }
    } else {
        printf("[ERRO] Impossível escrever pedido de login\n");
    }

    pthread_t espera_resposta;
    pthread_create(&espera_resposta, NULL, recebe_resposta, NULL);

    while (1) {
        PEDIDO p;
        char comando[20], str[TAM], topico[TAM_USERNAME];
        int tvida;
        p.pid = getpid();

        printf("COMANDO('help' para ver comandos): ");

        if (!fgets(comando, sizeof(comando), stdin)) {
            continue;
        }

        if (strncmp(comando, "msg", 3) == 0) {
            if(sscanf(comando, "msg %s %d", topico, &tvida) == 2){
                printf("DIGITE A MENSAGEM: ");
                if (fgets(str, sizeof(str), stdin)) {   
                    str[strcspn(str, "\n")] = '\0';
                    strncpy(p.str, str, sizeof(p.str) - 1);
                    p.str[sizeof(p.str) - 1] = '\0';
                    strcpy(p.topico, topico);
                    strcpy(p.nome, login.nome);
                    if(tvida < 0){
                        tvida = 0;
                    }
                    p.tempo_vida = tvida;
                    p.flagVerificado = 1;
                    escreve_mensagem_manager(p);
                }

            } else {
                printf("Comando inválido. Uso correto: msg <TOPICO> <DURACAO>\n");
            }
        } else if (strncmp(comando, "subscribe", 9) == 0) {
            if (sscanf(comando, "subscribe %s", topico) == 1) {
                printf("Subscrevendo o topico %s...\n", topico);
                strncpy(p.str, "Quero subscrever o topico!", 26);
                p.str[26] = '\0';
                strcpy(p.nome, login.nome);
                strcpy(p.topico, topico);
                p.tempo_vida = 0;
                p.flagVerificado = 2;
                escreve_mensagem_manager(p);
                
            } else {
                printf("Comando inválido. Uso correto: subscribe <TOPICO>\n");
            }
        } else if (strncmp(comando, "unsubscribe", 11) == 0) {
            if (sscanf(comando, "unsubscribe %s", str) == 1) {
                printf("Cancelando a subscricao do topico %s...\n", str);
                strncpy(p.str, "Quero cancelar a subscricao do topico!", 38);
                p.str[38] = '\0';
                strcpy(p.nome, login.nome);
                strcpy(p.topico, topico);
                p.tempo_vida = 0;
                p.flagVerificado = 3;
                escreve_mensagem_manager(p);
                
            } else {
                printf("Comando inválido. Uso correto: unsubscribe <TOPICO>\n");
            }
        }else if (strncmp(comando, "topics", 6) == 0) {
                strncpy(p.str, "Quero ver os topicos!", 21);
                p.str[21] = '\0';
                strcpy(p.topico, "");
                strcpy(p.nome, login.nome);
                p.tempo_vida = 0;
                p.flagVerificado = 4;
                escreve_mensagem_manager(p);

                
        }else if (strncmp(comando, "help", 4) == 0) {
                printf("Comandos disponíveis:\n");
                printf("  msg <TOPICO> <DURACAO>\n");
                printf("  subscribe <TOPICO>\n");
                printf("  unsubscribe <TOPICO>\n");
                printf("  topics\n");
                printf("  exit\n");
        }else if (strncmp(comando, "exit", 4) == 0) {
                strncpy(p.str, "Vou sair!", 9);
                p.str[9] = '\0';
                strcpy(p.topico, "");
                strcpy(p.nome, login.nome);
                p.tempo_vida = 0;
                p.flagVerificado = -1;
                escreve_mensagem_manager(p);
        }else {
            printf("Comando não reconhecido.\n");
        }
    }

    // Fechar e apagar FIFO do cliente
    close(fd);
    close(fd_feed);
    unlink(fifo_feed);

    return 0;
}



