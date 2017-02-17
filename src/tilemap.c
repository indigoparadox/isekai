
#include "tilemap.h"

#include <memory.h>
#include <string.h>

void tilemap_init( TILEMAP* t ) {
    memset( t, '\0', sizeof( TILEMAP ) );
}

void tilemap_load( TILEMAP* t, bstring filename ) {
    FILE* tmfile = NULL;
    uint8_t* tmdata = NULL;
    uint32_t datasize = 0;

    tmfile = fopen( bdata( filename ), "rb" );
    scaffold_check_null( tmfile );

    /* Allocate enough space to hold the map. */
    fseek( tmfile, 0, SEEK_END );
    datasize = ftell( tmfile );
    tmdata = calloc( datasize, sizeof( uint8_t ) );
    fseek( tmfile, 0, SEEK_SET );
    fread( tmdata, sizeof( uint8_t ), datasize, tmfile );
    fclose( tmfile );
    tmfile = NULL;

cleanup:
    return;
}

void tilemap_iterate_screen_row(
    TILEMAP* t, uint32_t x, uint32_t y, uint32_t screen_w, uint32_t screen_h,
    void (*callback)( TILEMAP* t, uint32_t x, uint32_t y, TILEMAP_TILE e )
) {

}
