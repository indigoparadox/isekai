
#ifndef SERVER_H
#define SERVER_H

#include "vector.h"
#include "connection.h"

typedef struct {
    CONNECTION server_connection; /* "Parent class" */

    BOOL running;
    int sentinal;
    VECTOR connections;
} SERVER;

#define server_cleanup( s ) \
    connection_cleanup( &(s->server_connection ) );

void server_init( SERVER* s );
void server_add_connection( SERVER* s, CONNECTION* n );
CONNECTION* server_get_connection( SERVER* s, int index );
void server_listen( SERVER* s );
void server_stop( SERVER* s );

#endif /* SERVER_H */
