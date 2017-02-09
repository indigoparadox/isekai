
#ifndef SERVER_H
#define SERVER_H

#include "vector.h"
#include "connection.h"

typedef struct {
    CONNECTION server_connection;
    VECTOR connections;
    BOOL running;
} SERVER;

void server_init( SERVER* s );
void server_listen( SERVER* s );
void server_stop( SERVER* s );

#endif /* SERVER_H */
