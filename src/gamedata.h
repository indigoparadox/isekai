
#ifndef GAMEDATA_H
#define GAMEDATA_H

#include "tilemap.h"

typedef struct {

} GAMEDATA_UPDATE;

typedef struct _GAMEDATA {
    TILEMAP tmap;
} GAMEDATA;

void gamedata_init( GAMEDATA* g, const bstring name );
void gamedata_update_server(
    GAMEDATA* d, CLIENT* c, const struct bstrList* args,
    bstring* reply_c, bstring* reply_l
);
void gamedata_update_client(
    GAMEDATA* d, CLIENT* c, const struct bstrList* args, bstring* reply
);

#endif /* GAMEDATA_H */
