
#ifndef CONNECTION_H
#define CONNECTION_H

#include "scaffold.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct _connection {
    int socket;
    BOOL listening;
    void* (*callback)( void* client );
    void* arg;
    pthread_t thread;
    struct sockaddr_in address;
} CONNECTION;

#define connection_free( c ) \
    if( 0 < c->socket ) { \
        close( c->socket ); \
    } \
    free( c );

void connection_init( CONNECTION* c );
void connection_listen(
    CONNECTION* c, int socket, void* (*callback)( void* client ), void* arg
);

#endif /* CONNECTION_H */
