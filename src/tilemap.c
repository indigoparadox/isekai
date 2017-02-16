
#include "tilemap.h"

#include <memory.h>

void tilemap_init( TILEMAP* t ) {
    memset( t, '\0', sizeof( TILEMAP ) );
}

void tilemap_iterate_screen_row(
    TILEMAP* t, uint32_t x, uint32_t y, uint32_t screen_w, uint32_t screen_h,
    void (*callback)( TILEMAP* t, uint32_t x, uint32_t y, TILEMAP_TILE e )
) {

}
