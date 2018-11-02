#ifndef CLIENT_H
#define CLIENT_H

#include "libvcol.h"
#include "graphics.h"
#include "input.h"
#include "datafile.h"
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

#define CLIENT_SENTINAL 254542

#define CLIENT_NAME_ALLOC 32
#define CLIENT_BUFFER_ALLOC 256

struct GAMEDATA;
struct INPUT;

BOOL cb_client_del_channels( struct VECTOR* v, SCAFFOLD_SIZE idx, void* iter, void* arg );
void* cb_client_get_nick( struct VECTOR* v, SCAFFOLD_SIZE idx, void* iter, void* arg );

struct CLIENT* client_new();
void client_init( struct CLIENT* c );
BOOL client_free_from_server( struct CLIENT* c );
BOOL client_free( struct CLIENT* c );
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
int client_set_names(
   struct CLIENT* c, bstring nick, bstring uname, bstring rname
);
struct MOBILE* client_get_puppet( struct CLIENT* c );
struct ITEM* client_get_item( struct CLIENT* c, SCAFFOLD_SIZE serial );
struct ITEM_SPRITESHEET* client_get_catalog(
   struct CLIENT* c, const bstring name
);
void client_set_item( struct CLIENT* c, SCAFFOLD_SIZE serial, struct ITEM* e );
GRAPHICS* client_get_screen( struct CLIENT* c );
struct TILEMAP* client_get_tilemap( struct CLIENT* c );
int client_set_remote( struct CLIENT* c, const bstring remote );
bstring client_get_nick( struct CLIENT* c );
struct CHUNKER* client_get_chunker( struct CLIENT* c, const bstring key );
GRAPHICS* client_get_sprite( struct CLIENT* c, bstring filename );
struct TILEMAP_TILESET* client_get_tileset(
   struct CLIENT* c, const bstring filename );
void client_add_ref( struct CLIENT* c );
void client_load_tileset_data(
   struct CLIENT* c, const bstring filename, BYTE* data, size_t length );
void client_load_tilemap_data(
   struct CLIENT* c, const bstring filename, BYTE* data, size_t length );
BOOL client_is_loaded( struct CLIENT* c );
struct CHANNEL* client_iterate_channels(
   struct CLIENT* c, hashmap_iter_cb cb, void* data
);
BOOL client_set_sprite( struct CLIENT* c, bstring filename, GRAPHICS* g );
BOOL client_set_tileset( struct CLIENT* c, bstring filename, struct TILEMAP_TILESET* set );
BOOL client_is_running( struct CLIENT* c );
struct CHANNEL* client_get_channel_active( struct CLIENT* c );
struct TWINDOW* client_get_local_window( struct CLIENT* c );
BOOL client_is_listening( struct CLIENT* c );
BOOL client_is_connected( struct CLIENT* c );
CLIENT_FLAGS client_test_flags( struct CLIENT* c, CLIENT_FLAGS flags );
void client_set_flag( struct CLIENT* c, CLIENT_FLAGS flags );
SCAFFOLD_SIZE_SIGNED client_write( struct CLIENT* c, const bstring buffer );
SCAFFOLD_SIZE_SIGNED client_read( struct CLIENT* c, const bstring buffer );
bstring client_get_realname( struct CLIENT* c );
bstring client_get_username( struct CLIENT* c );
int client_remove_channel( struct CLIENT* c, const bstring lname );
bstring client_get_remote( struct CLIENT* c );
size_t client_get_channels_count( struct CLIENT* c );
size_t client_remove_chunkers( struct CLIENT* c, bstring filter );
struct CLIENT* client_from_local_window( struct TWINDOW* twindow );

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
#endif /* CLIENT_C */

#endif /* CLIENT_H */
