
#define GRAPHICS_C
#include "../graphics.h"

void graphics_screen_new(
   GRAPHICS** g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h,
   SCAFFOLD_SIZE vw, SCAFFOLD_SIZE vh, int32_t arg1, void* arg2
) {
}

void graphics_surface_cleanup( GRAPHICS* g ) {
}

void graphics_surface_init( GRAPHICS* g, GFX_COORD_PIXEL w, GFX_COORD_PIXEL h ) {
}

void graphics_flip_screen( GRAPHICS* g ) {
}

void graphics_shutdown( GRAPHICS* g ) {
}

void graphics_set_image_path( GRAPHICS* g, const bstring path ) {
}

void graphics_set_image_data( GRAPHICS* g, const BYTE* data,
                              SCAFFOLD_SIZE length ) {
}

BYTE* graphics_export_image_data( GRAPHICS* g, SCAFFOLD_SIZE* out_len ) {
   return NULL;
}

void graphics_draw_rect(
   GRAPHICS* g, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
   GFX_COORD_PIXEL w, GFX_COORD_PIXEL h, GRAPHICS_COLOR color, BOOL filled
) {
}

void graphics_draw_char(
   GRAPHICS* g, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
   GRAPHICS_COLOR color, GRAPHICS_FONT_SIZE size, char c
) {
}

void graphics_transition( GRAPHICS* g, GRAPHICS_TRANSIT_FX fx ) {
}

void graphics_scale( GRAPHICS* g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h ) {
}

void graphics_blit_partial(
   GRAPHICS* g, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
   GFX_COORD_PIXEL s_x, GFX_COORD_PIXEL s_y,
   GFX_COORD_PIXEL s_w, GFX_COORD_PIXEL s_h, const GRAPHICS* src
) {
}

void graphics_sleep( uint16_t milliseconds ) {
}

uint32_t graphics_get_ticks() {
   return 0;
}
