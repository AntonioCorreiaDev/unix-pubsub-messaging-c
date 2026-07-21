#include "util.h"

pthread_mutex_t mutex_users = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_topicos = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    char nome[TAM_USERNAME];
    pid_t pid;
    int flag;
} USER;

typedef struct {
    char str[TAM];
    char nome_user[TAM_USERNAME];
    int tempo_vida;
} MENSAGEMP;

typedef struct {
    char nome[TAM_USERNAME];
    MENSAGEMP mensagens[MAX_MENSAGENS];
    int n_mensagens;
    int bloqueado;
    pid_t assinantes[MAX_USERS];
    int n_assinantes;
} TOPICO;


USER users[MAX_USERS];
int n_users = 0;
int n_topicos = 0;
TOPICO topicos[MAX_TOPICOS];


int envia_mensagem_todos(const char *mensagem, int flag) {
    char fifo_feed[TAM];
    int fd_feed, res;
    RESPOSTA r;

    pthread_mutex_lock(&mutex_users);

    if (n_users == 0) {
        pthread_mutex_unlock(&mutex_users);
        printf("Nao existem utilizadores\n");
        return 0;
    }

    for (int i = 0; i < MAX_USERS; i++) {
        if (strlen(users[i].nome) > 0 && users[i].pid > 0) {
            sprintf(fifo_feed, FIFO_FEED, users[i].pid);
            fd_feed = open(fifo_feed, O_WRONLY);

            if (fd_feed == -1) {
                perror("Erro ao abrir FIFO");
                continue;
            }

            r.flag = 1;
            strcpy(r.str, mensagem);
            strcpy(r.nome, "MANAGER");
     
            if(flag == 1){
                r.flag = 0;
            }
     
            res = write(fd_feed, &r, sizeof(RESPOSTA));
            close(fd_feed);
        }
    }

    pthread_mutex_unlock(&mutex_users);
    printf("MENSAGEM GLOBAL ENVIADA: '%s' - ALL (%d)\n", r.str, res);
    return 1;
}

int envia_mensagem(const char *mensagem, pid_t pidf, int flag1, int flag2) {
    char fifo_feed[TAM], nome[TAM_USERNAME], msg[TAM];
    int fd_feed, res, encontra = 0;
    RESPOSTA r;

    pthread_mutex_lock(&mutex_users);

    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].pid == pidf && users[i].flag) {
            encontra = 1;
            strcpy(nome, users[i].nome);
            sprintf(fifo_feed, FIFO_FEED, users[i].pid);
            fd_feed = open(fifo_feed, O_WRONLY);

            if (fd_feed == -1) {
                perror("Erro ao abrir FIFO");
                pthread_mutex_unlock(&mutex_users);
                return 0;
            }

            r.flag = 1;
            strcpy(r.str, mensagem);
            strcpy(r.nome, "MANAGER");
            
            char username[TAM_USERNAME];
            pid_t userpid = users[i].pid; //guarda variaveis caso seja para remover o user
            strcpy(username, users[i].nome);

            if(flag1 == 1){
                printf("Removendo user: %s (PID: %d)\n", users[i].nome, users[i].pid);
                r.flag = 0;
                memset(&users[i], 0, sizeof(USER));  // Reseta o usuário
                n_users--;
            }

            res = write(fd_feed, &r, sizeof(RESPOSTA));
            printf("MENSAGEM ENVIADA: '%s' para...%s (PID:%d) (RES:%d)\n", r.str, username, userpid, res);
        }
    }
    close(fd_feed);
    pthread_mutex_unlock(&mutex_users);
    if (encontra == 0){
        printf("User com pid '%d' nao encontrado!!\n", pidf);
        return 1;
    }
    if(flag2 == 1){
        sprintf(msg, "User %s removido pelo manager!", nome);
        envia_mensagem_todos(msg, 0);
        return 1;
    }
    if(res == 0){
        return 0;
    }
}


