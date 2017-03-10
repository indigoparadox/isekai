#include "../graphics.h"

#include <ncurses.h>

#define GFX_TEXT_DIVISOR_W 8
#define GFX_TEXT_DIVISOR_H 20

static void graphics_translate( gu* x, gu* y ) {
   *x /= GFX_TEXT_DIVISOR_W;
   *y /= GFX_TEXT_DIVISOR_H;
}

static void graphics_surface_cleanup( const struct REF *ref ) {
   GRAPHICS* g = scaffold_container_of( ref, struct _GRAPHICS, refcount );
   if( NULL != g->surface ) {
      g->surface = NULL;
   }
   if( NULL == g->palette ) {
      free( g->palette );
      g->palette = NULL;
   }
   /* TODO: Free surface. */
}

void graphics_screen_init( GRAPHICS* g, gu w, gu h ) {
   ref_init( &(g->refcount), graphics_surface_cleanup );
   initscr();
   noecho();
   raw();
}

void graphics_surface_init( GRAPHICS* g, gu x, gu y, gu w, gu h ) {
   ref_init( &(g->refcount), graphics_surface_cleanup );
   graphics_translate( &x, &y );
   g->surface = newwin( w, h, 0, 0 );
   g->w = w;
   g->h = h;
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

void graphics_draw_text( GRAPHICS* g, gu x, gu y, const bstring text ) {
   graphics_translate( &x, &y );
   mvprintw( y, x, "%s", bdata( text ) );
}

void graphics_draw_rect( GRAPHICS* g, gu x, gu y, gu w, gu h ) {

}

void graphics_measure_text( GRAPHICS* g, GRAPHICS_RECT* r,
                            const bstring text ) {
   r->w = blength( text ) * GFX_TEXT_DIVISOR_W;
   r->h = GFX_TEXT_DIVISOR_H;
}

void graphics_transition( GRAPHICS* g, GRAPHICS_TRANSIT_FX fx ) {

}

void graphics_scale( GRAPHICS* g, gu w, gu h ) {

}

void graphics_blit( GRAPHICS* g, gu x, gu y, gu s_w, gu s_h,
                    const GRAPHICS* src ) {

}

void graphics_blit_partial(
   GRAPHICS* g, gu x, gu y, gu s_x, gu s_y, gu s_w, gu s_h, const GRAPHICS* src
) {
}

void graphics_sleep( uint16_t milliseconds ) {
}
