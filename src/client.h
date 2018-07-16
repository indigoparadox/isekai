#ifndef CLIENT_H
#define CLIENT_H

#include "libvcol.h"
#include "graphics.h"
#include "input.h"
#include "datafile.h"
#include "mode.h"
#include "twindow.h"

struct CHANNEL;
struct MOBILE;
struct UI;
struct CONNECTION;
#ifdef USE_CHUNKS
struct CHUNKER_PROGRESS;
struct CHUNKER;
#endif /* USE_CHUNKS */

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
   struct REF refcount; /*!< Parent "class". The "root" class is REF. */

   struct CONNECTION* link;

   /* Items shared between server and client. */
   BOOL running;
   bstring nick;
   bstring username;
   bstring realname;
   bstring remote;
   bstring away;
   bstring mobile_sprite;
   uint8_t irc_mode;
   uint16_t flags;
   struct UI* ui;
   struct HASHMAP channels; /*!< All channels the client is in now, or all
                             *   channels available if this is a server.
                             */

   struct VECTOR delayed_files; /*!< Requests for files to be executed "later"
                                 *   as a way of getting around locks on
                                 *   resource holders due to dependencies
                                 *   (e.g. tileset needed by tilemap).
                                 *   NOT to be confused with chunkers!
                                 */
   struct MOBILE* puppet;
   struct HASHMAP sprites; /*!< Contains sprites for all mobiles/items this
                            *   client encounters on client-side. Not used
                            *   server-side.
                            */
   struct HASHMAP tilesets;
   SCAFFOLD_SIZE tilesets_loaded;
   struct HASHMAP item_catalogs;
   struct VECTOR unique_items;
   MODE gfx_mode;
   struct TILEMAP* active_tilemap;
   struct TWINDOW local_window;

#ifdef USE_CHUNKS
   struct HASHMAP chunkers;
#endif /* USE_CHUNKS */

#ifndef DISABLE_MODE_POV
   GRAPHICS_PLANE cam_pos;
   GRAPHICS_PLANE plane_pos;
   GFX_COORD_FPP* z_buffer;
#endif /* !DISABLE_MODE_POV */

   BOOL local_client;

   int sentinal;     /*!< Used in release version to distinguish from server. */
};
#define CLIENT_SENTINAL 254542

#define CLIENT_NAME_ALLOC 32
#define CLIENT_BUFFER_ALLOC 256

#define client_connected( c ) \
   (FALSE != ipc_connected( c->link ) && TRUE == (c)->running)

#define client_new( c ) \
    c = mem_alloc( 1, struct CLIENT ); \
    scaffold_check_null( c ); \
    client_init( c );

struct GAMEDATA;
struct INPUT;

BOOL cb_client_del_channels( struct VECTOR* v, SCAFFOLD_SIZE idx, void* iter, void* arg );
void* cb_client_get_nick( struct VECTOR* v, SCAFFOLD_SIZE idx, void* iter, void* arg );

void client_init( struct CLIENT* c );
BOOL client_free_from_server( struct CLIENT* c );
BOOL client_free( struct CLIENT* c );
void client_set_active_t( struct CLIENT* c, struct TILEMAP* t );
short client_add_channel( struct CLIENT* c, struct CHANNEL* l )
#ifdef USE_GNUC_EXTENSIONS
__attribute__ ((warn_unused_result))
#endif /* USE_GNUC_EXTENSIONS */
;
struct CHANNEL* client_get_channel_by_name( struct CLIENT* c, const bstring name );
BOOL client_connect( struct CLIENT* c, const bstring server, int port );
void client_set_local( struct CLIENT* c, BOOL val );
BOOL client_is_local( struct CLIENT* c );
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
BOOL client_poll_ui(
   struct CLIENT* c, struct CHANNEL* l, struct INPUT* p
);
void client_poll_input( struct CLIENT* c, struct CHANNEL* l, struct INPUT* p );
void client_set_names(
   struct CLIENT* c, bstring nick, bstring uname, bstring rname
);
struct MOBILE* client_get_puppet( struct CLIENT* c );
struct ITEM* client_get_item( struct CLIENT* c, SCAFFOLD_SIZE serial );
struct ITEM_SPRITESHEET* client_get_catalog(
   struct CLIENT* c, const bstring name
);
void client_set_item( struct CLIENT* c, SCAFFOLD_SIZE serial, struct ITEM* e );
GRAPHICS* client_get_screen( struct CLIENT* c );

#ifdef CLIENT_C
struct tagbstring str_client_cache_path =
   bsStatic( "testdata/livecache" );
struct tagbstring str_wid_debug_tiles_pos =
   bsStatic( "debug_tiles_pos" );
struct tagbstring str_client_window_id_chat = bsStatic( "chat" );
struct tagbstring str_client_window_title_chat = bsStatic( "Chat" );
struct tagbstring str_client_control_id_chat = bsStatic( "chat" );
struct tagbstring str_client_window_id_inv = bsStatic( "inventory" );
struct tagbstring str_client_window_title_inv = bsStatic( "Inventory" );
struct tagbstring str_client_control_id_inv_self = bsStatic( "inv_pane_self" );
struct tagbstring str_client_control_id_inv_ground = bsStatic( "inv_pane_ground" );
SCAFFOLD_MODULE( "client.c" );
#endif /* CLIENT_C */

#endif /* CLIENT_H */