int apaga_topico(char t[TAM_USERNAME]){
    
    pthread_mutex_lock(&mutex_topicos);
    
    for (int i = 0; i < MAX_TOPICOS; i++){
        if(strcmp(t, topicos[i].nome) == 0){
            memset(&topicos[i], 0, sizeof(TOPICO));
            n_topicos--;
            
            pthread_mutex_unlock(&mutex_topicos);
            printf("Topico %s apagado!", t);
            return 1;
        }
    }
    pthread_mutex_unlock(&mutex_topicos);
    return 0;
}

int apaga_utilizador_topicos(pid_t pidf){

    int encontrado = 0;
    pthread_mutex_lock(&mutex_topicos);
    
    for (int i = 0; i < MAX_TOPICOS; i++){
        if(topicos[i].n_assinantes > 0){
            for(int j = 0; j < MAX_USERS; j++){
                if (topicos[i].assinantes[j] == pidf){
                    encontrado = 1;
                    topicos[i].assinantes[j] = 0;
                    topicos[i].n_assinantes --;
                    if(topicos[i].n_assinantes <= 0){
                        char nomeT[TAM_USERNAME];
                        strcpy(nomeT, topicos[i].nome);
                        pthread_mutex_unlock(&mutex_topicos);
                        apaga_topico(nomeT);
                    }
                }
            }
        }
    }

    pthread_mutex_unlock(&mutex_topicos);
    return encontrado;

}

void salva_mensagens_persistentes() {
    const char *file_nome = getenv("MSG_FICH");
    if(!file_nome){
        printf("ERRO: impossivel definir variavel ambiente");
        return;
    }
    FILE *f = fopen(file_nome, "w");
    if (!f) {
        perror("Erro ao abrir MSG_FICH");
        return;
    }

    pthread_mutex_lock(&mutex_topicos);

    for (int i = 0; i < MAX_TOPICOS; i++) {
        if (strlen(topicos[i].nome) > 0) {
            for (int j = 0; j < topicos[i].n_mensagens; j++) {
                fprintf(f, "%s %s %d %s\n", topicos[i].nome, 
                topicos[i].mensagens[j].nome_user, 
                topicos[i].mensagens[j].tempo_vida,
                topicos[i].mensagens[j].str);
            }
        }
    }

    pthread_mutex_unlock(&mutex_topicos);
    fclose(f);
}

void handle_exit(int sig) {
    printf("A encerrar o servidor...\n");

    salva_mensagens_persistentes();

    envia_mensagem_todos("O servidor foi encerrado!!", 1);

    for (int i = 0; i < MAX_USERS; i++) {
        if (strlen(users[i].nome) > 0){
            memset(&users[i], 0, sizeof(USER));
        }
    }

    for (int i = 0; i < MAX_TOPICOS; i++) {
        if (strlen(topicos[i].nome) > 0){
            memset(&topicos[i], 0, sizeof(TOPICO));
        }
    }

    unlink(FIFO_MANAGER);
    pthread_mutex_destroy(&mutex_users);
    pthread_mutex_destroy(&mutex_topicos);
    exit(0);
}

int encontra_nome(const char *nome) {
    pthread_mutex_lock(&mutex_users);

    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].flag && strcmp(nome, users[i].nome) == 0) {
            pthread_mutex_unlock(&mutex_users);
            return i;
        }
    }

    pthread_mutex_unlock(&mutex_users);
    return -1;
}

int add_user(const char *nome, pid_t pid) {
    pthread_mutex_lock(&mutex_users);

    if (n_users >= MAX_USERS) {
        pthread_mutex_unlock(&mutex_users);
        printf("Numero maximo de users atingido\n");
        return 0;
    }

    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].flag == 0) {
            strcpy(users[i].nome, nome);
            users[i].pid = pid;
            users[i].flag = 1;
            n_users++;
            pthread_mutex_unlock(&mutex_users);
            return 1;
        }
    }

    pthread_mutex_unlock(&mutex_users);
    return 0;
}

int mostra_users() {
    pthread_mutex_lock(&mutex_users);

    if (n_users == 0) {
        pthread_mutex_unlock(&mutex_users);
        printf("Nao existem utilizadores\n");
        return 0;
    }

    printf("%d - USERS: ", n_users);
    for (int i = 0; i < MAX_USERS; i++) {
        if (users[i].flag != 0 && strlen(users[i].nome) > 0) {
            printf("%s - (PID: %d)\n", users[i].nome, users[i].pid);
        }
    }

    pthread_mutex_unlock(&mutex_users);
    return 1;
}

