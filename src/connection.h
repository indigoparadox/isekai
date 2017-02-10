
#ifndef CONNECTION_H
#define CONNECTION_H

#include "bstrlib/bstrlib.h"
#include "scaffold.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

typedef struct _connection {
    void* arg;
    int sentinal4;
    BOOL listening;
    int sentinal2;
    void* (*callback)( void* client );
    int sentinal3;
    struct sockaddr_in address;
    int sentinal5;
    pthread_mutex_t server_lock;
    int sentinal6;
    pthread_t thread;
    int sentinal7;
    int socket;
    int sentinal1;
} CONNECTION;

void connection_init( CONNECTION* c );
void connection_listen(
    CONNECTION* c, uint16_t port, void* (*callback)( void* client ), void* arg
);
void connection_connect( CONNECTION* c, bstring server, uint16_t port );
ssize_t connection_read_line( CONNECTION* c, bstring buffer );
void connection_lock( CONNECTION* c );
void connection_unlock( CONNECTION* c );
void connection_cleanup( CONNECTION* c );

#endif /* CONNECTION_H */
