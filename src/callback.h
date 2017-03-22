
#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "vector.h"
#include "hashmap.h"

void* callback_ingest_commands( const bstring key, void* iter, void* arg );
void* callback_concat_clients( const bstring key, void* iter, void* arg );
void* callback_search_clients( const bstring key, void* iter, void* arg );
void* callback_search_clients_r( const bstring key, void* iter, void* arg );
void* callback_search_clients_l( const bstring key, void* iter, void* arg );
void* callback_search_spawners( const bstring key, void* iter, void* arg );
void* callback_send_clients( const bstring key, void* iter, void* arg );
void* callback_search_channels( const bstring key, void* iter, void* arg );
void* callback_search_mobs_by_pos( const bstring res, void* iter, void* arg );
void* callback_search_windows( const bstring key, void* iter, void* arg );
void* callback_search_tileset_img_gid( const bstring key, void* iter, void* arg );
void* callback_search_tilesets_img_name( const bstring key, void* iter, void* arg );
void* callback_search_channels_tilemap_img_name( const bstring key, void* iter, void* arg );
void* callback_search_tilesets_name( const bstring key, void* iter, void* arg );
void* callback_search_graphics( const bstring key, void* iter, void* arg );
void* callback_search_servefiles( const bstring res, void* iter, void* arg );
#ifdef USE_CHUNKS
void* callback_send_chunkers_l( const bstring key, void* iter, void* arg );
#endif /* USE_CHUNKS */
void* callback_send_mobs_to_client( const bstring res, void* iter, void* arg );
void* callback_send_mobs_to_channel( const bstring res, void* iter, void* arg );
void* callback_send_updates_to_client( const bstring res, void* iter, void* arg );
void* callback_parse_mobs( const bstring res, void* iter, void* arg );
void* callback_parse_mob_channels( const bstring key, void* iter, void* arg );
BOOL callback_send_list_to_client( const bstring res, void* iter, void* arg );
void* callback_get_tile_stack_l( bstring key, void* iter, void* arg );
void* callback_get_tile_blocker( bstring res, void* iter, void* arg );
#ifdef USE_CHUNKS
void* callback_proc_client_chunkers( const bstring key, void* iter, void* arg );
void* callback_proc_chunkers( const bstring key, void* iter, void* arg );
#endif /* USE_CHUNKS */
void* callback_proc_mobile_vms( const bstring res, void* iter, void* arg );
void* callback_proc_channel_vms( const bstring res, void* iter, void* arg );
void* callback_proc_tileset_img_gs( const bstring key, void* iter, void* arg );
void* callback_proc_tileset_imgs( const bstring key, void* iter, void* arg );
void* callback_proc_channel_spawners(
   const bstring key, void* iter, void* arg
);
void* callback_proc_server_spawners( const bstring key, void* iter, void* arg );
void* callback_search_tilesets_gid( const bstring res, void* iter, void* arg );
void* callback_draw_mobiles( const bstring res, void* iter, void* arg );
BOOL callback_free_clients( const bstring key, void* iter, void* arg );
void* callback_remove_clients( const bstring res, void* iter, void* arg );
BOOL callback_free_channels( const bstring key, void* iter, void* arg );
BOOL callback_free_empty_channels( const bstring key, void* iter, void* arg );
BOOL callback_free_mobiles( const bstring key, void* iter, void* arg );
#ifdef USE_CHUNKS
BOOL callback_free_chunkers( const bstring key, void* iter, void* arg );
BOOL callback_free_finished_chunkers(
   const bstring key, void* iter, void* arg
);
BOOL callback_free_finished_unchunkers(
   const bstring key, void* iter, void* arg
);
#endif /* USE_CHUNKS */
BOOL callback_free_commands( const bstring res, void* iter, void* arg );
BOOL callback_free_generic( const bstring res, void* iter, void* arg );
BOOL callback_free_controls( const bstring res, void* iter, void* arg );
BOOL callback_free_strings( const bstring res, void* iter, void* arg );
BOOL callback_free_backlog( const bstring res, void* iter, void* arg );
BOOL callback_free_graphics( const bstring res, void* iter, void* arg );
BOOL callback_free_windows( const bstring res, void* iter, void* arg );
BOOL callback_free_ani_defs( const bstring key, void* iter, void* arg );
BOOL callback_free_spawners( const bstring res, void* iter, void* arg );
VECTOR_SORT_ORDER callback_sort_chunker_tracks( void* a, void* b );

#ifdef CALLBACKS_C
SCAFFOLD_MODULE( "callback.c" );
#endif /* CALLBACKS_C */

#endif /* CALLBACKS_H */
