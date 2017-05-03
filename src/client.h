#ifndef CLIENT_H
#define CLIENT_H

#include "bstrlib/bstrlib.h"
#include "chunker.h"
#include "vector.h"
#include "connect.h"
#include "graphics.h"
#include "channel.h"
#include "hashmap.h"
#include "mobile.h"
#include "input.h"

struct CHANNEL;
struct MOBILE;
struct UI;

typedef enum _CLIENT_FLAGS {
   CLIENT_FLAGS_HAVE_USER = 0x01,
   CLIENT_FLAGS_HAVE_WELCOME = 0x02,
   CLIENT_FLAGS_HAVE_MOTD = 0x04,
   CLIENT_FLAGS_HAVE_NICK = 0x08,
   CLIENT_FLAGS_SENT_CHANNEL_JOIN = 0x16
} CLIENT_FLAGS;

struct CLIENT_DELAYED_REQUEST {
   bstring filename;
   DATAFILE_TYPE type;
};

struct CLIENT {
   CONNECTION link;  /*!< Parent "class". The "root" class is REF. */

   /* Items shared between server and client. */
   BOOL running;
   bstring nick;
   bstring username;
   bstring realname;
   bstring remote;
   bstring away;
   bstring mobile_sprite;
   uint8_t mode;
   uint16_t flags;
   struct UI* ui;
   struct HASHMAP channels; /*!< All channels the client is in now, or all
                             *   channels available if this is a server.
                             */
   struct HASHMAP chunkers;
   struct VECTOR delayed_files;
   struct MOBILE* puppet;
   struct HASHMAP sprites; /*!< Contains sprites for all mobiles/items this
                            *   client encounters on client-side. Not used
                            *   server-side.
                            */
   struct HASHMAP tilesets;
   struct HASHMAP item_catalogs;
   struct VECTOR unique_items;
#ifdef ENABLE_LOCAL_CLIENT
   BOOL client_side; /*!< Are we the server mirror or the real client? */
#endif /* ENABLE_LOCAL_CLIENT */
   int sentinal;     /*!< Used in release version to distinguish from server. */
};
#define CLIENT_SENTINAL 254542

#define CLIENT_NAME_ALLOC 32
#define CLIENT_BUFFER_ALLOC 256

#define client_connected( c ) \
   (connection_connected( &(c->link) ) && TRUE == (c)->running)

#define client_new( c, client_side ) \
    c = (struct CLIENT*)calloc( 1, sizeof( struct CLIENT ) ); \
    scaffold_check_null( c ); \
    client_init( c, client_side );

struct GAMEDATA;
struct INPUT;

BOOL cb_client_del_channels( struct VECTOR* v, SCAFFOLD_SIZE idx, void* iter, void* arg );
void* cb_client_get_nick( struct VECTOR* v, SCAFFOLD_SIZE idx, void* iter, void* arg );

void client_init( struct CLIENT* c, BOOL client_side );
BOOL client_free_from_server( struct CLIENT* c );
BOOL client_free( struct CLIENT* c );
void client_add_channel( struct CLIENT* c, struct CHANNEL* l );
struct CHANNEL* client_get_channel_by_name( struct CLIENT* c, const bstring name );
void client_connect( struct CLIENT* c, const bstring server, int port );
void client_remove_all_channels( struct CLIENT* c );
BOOL client_update( struct CLIENT* c, GRAPHICS* g );
void client_free_channels( struct CLIENT* c );
#ifdef USE_CHUNKS
void client_free_chunkers( struct CLIENT* c );
#endif /* USE_CHUNKS */
void client_join_channel( struct CLIENT* c, const bstring name );
void client_leave_channel( struct CLIENT* c, const bstring lname );
void client_send( struct CLIENT* c, const bstring buffer );
void client_printf( struct CLIENT* c, const char* message, ... );
void client_lock_channels( struct CLIENT* c, BOOL lock );
void client_stop( struct CLIENT* c );
#ifdef USE_CHUNKS
void client_send_file(
   struct CLIENT* c, DATAFILE_TYPE type,
   const bstring serverpath, const bstring filepath
);
#endif /* USE_CHUNKS */
void client_set_puppet( struct CLIENT* c, struct MOBILE* o );
void client_clear_puppet( struct CLIENT* c );
void client_request_file_later(
   struct CLIENT* c, DATAFILE_TYPE type, const bstring filename
);
void client_request_file(
   struct CLIENT* c, DATAFILE_TYPE type, const bstring filename
);
#ifdef USE_CHUNKS
void client_process_chunk( struct CLIENT* c, struct CHUNKER_PROGRESS* cp );
void client_handle_finished_chunker( struct CLIENT* c, struct CHUNKER* h );
#endif /* USE_CHUNKS */
void client_poll_input( struct CLIENT* c, struct CHANNEL* l, struct INPUT* p );
void client_set_names(
   struct CLIENT* c, bstring nick, bstring uname, bstring rname
);
struct ITEM* client_get_item( struct CLIENT* c, SCAFFOLD_SIZE serial );
struct ITEM_SPRITESHEET* client_get_catalog(
   struct CLIENT* c, const bstring name
);
void client_set_item( struct CLIENT* c, SCAFFOLD_SIZE serial, struct ITEM* e );
GRAPHICS* client_get_local_screen( struct CLIENT* c );

#ifdef CLIENT_C
struct tagbstring str_client_cache_path =
   bsStatic( "testdata/livecache" );
static struct tagbstring str_wid_debug_tiles_pos =
   bsStatic( "debug_tiles_pos" );
static struct tagbstring str_client_window_id_chat = bsStatic( "chat" );
static struct tagbstring str_client_window_title_chat = bsStatic( "Chat" );
static struct tagbstring str_client_control_id_chat = bsStatic( "chat" );
static struct tagbstring str_client_window_id_inv = bsStatic( "inventory" );
static struct tagbstring str_client_window_title_inv = bsStatic( "Inventory" );
static struct tagbstring str_client_control_id_inv_self = bsStatic( "inv_pane_self" );
static struct tagbstring str_client_control_id_inv_ground = bsStatic( "inv_pane_ground" );
SCAFFOLD_MODULE( "client.c" );
#else
extern struct tagbstring str_client_cache_path;
#endif /* CLIENT_C */

#endif /* CLIENT_H */