int procura_topico(char t[TAM_USERNAME]){
    pthread_mutex_lock(&mutex_topicos);
    for (int i = 0; i < MAX_TOPICOS; i++){
        if(strcmp(t, topicos[i].nome) == 0){
            pthread_mutex_unlock(&mutex_topicos);
            return 1;
        }
    }
    pthread_mutex_unlock(&mutex_topicos);
    return 0;
}

int topico_islocked(char t[TAM_USERNAME]){
    pthread_mutex_lock(&mutex_topicos);
    for (int i = 0; i < MAX_TOPICOS; i++){
        if(strcmp(t, topicos[i].nome) == 0){
            if (topicos[i].bloqueado == 1){
                pthread_mutex_unlock(&mutex_topicos);
                return 0;
            }else{
                pthread_mutex_unlock(&mutex_topicos);
                return 1;
            }
        }
    }
    pthread_mutex_unlock(&mutex_topicos);
    printf("O topico nao existe!\n");
    return 0;
}

int lock_topico(char t[TAM_USERNAME]){
    pthread_mutex_lock(&mutex_topicos);
    for (int i = 0; i < MAX_TOPICOS; i++){
        if(strcmp(t, topicos[i].nome) == 0){
            if (topicos[i].bloqueado == 1){
                pthread_mutex_unlock(&mutex_topicos);
                printf("O topico ja esta bloqueado!\n");
                return 1;
            }
            
            topicos[i].bloqueado = 1;
            pthread_mutex_unlock(&mutex_topicos);
            printf("Topico %s bloqueado!\n", t);
            return 1;
        }
    }
    pthread_mutex_unlock(&mutex_topicos);
    printf("O topico nao existe!\n");
    return 0;
}

int unlock_topico(char t[TAM_USERNAME]){
    pthread_mutex_lock(&mutex_topicos);
    for (int i = 0; i < MAX_TOPICOS; i++){
        if(strcmp(t, topicos[i].nome) == 0){
            if (topicos[i].bloqueado == 0){
                pthread_mutex_unlock(&mutex_topicos);
                printf("O topico ja esta desbloqueado!\n");
                return 1;
            }
            
            topicos[i].bloqueado = 0;
            pthread_mutex_unlock(&mutex_topicos);
            printf("Topico %s desbloqueado!\n", t);
            return 1;
        }
    }
    pthread_mutex_unlock(&mutex_topicos);
    printf("O topico nao existe!\n");
    return 0;
}


int mostra_topicos(pid_t pidf){

    pthread_mutex_lock(&mutex_topicos);

    if(n_topicos == 0){
        if(pidf == 0){
            pthread_mutex_unlock(&mutex_topicos);
            printf("Nao existem topicos!\n");
            return 0;
        }else{
            pthread_mutex_unlock(&mutex_topicos);
            envia_mensagem("Nao existem topicos!", pidf, 0, 0);
            return 0;
        }
    }
    printf("%d Topicos:\n", n_topicos);
    for (int i = 0; i < MAX_TOPICOS; i++){
        if(pidf == 0){
            if(topicos[i].bloqueado == 0 && strlen(topicos[i].nome) > 0){
                printf("  NOME: %s MENSAGENS PERSISTENTES: %d -> UNLOCKED\n",topicos[i].nome ,topicos[i].n_mensagens);
            }else if(topicos[i].bloqueado == 1 && strlen(topicos[i].nome) > 0){
                printf("  NOME: %s MENSAGENS PERSISTENTES: %d -> LOCKED\n",topicos[i].nome ,topicos[i].n_mensagens);
            }
        }else{
            if(topicos[i].bloqueado == 0 && strlen(topicos[i].nome) > 0){
                char msg[TAM];
                sprintf(msg, "  NOME: %s MENSAGENS PERSISTENTES: %d -> UNLOCKED",topicos[i].nome ,topicos[i].n_mensagens);
                envia_mensagem(msg, pidf, 0, 0);
            }else if(topicos[i].bloqueado == 1 && strlen(topicos[i].nome) > 0){
                char msg[TAM];
                sprintf(msg, "  NOME: %s MENSAGENS PERSISTENTES: %d -> LOCKED",topicos[i].nome ,topicos[i].n_mensagens);
                envia_mensagem(msg, pidf, 0, 0);
            }
        }
    }
    pthread_mutex_unlock(&mutex_topicos);
    return 1;
}

