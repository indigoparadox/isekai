#ifndef SERVER_H
#define SERVER_H

#include "vector.h"
#include "connection.h"
#include "client.h"
#include "channel.h"

typedef struct _CLIENT CLIENT;

typedef struct _SERVER {
   /* "Root" class is REF*/

   /* "Parent class" */
   CLIENT self;

   /* Items after this line are server-specific. */
   HASHMAP clients;
   bstring servername;
   bstring version;
} SERVER;

#define SERVER_SENTINAL 164641

#define SERVER_RANDOM_NICK_LEN 10

#define server_new( s, myhost ) \
    s = (SERVER*)calloc( 1, sizeof( SERVER ) ); \
    server_init( s, myhost );

void server_init( SERVER* s, const bstring myhost );
inline void server_stop( SERVER* s );
BOOL server_free( SERVER* s );
void server_client_printf( SERVER* s, CLIENT* c, const char* message, ... );
void server_client_send( CLIENT* c, bstring buffer );
void server_channel_printf( SERVER* s, CHANNEL* l, CLIENT* c_skip, const char* message, ... );
void server_channel_send( SERVER* s, CHANNEL* l, CLIENT* c_skip, bstring buffer );
void server_add_client( SERVER* s, CLIENT* n );
CHANNEL* server_add_channel( SERVER* s, bstring l_name, CLIENT* c_first );
void server_add_connection( SERVER* s, CLIENT* n );
CHANNEL* server_get_channel_by_name( SERVER* s, bstring nick );
//CLIENT* server_get_client( SERVER* s, int index );
CLIENT* server_get_client( SERVER* s, const bstring nick );
CLIENT* server_get_client_by_ptr( SERVER* s, CLIENT* c );
void server_cleanup_client_channels( SERVER* s, CLIENT* c );
void server_drop_client( SERVER* s, bstring nick );
void server_listen( SERVER* s, int port );
void server_poll_new_clients( SERVER* s );
void server_service_clients( SERVER* s );
void server_set_client_nick( SERVER* s, CLIENT* c, const bstring nick );

#endif /* SERVER_H */
