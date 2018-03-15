
#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "vector.h"
#include "hashmap.h"

void* callback_ingest_commands( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_concat_clients( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_concat_strings( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_search_bstring_i(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
);
void* callback_search_clients( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_search_clients_r( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_search_clients_l( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_search_spawners( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_send_clients( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_search_channels( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_download_tileset( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_load_local_tilesets(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
);
void* callback_load_spawner_catalogs(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
);
void* callback_search_mobs_by_pos( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_search_windows( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_search_tilesets_small(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
);
void* callback_search_item_caches(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
);
void* callback_search_items(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
);
void* callback_search_item_type(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
);
void* callback_search_tileset_img_gid( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_search_tilesets_img_name( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_search_channels_tilemap_img_name( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_search_channels_tileset_path( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_search_tilesets_name( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_search_graphics( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_search_servefiles( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
#ifdef USE_CHUNKS
void* callback_send_chunkers_l( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
#endif /* USE_CHUNKS */
void* callback_send_mobs_to_client( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_send_mobs_to_channel( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_send_updates_to_client( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_parse_mobs( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_parse_mob_channels( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_attach_mob_sprites(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
);
void* callback_attach_channel_mob_sprites(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
);
BOOL callback_send_list_to_client( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_get_tile_stack_l( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_get_tile_blocker( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
#ifdef USE_CHUNKS
void* callback_proc_client_chunkers( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_proc_chunkers( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
#endif /* USE_CHUNKS */
BOOL callback_proc_client_delayed_files(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
);
#ifdef USE_VM
void* callback_proc_mobile_vms( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_proc_channel_vms( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
#endif /* USE_VM */
void* callback_proc_tileset_img_gs( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_proc_tileset_imgs( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_proc_channel_spawners(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
);
void* callback_proc_server_spawners( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_search_tilesets_gid( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_draw_mobiles( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_stop_clients( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
BOOL callback_free_clients( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void* callback_remove_clients( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
BOOL callback_free_channels( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
BOOL callback_free_empty_channels( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
BOOL callback_free_mobiles( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
BOOL callback_free_tilesets( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
BOOL callback_free_sprites( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
BOOL callback_free_catalogs( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
BOOL callback_free_item_cache_items(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
);
BOOL callback_free_item_caches(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
);
#ifdef USE_CHUNKS
BOOL callback_free_chunkers( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
BOOL callback_free_finished_chunkers(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
);
BOOL callback_free_finished_unchunkers(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
);
#endif /* USE_CHUNKS */
BOOL callback_free_commands( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
BOOL callback_free_generic( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
BOOL callback_free_controls( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
BOOL callback_free_strings( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
BOOL callback_free_backlog( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
BOOL callback_free_graphics( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
BOOL callback_free_windows( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
#ifdef DEBUG
void* callback_assert_windows( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
#endif /* DEBUG */
BOOL callback_free_ani_defs( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
BOOL callback_free_spawners( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
VECTOR_SORT_ORDER callback_sort_chunker_tracks( void* a, void* b );

#ifdef CALLBACKS_C
SCAFFOLD_MODULE( "callback.c" );
#endif /* CALLBACKS_C */

#endif /* CALLBACKS_H */
