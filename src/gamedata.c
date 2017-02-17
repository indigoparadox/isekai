
#include "gamedata.h"

#include <memory.h>

#include "channel.h"
#include "graphics.h"
#include "ui.h"

static inline CHANNEL* gamedata_get_channel( GAMEDATA* d ) {
    return (CHANNEL*)d;
}

void gamedata_init_server( GAMEDATA* d, const bstring name ) {
    bstring mapdata_path = NULL;
    bstring mapdata_filename = NULL;

    memset( d, '\0', sizeof( GAMEDATA ) );

    scaffold_print_info( "Loading data for channel: %s\n", bdata( name ) );
    mapdata_filename = bstrcpy( name );
    bdelete( mapdata_filename, 0, 1 );
    mapdata_path = bformat( "./%s.json", bdata( mapdata_filename ) );
    scaffold_print_info( "Loading for data in: %s\n", bdata( mapdata_path ) );
    tilemap_load_file( &(d->tmap), mapdata_path );
}

void gamedata_init_client( GAMEDATA* d, UI* ui, const bstring name ) {
    memset( d, '\0', sizeof( GAMEDATA ) );
    d->ui = ui;
}

void gamedata_update_server(
    GAMEDATA* d, CLIENT* c, const struct bstrList* args,
    bstring* reply_c, bstring* reply_l
) {
    CHANNEL* l = gamedata_get_channel( d );


}

void gamedata_update_client(
    GAMEDATA* d, CLIENT* c, const struct bstrList* args, bstring* reply
) {
    UI* ui = d->ui;
    GRAPHICS* g = ui->screen;


}
