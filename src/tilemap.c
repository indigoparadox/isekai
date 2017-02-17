
#include "tilemap.h"

#include "json.h"

#include <memory.h>
#include <string.h>

void tilemap_init( TILEMAP* t ) {
    memset( t, '\0', sizeof( TILEMAP ) );
}

void tilemap_load_file( TILEMAP* t, bstring filename ) {
    FILE* tmfile = NULL;
    uint8_t* tmdata = NULL;
    uint32_t datasize = 0;

    tmfile = fopen( bdata( filename ), "rb" );
    scaffold_check_null( tmfile );

    /* Allocate enough space to hold the map. */
    fseek( tmfile, 0, SEEK_END );
    datasize = ftell( tmfile );
    tmdata = calloc( datasize, sizeof( uint8_t ) + 1 ); /* +1 for term. */
    fseek( tmfile, 0, SEEK_SET );

    /* Read and close the map. */
    fread( tmdata, sizeof( uint8_t ), datasize, tmfile );
    fclose( tmfile );
    tmfile = NULL;

    JSON map_json;
    calloc( &map_json, sizeof( JSON ) );
    json_parse_object( &map_json, tmdata );
    json_walk( &map_json );

cleanup:
    return;
}

void tilemap_iterate_screen_row(
    TILEMAP* t, uint32_t x, uint32_t y, uint32_t screen_w, uint32_t screen_h,
    void (*callback)( TILEMAP* t, uint32_t x, uint32_t y, TILEMAP_TILE e )
) {

}