int procura_utilizador_topico(char t[TAM_USERNAME], pid_t pidf){
    pthread_mutex_lock(&mutex_topicos);
    for (int i = 0; i < MAX_TOPICOS; i++){
        if(strcmp(t, topicos[i].nome) == 0){
            for(int j = 0; j < MAX_USERS; j++){
                if (topicos[i].assinantes[j] == pidf){
                    pthread_mutex_unlock(&mutex_topicos);
                    return j;
                }
            }
            pthread_mutex_unlock(&mutex_topicos);
            return -1;
        }
    }
    pthread_mutex_unlock(&mutex_topicos);
    return -2;
}

int utilizador_cancela_sub(char t[TAM_USERNAME], pid_t pidf){
    pthread_mutex_lock(&mutex_topicos);
    for (int i = 0; i < MAX_TOPICOS; i++){
        if(strcmp(t, topicos[i].nome) == 0){
            for(int j = 0; j < MAX_USERS; j++){
                if(topicos[i].assinantes[j] == pidf){
                    topicos[i].assinantes[j] = 0;
                    topicos[i].n_assinantes --;
                    if(topicos[i].n_assinantes == 0 && topicos[i].n_mensagens == 0){ 
                        pthread_mutex_unlock(&mutex_topicos);
                        return -1;
                    }
                    pthread_mutex_unlock(&mutex_topicos);
                    return 1;
                }
            }
        }
    }
    pthread_mutex_unlock(&mutex_topicos);
    return 0;
}

int utilizador_subscreve_topico(char t[TAM_USERNAME], pid_t pidf){
    
    pthread_mutex_lock(&mutex_topicos);
    for (int i = 0; i < MAX_TOPICOS; i++){
        if(strcmp(t, topicos[i].nome) == 0){
            for(int j = 0; j < MAX_USERS; j++){
                if(topicos[i].assinantes[j] == 0){
                    topicos[i].assinantes[j] = pidf;
                    topicos[i].n_assinantes ++;
                    pthread_mutex_unlock(&mutex_topicos);
                    return 1;
                }
            }
        }
    }
    pthread_mutex_unlock(&mutex_topicos);
    return 0;
}

int envia_mensagem_topico(char t[TAM_USERNAME], const char *msg, const char *user, int tvida){
    char fifo_feed[TAM];
    int fd_feed, res, encontra = 0;
    RESPOSTA r;

    pthread_mutex_lock(&mutex_topicos);
    
    for (int i = 0; i < MAX_TOPICOS; i++){
        if(strcmp(t, topicos[i].nome) == 0){
            for(int j = 0; j < MAX_USERS; j++){
                if (topicos[i].assinantes[j] > 0){
                
                    sprintf(fifo_feed, FIFO_FEED, topicos[i].assinantes[j]);
                    fd_feed = open(fifo_feed, O_WRONLY);

                    if (fd_feed == -1) {
                        pthread_mutex_unlock(&mutex_topicos);
                        perror("Erro ao abrir FIFO\n");
                        return 0;
                    }
                    strcpy(r.str, msg);
                    strcpy(r.nome, user);
                    strcpy(r.topico, t);
                    r.flag = 2;

                    if(tvida > 0){
                        r.flag = 3;
                        r.tempo = tvida;
                    }

                    res = write(fd_feed, &r, sizeof(RESPOSTA));
                    printf("MENSAGEM ENVIADA: '%s' PARA: %s (PID:%d) (RES:%d)\n", 
                    r.str, t, topicos[i].assinantes[j], res);
                    close(fd_feed);
                    
                }
            }
            pthread_mutex_unlock(&mutex_topicos);
            return 1;
        }
    }
}

