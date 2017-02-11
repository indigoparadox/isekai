
#ifndef SERVER_H
#define SERVER_H

#include "vector.h"
#include "connection.h"
#include "parser.h"
#include "client.h"

typedef struct {
    /* "Parent class" */
    CLIENT self;

    /* Items after this line are server-specific. */
    VECTOR clients;
} SERVER;

#define server_cleanup( s ) \
    connection_cleanup( &(s->self.link ) ); \
    bdestroy( s->self.buffer );

#define SERVER_SENTINAL 164641

void server_init( SERVER* s );
void server_add_connection( SERVER* s, CONNECTION* n );
CONNECTION* server_get_connection( SERVER* s, int index );
void server_listen( SERVER* s );
void server_service_clients( SERVER* s );
void server_stop( SERVER* s );

#endif /* SERVER_H */
