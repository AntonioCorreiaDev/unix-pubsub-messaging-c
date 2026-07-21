# unix-pubsub-messaging-c

A topic-based messaging platform developed in C for UNIX/Linux environments, as part of the Operating Systems course.

The system is composed of two main programs:

- `Manager` — responsible for central management of users, topics, persistent messages, and administrative commands.
- `Feed` — a client interface used to send and receive messages, subscribe and unsubscribe from topics.

## Purpose

This project was developed to apply core Operating Systems concepts, including:

- inter-process communication;
- named pipes (FIFOs);
- synchronization with mutexes;
- concurrency with threads;
- signal handling;
- resource management;
- persistent data storage.

## Main features

- User login through the `Feed` client
- Topic creation and management
- Topic subscription and unsubscription
- Message delivery to all users or to specific topics
- Persistent messages with time-to-live support
- Listing of available topics
- Topic locking and unlocking
- Persistent message inspection
- Removal of active users
- Clean system shutdown
- Persistent message recovery from file

## Architecture

The application is split into two main components:

### Manager
The `Manager` is the core of the system. It is responsible for:

- authenticating and managing active users;
- creating, locking, and deleting topics;
- distributing messages;
- managing persistent messages;
- saving and loading persistent data;
- executing administrative commands;
- coordinating application shutdown.

### Feed
The `Feed` is the interactive client. It allows users to:

- log in;
- write messages;
- subscribe and unsubscribe from topics;
- list available topics;
- receive asynchronous messages;
- exit safely.

## Inter-process communication

Communication between `Manager` and `Feed` is handled through:

- **named pipes (FIFOs)** for request and response exchange;
- **threads** for concurrent message reception and background tasks;
- **mutexes** to ensure safe access to shared data structures;
- **signals** for clean shutdown handling.

## Main data structures

The project uses shared structures defined in `util.h`, including:

- `PEDIDO` — structure used by the client to send requests to the server;
- `RESPOSTA` — structure used by the server to return data to the client.

## Relevant files

- `manager.c`
- `feed.c`
- `util.h`
- `Makefile`

## Makefile

The `Makefile` automates executable compilation and cleanup of generated files.

Main targets:
- `all`
- `manager`
- `feed`
- `clean`

## Persistence

Persistent messages are saved to a file and reloaded when the `Manager` starts again, preserving the system state.

## Shutdown

The system performs an orderly shutdown, ensuring that:

- FIFOs are closed;
- resources are released;
- clients are notified;
- persistent state is saved.

## Author

- António Correia
