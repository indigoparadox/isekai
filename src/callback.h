
#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "scaffold.h"

/* Hashmap Callbacks */
#ifdef USE_CHUNKS
void* callback_send_chunkers_l( bstring idx, void* iter, void* arg );
void* callback_proc_client_chunkers( bstring idx, void* iter, void* arg );
void* callback_proc_chunkers( bstring idx, void* iter, void* arg );
VBOOL callback_free_chunkers( bstring idx, void* iter, void* arg );
VBOOL callback_free_finished_chunkers(
   bstring idx, void* iter, void* arg );
#endif /* USE_CHUNKS */
#ifdef USE_VM
void* callback_proc_channel_vms( bstring idx, void* iter, void* arg );
#endif /* USE_VM */
void* callback_concat_clients( bstring idx, void* iter, void* arg );
void* callback_search_clients_r( bstring idx, void* iter, void* arg );
void* callback_download_tileset( bstring idx, void* iter, void* arg );
void* callback_load_local_tilesets(
   bstring idx, void* iter, void* arg );
void* callback_search_tileset_img_gid(
   bstring idx, void* iter, void* arg );
void* callback_search_channels_tilemap_img_name(
   bstring idx, void* iter, void* arg );
void* callback_search_graphics( bstring idx, void* iter, void* arg );
void* callback_send_mobs_to_channel( bstring idx, void* iter, void* arg );
void* callback_send_updates_to_client( bstring idx, void* iter, void* arg );
void* callback_attach_channel_mob_sprites(
   bstring idx, void* iter, void* arg );
void* callback_proc_tileset_img_gs( bstring idx, void* iter, void* arg );
void* callback_proc_server_spawners( bstring idx, void* iter, void* arg );
void* callback_stop_clients( bstring idx, void* iter, void* arg );
VBOOL callback_free_channels( bstring idx, void* iter, void* arg );
VBOOL callback_free_empty_channels( bstring idx, void* iter, void* arg );
VBOOL callback_free_tilesets( bstring idx, void* iter, void* arg );
VBOOL callback_free_catalogs( bstring idx, void* iter, void* arg );
VBOOL callback_free_controls( bstring idx, void* iter, void* arg );
VBOOL callback_free_graphics( bstring idx, void* iter, void* arg );
VBOOL callback_free_ani_defs( bstring idx, void* iter, void* arg );
VBOOL callback_h_free_strings( bstring idx, void* iter, void* arg );
VBOOL callback_h_free_clients( bstring idx, void* iter, void* arg );
void* callback_search_tilesets_name( bstring idx, void* iter, void* arg );

/* Vector Callbacks */
VECTOR_SORT_ORDER callback_sort_chunker_tracks( void* a, void* b );
#ifdef USE_VM
void* callback_proc_mobile_vms( size_t idx, void* iter, void* arg );
#endif /* USE_VM */
#ifdef DEBUG
void* callback_assert_windows( size_t idx, void* iter, void* arg );
#endif /* DEBUG */
void* callback_concat_strings( size_t idx, void* iter, void* arg );
void* callback_search_bstring_i(
   size_t idx, void* iter, void* arg );
void* callback_search_spawners( size_t idx, void* iter, void* arg );
void* callback_load_spawner_catalogs(
   size_t idx, void* iter, void* arg );
void* callback_search_mobs_by_pos( size_t idx, void* iter, void* arg );
void* callback_search_windows( size_t idx, void* iter, void* arg );
void* callback_search_tilesets_small(
   size_t idx, void* iter, void* arg );
void* callback_search_item_caches(
   size_t idx, void* iter, void* arg );
void* callback_search_items(
   size_t idx, void* iter, void* arg );
void* callback_search_item_type(
   size_t idx, void* iter, void* arg );
void* callback_search_tilesets_img_name(
   size_t idx, void* iter, void* arg );
void* callback_send_mobs_to_client( size_t idx, void* iter, void* arg );
void* callback_attach_mob_sprites(
   size_t idx, void* iter, void* arg );
void* callback_get_tile_stack_l( size_t idx, void* iter, void* arg );
VBOOL callback_proc_client_delayed_files(
   size_t idx, void* iter, void* arg );
void* callback_proc_channel_spawners(
   size_t idx, void* iter, void* arg );
void* callback_search_tilesets_gid( size_t idx, void* iter, void* arg );
VBOOL callback_free_mobiles( size_t idx, void* iter, void* arg );
VBOOL callback_free_sprites( size_t idx, void* iter, void* arg );
VBOOL callback_free_item_cache_items(
   size_t idx, void* iter, void* arg );
VBOOL callback_free_item_caches(
   size_t idx, void* iter, void* arg );
VBOOL callback_free_generic( size_t idx, void* iter, void* arg );
VBOOL callback_v_free_strings( size_t idx, void* iter, void* arg );
VBOOL callback_free_backlog( size_t idx, void* iter, void* arg );
VBOOL callback_free_windows( size_t idx, void* iter, void* arg );
VBOOL callback_free_spawners( size_t idx, void* iter, void* arg );
VBOOL callback_v_free_clients( size_t idx, void* iter, void* arg );

#endif /* CALLBACKS_H */
