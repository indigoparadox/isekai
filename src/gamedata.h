#ifndef GAMEDATA_H
#define GAMEDATA_H

#include "tilemap.h"
#include "hashmap.h"

#include <stdlib.h>

struct UI;
struct MOBILE;
struct CHUNKER_PROGRESS;

struct GAMEDATA_UPDATE {
   struct MOBILE* mobile;
};

struct GAMEDATA {
   struct TILEMAP tmap;
   struct HASHMAP incoming_chunkers;
   uint8_t* incoming_buffer;
   size_t incoming_buffer_len;
   struct VECTOR mobiles;
   struct HASHMAP mob_sprites;
};

#include "mobile.h"

#define gamedata_new_server( d ) \
    d = (struct GAMEDATA*)calloc( 1, sizeof( struct GAMEDATA ) ); \
    scaffold_check_null( d ); \
    gamedata_init_server( d );

#define gamedata_new_client( d ) \
    d = (struct GAMEDATA*)calloc( 1, sizeof( struct GAMEDATA ) ); \
    scaffold_check_null( d ); \
    gamedata_init_client( d );

void gamedata_init_server( struct GAMEDATA* g, const bstring name );
void gamedata_init_client( struct GAMEDATA* g );
void gamedata_cleanup( struct GAMEDATA* d );
void gamedata_update_server(
   struct GAMEDATA* d, struct CLIENT* c, const struct bstrList* args,
   bstring* reply_c, bstring* reply_l
);
void gamedata_react_client(
   struct GAMEDATA* d, struct CLIENT* c, const struct bstrList* args, bstring* reply
);
void gamedata_poll_input( struct GAMEDATA* d, struct CLIENT* c );
void gamedata_update_client( struct CLIENT* c, GRAPHICS* g, struct UI* ui );
void gamedata_add_mobile( struct GAMEDATA* s, struct MOBILE* o );
void gamedata_remove_mobile( struct GAMEDATA* s, size_t serial );
void gamedata_process_data_block(
   struct GAMEDATA* d, struct CLIENT* c, struct CHUNKER_PROGRESS* progress
);
void gamedata_process_finished_chunker(
   struct GAMEDATA* d, struct CLIENT* c, struct CHUNKER* h
);

#endif /* GAMEDATA_H */
