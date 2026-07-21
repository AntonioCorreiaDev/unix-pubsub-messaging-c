# unix-pubsub-messaging-c

Plataforma de mensagens baseada em tópicos desenvolvida em C para ambiente UNIX/Linux, no âmbito da unidade curricular de Sistemas Operativos.

O sistema é composto por dois programas principais:

- `Manager` — responsável pela gestão central de utilizadores, tópicos, mensagens persistentes e comandos administrativos.
- `Feed` — interface de cliente para envio e receção de mensagens, subscrição e cancelamento de tópicos.

## Objetivo

Este projeto foi desenvolvido para aplicar conceitos fundamentais de Sistemas Operativos, nomeadamente:

- comunicação entre processos;
- uso de named pipes (FIFOs);
- sincronização com mutexes;
- concorrência com threads;
- tratamento de sinais;
- gestão de recursos do sistema;
- armazenamento persistente de dados.

## Funcionalidades principais

- Login de utilizadores através do `Feed`
- Criação e gestão de tópicos
- Subscrição e cancelamento de subscrição em tópicos
- Envio de mensagens para todos os utilizadores ou para tópicos específicos
- Suporte a mensagens persistentes com tempo de vida
- Listagem de tópicos disponíveis
- Bloqueio e desbloqueio de tópicos
- Visualização de mensagens persistentes
- Remoção de utilizadores ativos
- Encerramento ordenado do sistema
- Recuperação de mensagens persistentes a partir de ficheiro

## Arquitetura

A aplicação foi dividida em dois componentes principais:

### Manager
O `Manager` funciona como o núcleo do sistema. É responsável por:

- autenticar e gerir utilizadores ativos;
- criar, bloquear e eliminar tópicos;
- distribuir mensagens;
- gerir mensagens persistentes;
- carregar e guardar mensagens persistentes;
- executar comandos administrativos;
- coordenar o encerramento da aplicação.

### Feed
O `Feed` é o cliente interativo. Permite ao utilizador:

- iniciar sessão;
- escrever mensagens;
- subscrever e cancelar subscrições;
- consultar tópicos disponíveis;
- receber mensagens assíncronas;
- terminar sessão de forma segura.

## Comunicação entre processos

A comunicação entre `Manager` e `Feed` é feita através de:

- **named pipes (FIFOs)** para troca de pedidos e respostas;
- **threads** para receção concorrente de mensagens e gestão de tarefas paralelas;
- **mutexes** para garantir acesso seguro às estruturas partilhadas;
- **sinais** para tratamento de encerramento limpo.

## Estruturas principais

O projeto utiliza estruturas partilhadas definidas em `util.h`, incluindo:

- `PEDIDO` — estrutura usada pelo cliente para enviar ações ao servidor;
- `RESPOSTA` — estrutura usada pelo servidor para devolver informações ao cliente.

## Ficheiros relevantes

- `manager.c`
- `feed.c`
- `util.h`
- `Makefile`

## Makefile

O `Makefile` automatiza a compilação dos executáveis e a limpeza dos ficheiros gerados.

Targets principais:
- `all`
- `manager`
- `feed`
- `clean`

## Persistência

As mensagens persistentes são gravadas em ficheiro e recarregadas quando o `Manager` volta a iniciar, garantindo continuidade do estado do sistema.

## Encerramento

O encerramento do sistema é feito de forma ordenada, garantindo:

- fecho de FIFOs;
- libertação de recursos;
- aviso aos clientes;
- gravação do estado persistente.

## Autor
- António Correia
