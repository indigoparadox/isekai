#ifndef CHANNEL_H
#define CHANNEL_H

#include "libvcol.h"
#include "client.h"
#include "tilemap.h"

#define CHANNEL_SENTINAL 888

struct CHANNEL {
   struct REF refcount;
   uint16_t sentinal;
   bstring name;
   bstring topic;
   bstring mode;
   struct HASHMAP* clients;
   struct HASHMAP* client_mode_data;
   struct CLIENT* client_or_server;
   struct VECTOR* mobiles;
   struct VECTOR* mobile_mode_data;
   struct TILEMAP* tilemap;
#ifdef USE_VM
   struct VM_CADDY* vm_caddy;
#endif /* USE_VM */
   bstring error;
};

#define channel_new( l, name, local_graphics, server, mode ) \
    lgc_null( name ); \
    l = mem_alloc( 1, struct CHANNEL ); \
    lgc_null( l ); \
    channel_init( l, name, local_graphics, server, mode );

struct MOBILE;

void channel_init(
   struct CHANNEL* l, const bstring name, bool local_graphics,
   struct CLIENT* server, bstring mode
);
void channel_free( struct CHANNEL* l );
struct CLIENT* channel_client_present( struct CHANNEL* l, struct CLIENT* c );
void channel_add_client( struct CHANNEL* l, struct CLIENT* c, bool spawn );
void channel_set_mobile(
   struct CHANNEL* l, uint8_t serial,
   const bstring mob_id, const bstring sprites_filename,
   const bstring nick, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y
);
void channel_remove_client( struct CHANNEL* l, struct CLIENT* c );
struct CLIENT* channel_get_client_by_name(
   const struct CHANNEL* l, bstring nick );
struct bstrList* channel_list_clients( struct CHANNEL* l );
void channel_add_mobile( struct CHANNEL* l, struct MOBILE* o );
void channel_remove_mobile( struct CHANNEL* l, SCAFFOLD_SIZE serial );
void channel_load_tilemap( struct CHANNEL* l );
void channel_speak( struct CHANNEL* l, const bstring nick, const bstring msg );
void* channel_backlog_iter( struct CHANNEL* l, vector_iter_cb cb, void* arg );
bool channel_has_error( const struct CHANNEL* l );
void channel_set_error( struct CHANNEL* l, const char* error );
bool channel_is_loaded( struct CHANNEL* l );
bstring channel_get_name( const struct CHANNEL* l );
struct TILEMAP* channel_get_tilemap( const struct CHANNEL* l );
size_t channel_get_clients_count( const struct CHANNEL* l );
struct MOBILE* channel_search_mobiles(
   const struct CHANNEL* l, struct TILEMAP_POSITION* pos
);
void channel_set_mode( struct CHANNEL* l, const bstring mode );

#ifdef CHANNEL_C
static struct tagbstring str_player = bsStatic( "player" );
#endif /* CHANNEL_C */

#endif /* CHANNEL_H */
