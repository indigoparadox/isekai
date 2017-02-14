
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
    bstring servername;
    bstring version;
} SERVER;


#define SERVER_SENTINAL 164641

void server_init( SERVER* s, bstring myhost );
void server_cleanup( SERVER* s );
void server_add_connection( SERVER* s, CLIENT* n );
CLIENT* server_get_connection( SERVER* s, int index );
void server_listen( SERVER* s, int port );
void server_service_clients( SERVER* s );
void server_stop( SERVER* s );

#endif /* SERVER_H */
