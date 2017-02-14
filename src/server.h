
#ifndef SERVER_H
#define SERVER_H

#include "vector.h"
#include "connection.h"
#include "parser.h"
#include "client.h"
#include "channel.h"

typedef struct {
    /* "Parent class" */
    CLIENT self;

    /* Items after this line are server-specific. */
    VECTOR clients;
    VECTOR channels;
    bstring servername;
    bstring version;
    //uint64_t last_serial;
} SERVER;

#define SERVER_SERIAL_MIN 10000
#define SERVER_SENTINAL 164641

#define server_new( s, myhost ) \
    s = (SERVER*)calloc( 1, sizeof( SERVER ) ); \
    server_init( s, myhost );

void server_init( SERVER* s, const bstring myhost );
void server_cleanup( SERVER* s );
void server_add_connection( SERVER* s, CLIENT* n );
void server_add_channel( SERVER* s, CHANNEL* l );
CLIENT* server_get_client( SERVER* s, int index );
CLIENT* server_get_client_by_nick( SERVER* s, const bstring nick, BOOL lock );
CHANNEL* server_get_channel_by_name( SERVER* s, const bstring name );
void server_drop_client( SERVER* s, int index );
void server_listen( SERVER* s, int port );
void server_service_clients( SERVER* s );
int server_set_client_nick( SERVER* s, CLIENT* c, const bstring nick );
void server_stop( SERVER* s );
void server_lock_clients( SERVER* s, BOOL locked );
void server_lock_channels( SERVER* s, BOOL locked );

#endif /* SERVER_H */
