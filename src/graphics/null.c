
#define GRAPHICS_C
#include "../graphics.h"

void graphics_screen_new(
   GRAPHICS** g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h,
   SCAFFOLD_SIZE vw, SCAFFOLD_SIZE vh, int32_t arg1, void* arg2
) {
}

void graphics_surface_init( GRAPHICS* g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h ) {
}

void graphics_surface_free( GRAPHICS* g ) {
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
   GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE w, SCAFFOLD_SIZE h, GRAPHICS_COLOR color
) {
}

void graphics_draw_char(
   GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, GRAPHICS_COLOR color,
   GRAPHICS_FONT_SIZE size, char c
) {
}

void graphics_transition( GRAPHICS* g, GRAPHICS_TRANSIT_FX fx ) {
}

void graphics_scale( GRAPHICS* g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h ) {
}

void graphics_blit( GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE s_w, SCAFFOLD_SIZE s_h,
                    const GRAPHICS* src ) {
}

void graphics_blit_partial(
   GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE s_x, SCAFFOLD_SIZE s_y, SCAFFOLD_SIZE s_w, SCAFFOLD_SIZE s_h, const GRAPHICS* src
) {
}

void graphics_sleep( uint16_t milliseconds ) {
}

uint32_t graphics_get_ticks() {
   return 0;
}
