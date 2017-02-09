
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
    int socket;
    BOOL listening;
    void* (*callback)( void* client );
    void* arg;
    pthread_t thread;
    struct sockaddr_in address;
    pthread_mutex_t server_lock;
} CONNECTION;

#define connection_free( c ) \
    if( 0 < c->socket ) { \
        close( c->socket ); \
    } \
    free( c );

void connection_init( CONNECTION* c );
void connection_listen(
    CONNECTION* c, uint16_t port, void* (*callback)( void* client ), void* arg
);
void connection_connect( CONNECTION* c, bstring server, uint16_t port );
ssize_t connection_read_line( CONNECTION* c, bstring buffer );
void connection_lock( CONNECTION* c );
void connection_unlock( CONNECTION* c );

#endif /* CONNECTION_H */