int cria_topico(pid_t criador, char t[TAM_USERNAME]){
    pthread_mutex_lock(&mutex_topicos);
    if(n_topicos < MAX_TOPICOS && strlen(t) > 0){
        strcpy(topicos[n_topicos].nome, t);
        topicos[n_topicos].assinantes[topicos[n_topicos].n_assinantes] = criador;
        topicos[n_topicos].n_assinantes ++;
        n_topicos ++;
        pthread_mutex_unlock(&mutex_topicos);
        printf("Topico %s criado!\n", t);
        return 1;
    }
    pthread_mutex_unlock(&mutex_topicos);
    printf("Numero maximo de topicos atingido!\n");
    return 0;

}


int add_mensagem_persistente(char t[TAM_USERNAME], const char *user, int tvida, const char *msg){
    
    pthread_mutex_lock(&mutex_topicos);
    for (int i = 0; i < MAX_TOPICOS; i++){
        if(strcmp(t, topicos[i].nome) == 0){
            strcpy(topicos[i].mensagens[topicos[i].n_mensagens].str, msg);
            strcpy(topicos[i].mensagens[topicos[i].n_mensagens].nome_user, user);
            topicos[i].mensagens[topicos[i].n_mensagens].tempo_vida = tvida;
            topicos[i].n_mensagens++;
            pthread_mutex_unlock(&mutex_topicos);
            return 1;
        }
    }
    pthread_mutex_unlock(&mutex_topicos);
    return 0;
}

int mostra_mensagens_persistentes(char t[TAM_USERNAME]){
    
    pthread_mutex_lock(&mutex_topicos);
    for (int i = 0; i < MAX_TOPICOS; i++){
        if(strcmp(t, topicos[i].nome) == 0){
            if(topicos[i].n_mensagens == 0){
                pthread_mutex_unlock(&mutex_topicos);
                return 0;
            }else{
                printf("-----MENSAGENS-PERSISTENTES-----\n");
            }
            for(int j = 0; j < topicos[i].n_mensagens; j++){
                printf("TOPICO:%s USER:%s TVIDA:%d MSG:%s\n",topicos[i].nome, topicos[i].mensagens[j].nome_user, 
                topicos[i].mensagens[j].tempo_vida, topicos[i].mensagens[j].str);
            }
            pthread_mutex_unlock(&mutex_topicos);
            return 1;
        }
    }
    pthread_mutex_unlock(&mutex_topicos);
    return -1;
}


int envia_mensagens_persistentes(char t[TAM_USERNAME], pid_t pidf){
    char fifo_feed[TAM];
    int fd_feed, res, encontra = 0;
    RESPOSTA r;

    pthread_mutex_lock(&mutex_topicos);
    
    for (int i = 0; i < MAX_TOPICOS; i++){
        if(strcmp(t, topicos[i].nome) == 0){
            if(topicos[i].n_mensagens == 0){
                pthread_mutex_unlock(&mutex_topicos);
                return 0;
            }else{
                sprintf(fifo_feed, FIFO_FEED, pidf);
                fd_feed = open(fifo_feed, O_WRONLY);

                if (fd_feed == -1) {
                    pthread_mutex_unlock(&mutex_topicos);
                    perror("Erro ao abrir FIFO\n");
                    return 0;
                }
            }
            for(int j = 0; j < topicos[i].n_mensagens; i++){
                strcpy(r.str, topicos[i].mensagens[j].str);
                strcpy(r.nome, topicos[i].mensagens[j].nome_user);
                strcpy(r.topico, topicos[i].nome);
                r.tempo = topicos[i].mensagens[j].tempo_vida;
                r.flag = 3;

                res = write(fd_feed, &r, sizeof(RESPOSTA));

            }
            pthread_mutex_unlock(&mutex_topicos);
            close(fd_feed);
            return 1;
        }
    }
    pthread_mutex_unlock(&mutex_topicos);
    return -1;
}

