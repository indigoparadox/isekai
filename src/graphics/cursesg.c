
#define GRAPHICS_C
#include "../graphics.h"

#include <ncurses.h>

#define GFX_TEXT_DIVISOR_W 8
#define GFX_TEXT_DIVISOR_H 20

static void graphics_translate( SCAFFOLD_SIZE* x, SCAFFOLD_SIZE* y ) {
   *x /= GFX_TEXT_DIVISOR_W;
   *y /= GFX_TEXT_DIVISOR_H;
}

static void graphics_surface_cleanup( const struct REF *ref ) {
   GRAPHICS* g = scaffold_container_of( ref, struct GRAPHICS, refcount );
   if( NULL != g->surface ) {
      g->surface = NULL;
   }
   if( NULL == g->palette ) {
      mem_free( g->palette );
      g->palette = NULL;
   }
   /* TODO: Free surface. */
}

void graphics_screen_new(
   GRAPHICS** g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h,
   SCAFFOLD_SIZE vw, SCAFFOLD_SIZE vh, int32_t arg1, void* arg2
) {
   ref_init( &((*g)->refcount), graphics_surface_cleanup );
   initscr();
   noecho();
   raw();
}

void graphics_surface_init( GRAPHICS* g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h ) {
   graphics_translate( &w, &h );
   g->surface = newwin( w, h, 0, 0 );
   g->w = w;
   g->h = h;
   g->fp_w = graphics_precise( g->w );
   g->fp_h = graphics_precise( g->h );
}

void graphics_surface_free( GRAPHICS* g ) {
}

void graphics_flip_screen( GRAPHICS* g ) {
   refresh();
}

void graphics_shutdown( GRAPHICS* g ) {
   endwin();
}

void graphics_set_font( GRAPHICS* g, const bstring name ) {
   /* NOOP */
}

void graphics_set_color( GRAPHICS* g, GRAPHICS_COLOR* color ) {

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

/*
void graphics_draw_text(
   GRAPHICS* g, SCAFFOLD_SIZE x_start, SCAFFOLD_SIZE y_start,
   GRAPHICS_TEXT_ALIGN align, GRAPHICS_COLOR color, GRAPHICS_FONT_SIZE size,
   const bstring text
) {
   graphics_translate( &x_start, &y_start );
   mvprintw( y_start, x_start, "%s", bdata( text ) );
}
*/

void graphics_draw_rect(
   GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE w, SCAFFOLD_SIZE h, GRAPHICS_COLOR color
) {
}

/*
void graphics_measure_text(
   GRAPHICS* g, GRAPHICS_RECT* r, GRAPHICS_FONT_SIZE size, const bstring text
) {
   r->w = blength( text ) * GFX_TEXT_DIVISOR_W;
   r->h = GFX_TEXT_DIVISOR_H;
}
*/

void graphics_transition( GRAPHICS* g, GRAPHICS_TRANSIT_FX fx ) {

}

void graphics_scale( GRAPHICS* g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h ) {

}

void graphics_blit( GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE s_w, SCAFFOLD_SIZE s_h,
                    const GRAPHICS* src ) {

}

void graphics_sleep( uint16_t milliseconds ) {
}

uint32_t graphics_get_ticks() {
   return clock();
}

void graphics_draw_char(
   GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, GRAPHICS_COLOR color,
   GRAPHICS_FONT_SIZE size, char c
) {
}

void graphics_set_window_title( GRAPHICS* g, bstring title, void* icon ) {
}
