#ifndef CLIENT_H
#define CLIENT_H

#include "bstrlib/bstrlib.h"
#include "vector.h"
#include "connection.h"
#include "graphics.h"
#include "gamedata.h"
#include "channel.h"
#include "mailbox.h"
#include "hashmap.h"

typedef struct _CHANNEL CHANNEL;

typedef struct _CLIENT {
   /* "Root" class is REF*/

   /* "Parent class" */
   CONNECTION link;

   /* Items shared between server and client. */
   BOOL running;
   bstring nick;
   bstring username;
   bstring realname;
   bstring remote;
   bstring away;
   uint8_t mode;
   uint8_t flags;
   int x; /* Tile X */
   int y; /* Tile Y */
   HASHMAP channels; /* All channels in now; all channels avail on server. */
   HASHMAP chunkers;
   VECTOR command_queue;
   int sentinal;
} CLIENT;

#define CLIENT_FLAGS_HAVE_USER 0x01
#define CLIENT_FLAGS_HAVE_WELCOME 0x02
#define CLIENT_FLAGS_HAVE_MOTD 0x04
#define CLIENT_FLAGS_HAVE_NICK 0x08
#define CLIENT_FLAGS_HAVE_TILEMAP 0x16

#define CLIENT_SENTINAL 254542

#define CLIENT_NAME_ALLOC 32
#define CLIENT_BUFFER_ALLOC 256

#define client_new( c ) \
    c = (CLIENT*)calloc( 1, sizeof( CLIENT ) ); \
    scaffold_check_null( c ); \
    client_init( c );

typedef struct _GAMEDATA GAMEDATA;

BOOL cb_client_del_channels( VECTOR* v, size_t idx, void* iter, void* arg );
void* cb_client_get_nick( VECTOR* v, size_t idx, void* iter, void* arg );
//void* client_cmp_ptr( VECTOR* v, size_t idx, void* iter, void* arg );

void client_init( CLIENT* c );
BOOL client_free( CLIENT* c );
void client_add_channel( CLIENT* c, CHANNEL* l );
CHANNEL* client_get_channel_by_name( CLIENT* c, const bstring name );
void client_connect( CLIENT* c, bstring server, int port );
void client_update( CLIENT* c );
void client_join_channel( CLIENT* c, bstring name );
void client_send( CLIENT* c, bstring buffer );
void client_printf( CLIENT* c, const char* message, ... );
void client_lock_channels( CLIENT* c, BOOL lock );
void client_stop( CLIENT* c );
void client_send_file( CLIENT* c, bstring channel, bstring filepath );

#endif /* CLIENT_H */
