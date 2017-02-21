
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

#ifdef INIT_ZEROES
    memset( d, '\0', sizeof( GAMEDATA ) );
#endif /* INIT_ZEROES */

    tilemap_init( &(d->tmap) );

    scaffold_print_info( "Loading data for channel: %s\n", bdata( name ) );
    mapdata_filename = bstrcpy( name );
    scaffold_check_null( mapdata_filename );
    bdelete( mapdata_filename, 0, 1 );
    mapdata_path = bformat( "./%s.tmx", bdata( mapdata_filename ) );
    scaffold_check_null( mapdata_path );
    scaffold_print_info( "Loading for data in: %s\n", bdata( mapdata_path ) );
    tilemap_load_file( &(d->tmap), mapdata_path );

cleanup:
    bdestroy( mapdata_path );
    bdestroy( mapdata_filename );
    return;
}

void gamedata_cleanup( GAMEDATA* d ) {
    tilemap_cleanup( &(d->tmap) );

    /* Don't cleanup UI since other things probably use that. */
}

void gamedata_init_client( GAMEDATA* d, UI* ui, const bstring name ) {
#ifdef INIT_ZEROES
    memset( d, '\0', sizeof( GAMEDATA ) );
#endif /* INIT_ZEROES */
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
