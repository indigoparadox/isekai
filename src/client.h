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
#include "mobile.h"
#include "chunker.h"

struct CHANNEL;
struct MOBILE;

typedef enum _CLIENT_FLAGS {
   CLIENT_FLAGS_HAVE_USER = 0x01,
   CLIENT_FLAGS_HAVE_WELCOME = 0x02,
   CLIENT_FLAGS_HAVE_MOTD = 0x04,
   CLIENT_FLAGS_HAVE_NICK = 0x08
} CLIENT_FLAGS;

struct CLIENT {
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
   struct HASHMAP channels; /* All channels in now; all channels avail on server. */
   struct HASHMAP chunkers;
   struct VECTOR command_queue;
   struct MOBILE* puppet;
   int sentinal;
};
#define CLIENT_SENTINAL 254542

#define CLIENT_NAME_ALLOC 32
#define CLIENT_BUFFER_ALLOC 256

#define client_new( c ) \
    c = (struct CLIENT*)calloc( 1, sizeof( struct CLIENT ) ); \
    scaffold_check_null( c ); \
    client_init( c );

struct GAMEDATA;

BOOL cb_client_del_channels( struct VECTOR* v, size_t idx, void* iter, void* arg );
void* cb_client_get_nick( struct VECTOR* v, size_t idx, void* iter, void* arg );

void client_init( struct CLIENT* c );
BOOL client_free( struct CLIENT* c );
void client_add_channel( struct CLIENT* c, struct CHANNEL* l );
struct CHANNEL* client_get_channel_by_name( struct CLIENT* c, const bstring name );
void client_connect( struct CLIENT* c, bstring server, int port );
void client_update( struct CLIENT* c );
void client_join_channel( struct CLIENT* c, bstring name );
void client_send( struct CLIENT* c, bstring buffer );
void client_printf( struct CLIENT* c, const char* message, ... );
void client_lock_channels( struct CLIENT* c, BOOL lock );
void client_stop( struct CLIENT* c );
void client_send_file(
   struct CLIENT* c, bstring channel, CHUNKER_DATA_TYPE type, bstring serverpath,
   bstring filepath
);
void client_add_puppet( struct CLIENT* c, struct MOBILE* o );
void client_clear_puppet( struct CLIENT* c );

#endif /* CLIENT_H */
