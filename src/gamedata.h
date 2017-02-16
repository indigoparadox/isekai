
#ifndef GAMEDATA_H
#define GAMEDATA_H

#include "tilemap.h"

typedef struct {

} GAMEDATA_UPDATE;

typedef struct {
    TILEMAP tmap;
} GAMEDATA;

void gamedata_init( GAMEDATA* g, const bstring name );
void gamedata_update(
    GAMEDATA* g, CLIENT* c, const struct bstrList* args, bstring* reply_c,
    bstring* reply_l
);

#endif /* GAMEDATA_H */
