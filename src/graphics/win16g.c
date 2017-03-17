
#define GRAPHICS_C
#include "../graphics.h"

void graphics_screen_init(
   GRAPHICS* g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h,
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

void graphics_screen_scroll(
   GRAPHICS* g, SCAFFOLD_SIZE offset_x, SCAFFOLD_SIZE offset_y
) {
}

void graphics_set_font( GRAPHICS* g, bstring name ) {
}

void graphics_set_color( GRAPHICS* g, GRAPHICS_COLOR color ) {
}

void graphics_set_color_ex(
   GRAPHICS* gr, uint8_t r, uint8_t g, uint8_t b, uint8_t a
) {
}

void graphics_set_image_path( GRAPHICS* g, const bstring path ) {
}

void graphics_set_image_data(
   GRAPHICS* g, const BYTE* data, SCAFFOLD_SIZE length
) {
}

BYTE* graphics_export_image_data( GRAPHICS* g, SCAFFOLD_SIZE* out_len ) {
}

void graphics_draw_text(
   GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, GRAPHICS_TEXT_ALIGN align,
   const bstring text
) {
}

void graphics_draw_rect(
   GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE w, SCAFFOLD_SIZE h
) {
}

void graphics_measure_text( GRAPHICS* g, GRAPHICS_RECT* r, const bstring text ) {
}

void graphics_transition( GRAPHICS* g, GRAPHICS_TRANSIT_FX fx ) {
}

void graphics_scale( GRAPHICS* g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h ) {
}

void graphics_blit(
   GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE s_w, SCAFFOLD_SIZE s_h,
   const GRAPHICS* src
) {
}

void graphics_blit_partial(
   GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE s_x,
   SCAFFOLD_SIZE s_y, SCAFFOLD_SIZE s_w, SCAFFOLD_SIZE s_h, const GRAPHICS* src
) {
}

void graphics_sleep( uint16_t milliseconds ) {
}

void graphics_colors_to_surface(
   GRAPHICS* g, GRAPHICS_COLOR* colors, SCAFFOLD_SIZE colors_sz
) {
}

void graphics_wait_for_fps_timer() {
}

