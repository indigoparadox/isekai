
#ifndef CONNECTION_H
#define CONNECTION_H

#include "bstrlib/bstrlib.h"
#include "scaffold.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

typedef struct _connection {
    int socket;
    BOOL listening;
    void* (*callback)( void* client );
    void* arg;
    struct sockaddr_in address;
} CONNECTION;

void connection_init_socket( CONNECTION* c );
CONNECTION* connection_register_incoming( CONNECTION* n_server );
void connection_listen( CONNECTION* n, uint16_t port );
void connection_connect( CONNECTION* c, bstring server, uint16_t port );
ssize_t connection_read_line( CONNECTION* c, bstring buffer );
void connection_lock( CONNECTION* c );
void connection_unlock( CONNECTION* c );
void connection_cleanup( CONNECTION* c );

#endif /* CONNECTION_H */
