#ifndef GAMEDATA_H
#define GAMEDATA_H

#include "tilemap.h"
#include "hashmap.h"

typedef struct _UI UI;

typedef struct {

} GAMEDATA_UPDATE;

typedef struct _GAMEDATA {
   TILEMAP tmap;
   UI* ui;
   map_t incoming_chunkers;
} GAMEDATA;

#define gamedata_new_server( d ) \
    d = (GAMEDATA*)calloc( 1, sizeof( GAMEDATA ) ); \
    scaffold_check_null( d ); \
    gamedata_init_server( d );

#define gamedata_new_client( d ) \
    d = (GAMEDATA*)calloc( 1, sizeof( GAMEDATA ) ); \
    scaffold_check_null( d ); \
    gamedata_init_client( d );

void gamedata_init_server( GAMEDATA* g, const bstring name );
void gamedata_init_client( GAMEDATA* g, UI* ui, const bstring name );
void gamedata_cleanup( GAMEDATA* d );
void gamedata_update_server(
   GAMEDATA* d, CLIENT* c, const struct bstrList* args,
   bstring* reply_c, bstring* reply_l
);
void gamedata_react_client(
   GAMEDATA* d, CLIENT* c, const struct bstrList* args, bstring* reply
);
void gamedata_update_client( GAMEDATA* d, CLIENT* c );

#endif /* GAMEDATA_H */
