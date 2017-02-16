
#include "gamedata.h"

#include <memory.h>

void gamedata_init( GAMEDATA* g, const bstring name ) {
    memset( g, '\0', sizeof( GAMEDATA ) );
}

void gamedata_update(
    GAMEDATA* g, CLIENT* c, const struct bstrList* args, bstring* reply_c,
    bstring* reply_l
) {

}