void carrega_mensagens_persistentes() {
    const char *file_nome = getenv("MSG_FICH");
    if(!file_nome){
        printf("ERRO: impossivel definir variavel ambiente");
        return;
    }
    FILE *f = fopen(file_nome, "r");
    if (!f) {
        printf("Arquivo MSG_FICH nao encontrado\n");
        return;
    }

    char topico[TAM_USERNAME], mensagem[TAM], autor[TAM_USERNAME];
    int tempo_vida;
    while (fscanf(f, "%s %s %d %[^\n]", topico, autor, &tempo_vida, mensagem) == 4) {
        for (int i = 0; i < MAX_TOPICOS; i++) {
            if (strcmp(topicos[i].nome, topico) == 0 || strlen(topicos[i].nome) == 0) {
                if (strlen(topicos[i].nome) == 0) {
                    strcpy(topicos[i].nome, topico);
                    n_topicos++;
                }
                strcpy(topicos[i].mensagens[topicos[i].n_mensagens].str, mensagem);
                strcpy(topicos[i].mensagens[topicos[i].n_mensagens].nome_user, autor);
                topicos[i].mensagens[topicos[i].n_mensagens].tempo_vida = tempo_vida;
                topicos[i].n_mensagens++;
                break;
            }
        }
    }

    fclose(f);
}


void *decrementa_tempo_mensagens(void *arg) {
    while (1) {
        int flag = 0;
        char t[TAM_USERNAME];
        pthread_mutex_lock(&mutex_topicos);

        for (int i = 0; i < MAX_TOPICOS; i++) {
            if (strlen(topicos[i].nome) > 0) { // Verifica topico
                for (int j = 0; j < topicos[i].n_mensagens; j++) {
                    if (topicos[i].mensagens[j].tempo_vida > 0) {
                        topicos[i].mensagens[j].tempo_vida--;

                        // Remove se o tempo de vida chegar a 0
                        if (topicos[i].mensagens[j].tempo_vida == 0) {
                            printf("Mensagem expirada no tópico %s: %s\n",topicos[i].nome, 
                            topicos[i].mensagens[j].str);

                            // Remover mensagens deslocando
                            for (int k = j; k < topicos[i].n_mensagens - 1; k++) {
                                topicos[i].mensagens[k] = topicos[i].mensagens[k + 1];
                            }
                            topicos[i].n_mensagens--;
                            if(topicos[i].n_mensagens == 0 && topicos[i].n_assinantes == 0){
                                strcpy(t, topicos[i].nome);
                                flag = 1;
                            }
                            j--; // Ajusta i isdndice para continuar a verificar mensagens
                        }
                    }
                }
            }
        }

        pthread_mutex_unlock(&mutex_topicos);
        if(flag == 1){
            apaga_topico(t);
        }
        // Espera 1 seg
        sleep(1);
    }

    return NULL;
}


void *comandos_manager(void *arg) {
    char comando[TAM_USERNAME];
    char str[TAM], topico[TAM_USERNAME];
    pid_t pid;

    while (1) {
        printf("Comando: ");
        if (!fgets(comando, sizeof(comando), stdin)) {
            continue;
        }

        if (strncmp(comando, "remover", 7) == 0) {
            if (sscanf(comando, "remover %d", &pid) == 1) {
                envia_mensagem("Voce foi expulso pelo manager", pid, 1, 1);
                apaga_utilizador_topicos(pid);
            } else {
                printf("Comando inválido. Uso correto: remover <PID>\n");
            }
        } else if (strncmp(comando, "users", 5) == 0) {
                mostra_users();
        } else if (strncmp(comando, "topics", 6) == 0) {
                printf("Listando topicos...\n");
                mostra_topicos(0);
        }else if (strncmp(comando, "show", 4) == 0) {
            if (sscanf(comando, "show %s", topico) == 1) {
                printf("Mostrando o topico %s...\n", topico);
                int resultado = mostra_mensagens_persistentes(topico);
                if(resultado == 0){
                    printf("Nao existem mensagens persistentes no topico %s\n", topico);
                }else if(resultado == -1){
                    printf("Impossivel mostrar mensagens persistentes\n");
                }
                
            } else {
                printf("Comando inválido. Uso correto: show <TOPICO>\n");
            }
        }else if (strncmp(comando, "lock", 4) == 0) {
            if (sscanf(comando, "lock %s", topico) == 1) {
                printf("Bloqueando o topico %s...\n", topico);
                lock_topico(topico);
                envia_mensagem_topico(topico, "O topico foi bloqueado!", "MANAGER", 0);
            } else {
                printf("Comando inválido. Uso correto: lock <TOPICO>\n");
            }
        }else if (strncmp(comando, "unlock", 6) == 0) {
            if (sscanf(comando, "unlock %s", &topico) == 1) {
                printf("Desbloqueando o topico %s...\n", topico);
                envia_mensagem_topico(topico, "O topico foi desbloqueado!", "MANAGER", 0);
                unlock_topico(topico);
                
            } else {
                printf("Comando inválido. Uso correto: unlock <TOPICO>\n");
            }
        } else if (strncmp(comando, "close", 5) == 0) {
                handle_exit(0);
        }else {
            printf("Comando não reconhecido.\n");
        }
    }

    return NULL;
}


