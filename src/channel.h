#ifndef CHANNEL_H
#define CHANNEL_H

#include "bstrlib/bstrlib.h"
#include "vector.h"
#include "client.h"
#include "hashmap.h"
#include "tilemap.h"

#define CHANNEL_SENTINAL 888

#ifdef USE_TINYPY
struct tp_vm;
#endif /* USE_TINYPY */
#ifdef USE_DUKTAPE
struct duk_hthread;
#endif /* USE_DUKTAPE */

struct CHANNEL {
   struct REF refcount;
   uint16_t sentinal;
   bstring name;
   bstring topic;
   struct HASHMAP clients;
   struct CLIENT* client_or_server;
   struct VECTOR mobiles;
   struct TILEMAP tilemap;
   struct VM* vm;
   struct VM_CADDY* vm_caddy;
   BOOL vm_started;
   struct HASHMAP vm_globals;
#ifdef USE_TINYPY
   struct tp_vm* vm;
   int vm_cur;
   int vm_step_ret;
#endif /* USE_TINYPY */
};

struct CHANNEL_CLIENT {
   struct CHANNEL* l;
   struct CLIENT* c;
};

#define channel_new( l, name, local_graphics, server ) \
    scaffold_check_null( name ); \
    l = (struct CHANNEL*)calloc( 1, sizeof( struct CHANNEL ) ); \
    scaffold_check_null( l ); \
    channel_init( l, name, local_graphics, server );

struct MOBILE;

void channel_init(
   struct CHANNEL* l, const bstring name, BOOL local_graphics,
   struct CLIENT* server
);
void channel_free( struct CHANNEL* l );
struct CLIENT* channel_client_present( struct CHANNEL* l, struct CLIENT* c );
void channel_add_client( struct CHANNEL* l, struct CLIENT* c, BOOL spawn );
void channel_set_mobile(
   struct CHANNEL* l, uint8_t serial,
   const bstring mob_id, const bstring sprites_filename,
   const bstring nick, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   struct CLIENT* local_c
);
void channel_remove_client( struct CHANNEL* l, struct CLIENT* c );
struct CLIENT* channel_get_client_by_name( struct CHANNEL* l, bstring nick );
struct bstrList* channel_list_clients( struct CHANNEL* l );
void channel_add_mobile( struct CHANNEL* l, struct MOBILE* o );
void channel_remove_mobile( struct CHANNEL* l, SCAFFOLD_SIZE serial );
void channel_load_tilemap( struct CHANNEL* l );
void channel_speak( struct CHANNEL* l, const bstring nick, const bstring msg );
void* channel_backlog_iter( struct CHANNEL* l, vector_search_cb cb, void* arg );
void channel_vm_start( struct CHANNEL* l, bstring code );
void channel_vm_step( struct CHANNEL* l );
void channel_vm_end( struct CHANNEL* l );
BOOL channel_vm_can_step( struct CHANNEL* l );

#ifdef CHANNEL_C
SCAFFOLD_MODULE( "channel.c" );
static struct tagbstring str_player = bsStatic( "player" );
#endif /* CHANNEL_C */

#endif /* CHANNEL_H */
