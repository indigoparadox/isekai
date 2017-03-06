#ifndef SERVER_H
#define SERVER_H

#include "vector.h"
#include "connection.h"
#include "client.h"
#include "channel.h"

struct CLIENT;

typedef struct _SERVER {
   /* "Root" class is REF*/

   /* "Parent class" */
   struct CLIENT self;

   /* Items after this line are server-specific. */
   struct HASHMAP clients;
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
void server_client_printf( SERVER* s, struct CLIENT* c, const char* message, ... );
void server_client_send( struct CLIENT* c, bstring buffer );
void server_channel_printf( SERVER* s, struct CHANNEL* l, struct CLIENT* c_skip, const char* message, ... );
void server_channel_send( SERVER* s, struct CHANNEL* l, struct CLIENT* c_skip, bstring buffer );
void server_add_client( SERVER* s, struct CLIENT* n );
struct CHANNEL* server_add_channel( SERVER* s, bstring l_name, struct CLIENT* c_first );
void server_add_connection( SERVER* s, struct CLIENT* n );
struct CHANNEL* server_get_channel_by_name( SERVER* s, bstring nick );
struct CLIENT* server_get_client( SERVER* s, const bstring nick );
struct CLIENT* server_get_client_by_ptr( SERVER* s, struct CLIENT* c );
void server_cleanup_client_channels( SERVER* s, struct CLIENT* c );
void server_drop_client( SERVER* s, bstring nick );
void server_listen( SERVER* s, int port );
void server_poll_new_clients( SERVER* s );
void server_service_clients( SERVER* s );
void server_set_client_nick( SERVER* s, struct CLIENT* c, const bstring nick );

#endif /* SERVER_H */