int main(int argc, char *argv[]) {
    int fd, fd_feed, res;
    char fifo_feed[TAM];
    RESPOSTA rlogin;

    signal(SIGINT, handle_exit);

    if (access(FIFO_MANAGER, F_OK) == 0) {
        printf("[ERRO] Já existe um servidor!\n");
        exit(3);
    }

    if (mkfifo(FIFO_MANAGER, 0666) == -1) {
        perror("Erro ao criar named pipe\n");
        exit(1);
    }

    printf("A aguardar mensagens...\n");
    fd = open(FIFO_MANAGER, O_RDWR);
    if (fd == -1) {
        perror("Erro ao abrir named pipe\n");
        exit(3);
    }

    pthread_t thread_comando;
    pthread_create(&thread_comando, NULL, comandos_manager, NULL);

    pthread_t thread_mensagensp;
    pthread_create(&thread_mensagensp, NULL, decrementa_tempo_mensagens, NULL);

    carrega_mensagens_persistentes();
    
    while (1) {
        PEDIDO p;
        res = read(fd, &p, sizeof(PEDIDO));
        if (res == sizeof(PEDIDO)) {
            sprintf(fifo_feed, FIFO_FEED, p.pid);
            fd_feed = open(fifo_feed, O_WRONLY);
            if (fd_feed == -1) {
                perror("Erro ao abrir FIFO do cliente\n");
                pthread_exit(NULL);
            }

            if (p.flagVerificado == 0) {
                if (encontra_nome(p.nome) == -1) {
                    if (add_user(p.nome, p.pid)) {
                        strcpy(rlogin.str, "USER ADICIONADO");
                        strcpy(rlogin.nome, "MANAGER");
                        rlogin.flag = 1;
                    } else {
                        strcpy(rlogin.str, "USERS MAX");
                        strcpy(rlogin.nome, "MANAGER");
                        rlogin.flag = 0;
                    }
                } else{
                    strcpy(rlogin.str, "USER JA EXISTE");
                    strcpy(rlogin.nome, "MANAGER");
                    rlogin.flag = 0;
                }
                printf("MENSAGEM ENVIADA: '%s' PARA: %s (PID:%d) (RES:%d)\n", rlogin.str, p.nome, p.pid, res);
                write(fd_feed, &rlogin, sizeof(RESPOSTA));
                close(fd_feed);

            } else if (p.flagVerificado == 1) { //mensagem enviada para topico
                if(p.tempo_vida > 0){
                    printf("USER:%s: TOPICO:%s TVIDA:%d MSG:%s - PID:%d\n", p.nome, p.topico, p.tempo_vida, p.str, p.pid);
                    if(procura_topico(p.topico) == 1){
                            if(procura_utilizador_topico(p.topico, p.pid) >= 0){
                                if(topico_islocked(p.topico) == 1){
                                    envia_mensagem_topico(p.topico, p.str, p.nome, p.tempo_vida);
                                    if (add_mensagem_persistente(p.topico, p.nome, p.tempo_vida, p.str) == 1){
                                        printf("Mensagem persistente adicionada com sucesso\n");    
                                    }
                                }else{
                                    envia_mensagem("[ERRO] O topico esta bloqueado!", p.pid, 0, 0);
                                }
                            }else{
                                envia_mensagem("[ERRO] Voce nao subscreveu o topico!", p.pid, 0, 0);
                            }
                        }else{
                            envia_mensagem("[ERRO] O topico nao existe!", p.pid, 0, 0);
                        }
                }else{
                    printf("USER:%s: TOPICO:%s MSG:%s - PID:%d\n", p.nome, p.topico, p.str, p.pid);
                        if(procura_topico(p.topico) == 1){
                            if(procura_utilizador_topico(p.topico, p.pid) >= 0){
                                if(topico_islocked(p.topico) == 1){
                                    envia_mensagem_topico(p.topico, p.str, p.nome, 0);
                                }else{
                                    envia_mensagem("[ERRO] O topico esta bloqueado!", p.pid, 0, 0);
                                }
                            }else{
                                envia_mensagem("[ERRO] Voce nao subscreveu o topico!", p.pid, 0, 0);
                            }
                        }else{
                            envia_mensagem("[ERRO] O topico nao existe!", p.pid, 0, 0);
                        }
                    }

            }else if (p.flagVerificado == 2) { //subscreve topico
                printf("USER:%s: MSG:%s TOPICO:%s - PID:%d\n", p.nome, p.str, p.topico, p.pid);
                envia_mensagem("OK", p.pid, 0, 0);

                if(procura_topico(p.topico) == 0){
                    cria_topico(p.pid, p.topico);
                    char msg[50];
                    sprintf(msg, "Criou e subscreveu o topico %s", p.topico);
                    envia_mensagem(msg, p.pid, 0, 0);
                }else{
                    if(procura_utilizador_topico(p.topico, p.pid) >= 0){
                        envia_mensagem("O topico ja foi subscrito", p.pid, 0, 0);
                    }else{
                        if(utilizador_subscreve_topico(p.topico, p.pid) == 1){
                            char msg[50];
                            sprintf(msg, "Subscreveu o topico %s", p.topico);
                            envia_mensagem(msg, p.pid, 0, 0);
                            envia_mensagens_persistentes(p.topico, p.pid);
                            envia_mensagem_topico(p.topico, "Subscreveu o topico", p.nome, 0);

                        }
                    }
                }

            }else if (p.flagVerificado == 3) { //unsub topico
                printf("USER:%s: MSG:%s TOPICO:%s - PID:%d\n", p.nome, p.str, p.topico, p.pid);
                envia_mensagem("OK", p.pid, 0, 0);

                if(procura_topico(p.topico) == 0){
                    envia_mensagem("O topico nao existe", p.pid, 0, 0);
                }else{
                    if(procura_utilizador_topico(p.topico, p.pid) < 0){
                        envia_mensagem("Voce nao esta subscrito ao topico", p.pid, 0, 0);
                    }else{
                        if(utilizador_cancela_sub(p.topico, p.pid) == -1){
                            envia_mensagem("Subscricao cancelada com sucesso", p.pid, 0, 0);
                            apaga_topico(p.topico);
                        }else{
                            envia_mensagem("Subscricao cancelada com sucesso", p.pid, 0, 0);
                            envia_mensagem_topico(p.topico, "Cancelou a subscricao", p.nome, 0);

                        }
                    }
                }   

            }else if (p.flagVerificado == 4) { //mostra topicos ao utilizador
                printf("USER:%s: MSG:%s TOPICO:%s - PID:%d\n", p.nome, p.str, p.topico, p.pid);
                envia_mensagem("OK", p.pid, 0, 0);
                mostra_topicos(p.pid);

            }else if (p.flagVerificado == -1){ //saida do utilizador
                printf("%s: %s - %d\n", p.nome, p.str, p.pid);
                envia_mensagem("Voce foi removido pelo manager", p.pid, 1, 0);
                apaga_utilizador_topicos(p.pid);
            }
        }else {
            printf("[ERRO] Mensagem desconhecida ou incompleta recebida.\n");
        }
    }


    close(fd);
    handle_exit(0);
}
