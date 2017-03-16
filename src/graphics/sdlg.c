
#define GRAPHICS_C
#include "../graphics.h"

#include <SDL/SDL.h>
#ifdef USE_SDL_IMAGE
#include <SDL/SDL_image.h>
#endif /* USE_SDL_IMAGE */
#include "../font.h"

static
SDL_Color graphics_stock_colors[16] = {   /*  r,   g,   b   */
   /* GRAPHICS_COLOR_TRANSPARENT =  0, */ {   0,   0,   0, 0 },
   /* GRAPHICS_COLOR_DARK_BLUE   =  1, */ {   0,   0, 128, 0 },
   /* GRAPHICS_COLOR_DARK_GREEN  =  2, */ {   0, 128,   0, 0 },
   /* GRAPHICS_COLOR_DARK_CYAN   =  3, */ {   0, 128, 128, 0 },
   /* GRAPHICS_COLOR_DARK_RED    =  4, */ { 128,   0,   0, 0 },
   /* GRAPHICS_COLOR_PURPLE      =  5, */ { 128,   0, 128, 0 },
   /* GRAPHICS_COLOR_BROWN       =  6, */ { 128, 128,   0, 0 },
   /* GRAPHICS_COLOR_GRAY        =  7, */ { 128, 128, 128, 0 },
   /* GRAPHICS_COLOR_DARK_GRAY   =  8, */ {   0,   0,   0, 0 },
   /* GRAPHICS_COLOR_BLUE        =  9, */ {   0,   0, 255, 0 },
   /* GRAPHICS_COLOR_GREEN       = 10, */ {   0, 255,   0, 0 },
   /* GRAPHICS_COLOR_CYAN        = 11, */ {   0, 255, 255, 0 },
   /* GRAPHICS_COLOR_RED         = 12, */ { 255,   0,   0, 0 },
                                          { 255,   0, 255, 0 },
   /* GRAPHICS_COLOR_YELLOW      = 13, */ { 255, 255,   0, 0 },
   /* GRAPHICS_COLOR_WHITE       = 14  */ { 255, 255, 255, 0 }
};

static void SDL_PutPixel( SDL_Surface *surface, int x, int y, uint32_t pixel ) {
    int bpp = surface->format->BytesPerPixel;
    uint8_t* p = (uint8_t*)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(uint16_t*)p = pixel;
        break;

    case 3:
        if( SDL_BYTEORDER == SDL_BIG_ENDIAN ) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(uint32_t*)p = pixel;
        break;
    }
}

static void graphics_surface_cleanup( const struct REF *ref ) {
   GRAPHICS* g = scaffold_container_of( ref, GRAPHICS, refcount );
   if( NULL != g->surface ) {
      SDL_FreeSurface( g->surface );
      g->surface = NULL;
   }
   if( NULL == g->palette ) {
      scaffold_free( g->palette );
      g->palette = NULL;
   }
   /* TODO: Free surface. */
}

void graphics_screen_new(
   GRAPHICS** g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h,
   SCAFFOLD_SIZE vw, SCAFFOLD_SIZE vh, int32_t arg1, void* arg2
) {

   graphics_setup();

   (*g) = scaffold_alloc( 1, GRAPHICS );
   SDL_Init( SDL_INIT_EVERYTHING );
   (*g)->surface = SDL_SetVideoMode(
      w, h, 0,
#ifdef USE_SDL_IMAGE
      SDL_HWSURFACE
#else
      SDL_SWSURFACE
#endif /* USE_SDL_IMAGE */
      | SDL_DOUBLEBUF
   );
   scaffold_check_null( (*g)->surface );
   (*g)->w = w;
   (*g)->h = h;
   (*g)->virtual_x = vw;
   (*g)->virtual_y = vh;
   (*g)->palette = NULL;
   (*g)->font = NULL;
cleanup:
   return;
}

void graphics_set_window_title( GRAPHICS* g, bstring title, void* icon ) {
   SDL_WM_SetCaption( bdata( title ), icon );
}

void graphics_surface_init( GRAPHICS* g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h ) {
   SDL_Surface* screen = NULL;

   screen = SDL_GetVideoSurface();

   ref_init( &(g->refcount), graphics_surface_cleanup );

   g->surface = SDL_CreateRGBSurface(
#ifdef USE_SDL_IMAGE
      SDL_HWSURFACE,
#else
      SDL_SWSURFACE,
#endif /* USE_SDL_IMAGE */
      w,
      h,
      screen->format->BitsPerPixel,
      screen->format->Rmask,
      screen->format->Gmask,
      screen->format->Bmask,
      screen->format->Amask
   );
   scaffold_check_null( g->surface );
   g->virtual_x = 0;
   g->virtual_y = 0;
   g->w = w;
   g->h = h;
cleanup:
   return;
}

void graphics_surface_free( GRAPHICS* g ) {
   if( NULL != g->surface ) {
      SDL_FreeSurface( g->surface );
   }
}

void graphics_flip_screen( GRAPHICS* g ) {
   SDL_Flip( g->surface );
}

void graphics_shutdown( GRAPHICS* g ) {
   SDL_Quit();
}

void graphics_screen_scroll(
   GRAPHICS* g, SCAFFOLD_SIZE offset_x, SCAFFOLD_SIZE offset_y
) {
}

void graphics_set_image_path( GRAPHICS* g, const bstring path ) {
}

void graphics_set_image_data(
   GRAPHICS* g, const BYTE* data, SCAFFOLD_SIZE length
) {
   struct GRAPHICS_BITMAP* bitmap = NULL;
   int sdl_ret,
      bytes_per_pixel;
   SCAFFOLD_SIZE_SIGNED surface_i = 0,
      y,
      i = 0;
   SDL_Surface* surface = NULL,
      * screen = NULL,
      * temp_surface = NULL;
   SDL_Color* pixel = NULL;
   SDL_PixelFormat* format = NULL;
   SDL_RWops* rwop;
   uint16_t temp;
   uint32_t color_key;
   BYTE* holder = NULL;

   screen = SDL_GetVideoSurface();
   scaffold_check_null( screen );

   if( NULL != g->surface ) {
      SDL_FreeSurface( g->surface );
      g->surface = NULL;
   }

#ifdef USE_SDL_IMAGE

   rwop = SDL_RWFromConstMem( data, length );

   temp_surface = IMG_LoadBMP_RW( rwop );
   scaffold_check_null( temp_surface );

   /* Setup transparency. */
   color_key = SDL_MapRGB( temp_surface->format, 0, 0, 0 );
   SDL_SetColorKey( temp_surface, SDL_RLEACCEL | SDL_SRCCOLORKEY, color_key );
   g->surface = SDL_DisplayFormatAlpha( temp_surface );

   /* Image load. */
   surface = (SDL_Surface*)g->surface;
   g->w = surface->w;
   g->h = surface->h;

   scaffold_check_null( g->surface );

cleanup:
   if( NULL != temp_surface ) {
      SDL_FreeSurface( temp_surface );
   }
   SDL_FreeRW( rwop );
#else
   graphics_bitmap_load( data, length, &bitmap );
   scaffold_check_null( bitmap );
   scaffold_check_null( bitmap->pixels );
   g->surface = SDL_CreateRGBSurface(
      SDL_SWSURFACE,
      bitmap->w,
      bitmap->h,
      screen->format->BitsPerPixel,
      screen->format->Rmask,
      screen->format->Gmask,
      screen->format->Bmask,
      screen->format->Amask
   );
   scaffold_check_null( g->surface );
   g->w = bitmap->w;
   g->h = bitmap->h;
   sdl_ret = SDL_LockSurface( g->surface );
   scaffold_check_nonzero( sdl_ret );
   surface = (SDL_Surface*)g->surface;
   scaffold_check_null( surface );
   format = surface->format;
   scaffold_check_null( format );
   scaffold_assert( 16 == format->BitsPerPixel );
   //holder = scaffold_alloc( bytes_per_pixel, BYTE );
   for( y = 0 ; surface->h > y ; y++ ) {
      surface_i = (y / bitmap->w) + (y % bitmap->w);

      pixel = &(graphics_stock_colors[(int)(bitmap->pixels[i++])]);
      ((uint16_t*)surface->pixels)[surface_i] =
         ((pixel->r >> format->Rshift) << format->Rloss);

      scaffold_assert( ((uint16_t*)surface->pixels)[surface_i] == 0 );

      /*
      holder =
         graphics_stock_colors[(int)(bitmap->pixels[i++])].r;
      holder << format->Rshift;
      *(surface->pixels + (surface_i * bytes_per_pixel)) &= *holder;
      */


//      /* FIXME: Read and use the bitmap format. */
//      ((uint8_t*)(surface->pixels))[surface_i] =;
   }
   SDL_UnlockSurface( g->surface );

cleanup:
   if( NULL != holder ) {
      free( holder );
   }
   graphics_free_bitmap( bitmap );
#endif /* USE_SDL_IMAGE */
}

BYTE* graphics_export_image_data( GRAPHICS* g, SCAFFOLD_SIZE* out_len ) {
}

void graphics_draw_rect(
   GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE w, SCAFFOLD_SIZE h, GRAPHICS_COLOR color_i
) {
   SDL_Rect rect;
   SDL_Color* color = &(graphics_stock_colors[color_i]);
   SDL_Surface* surface = g->surface;

   rect.x = x;
   rect.y = y;
   rect.w = w,
   rect.h = h;

   SDL_FillRect(
      g->surface,
      &rect,
      SDL_MapRGB( surface->format, color->r, color->g, color->b )
   );
}

void graphics_draw_char(
   GRAPHICS* g, SCAFFOLD_SIZE x_start, SCAFFOLD_SIZE y_start,
   GRAPHICS_COLOR color_i, GRAPHICS_FONT_SIZE size, char c
) {
   SCAFFOLD_SIZE x, y, bit;
   uint8_t* font_char;
   float divisor;
   SDL_Color* color;

   divisor = size / 8.0f;

   color = &(graphics_stock_colors[color_i]);

   SDL_LockSurface( g->surface );
   for( y = 0 ; size > y ; y++ ) {
      for( x = 0 ; size > x ; x++ ) {
         if( font8x8_basic[c][(uint8_t)(y / divisor)] & 1 << (uint8_t)(x / divisor) ) {
            SDL_PutPixel( g->surface, x_start + x, y_start + y, *((uint32_t*)color) );
         }
      }
   }
   SDL_UnlockSurface( g->surface );
}

void graphics_transition( GRAPHICS* g, GRAPHICS_TRANSIT_FX fx ) {
}

void graphics_scale( GRAPHICS* g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h ) {
}

void graphics_blit(
   GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE s_w, SCAFFOLD_SIZE s_h,
   const GRAPHICS* src
) {
   SDL_Rect src_rect,
      dest_rect;

   src_rect.x = 0;
   src_rect.y = 0;
   src_rect.w = s_w;
   src_rect.h = s_h;

   dest_rect.x = x;
   dest_rect.y = y;
   dest_rect.w = s_w;
   dest_rect.h = s_h;

   SDL_BlitSurface( src->surface, &src_rect, g->surface, &dest_rect );
}

void graphics_blit_partial(
   GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE s_x,
   SCAFFOLD_SIZE s_y, SCAFFOLD_SIZE s_w, SCAFFOLD_SIZE s_h, const GRAPHICS* src
) {
   SDL_Rect src_rect,
      dest_rect;

   src_rect.x = s_x;
   src_rect.y = s_y;
   src_rect.w = s_w;
   src_rect.h = s_h;

   dest_rect.x = x;
   dest_rect.y = y;
   dest_rect.w = s_w;
   dest_rect.h = s_h;

   SDL_BlitSurface( src->surface, &src_rect, g->surface, &dest_rect );
}

void graphics_sleep( uint16_t milliseconds ) {
   SDL_Delay( milliseconds );
}

uint32_t graphics_get_ticks() {
   return SDL_GetTicks();
}

#if 0
void graphics_wait_for_fps_timer() {
   //SDL_Delay( 1000 / GRAPHICS_TIMER_FPS );
   if( GRAPHICS_TIMER_FPS > (SDL_GetTicks() - graphics_time) ) {
      /* Subtract the time since graphics_Start_fps_timer() was last called
       * from the nominal delay required to maintain our FPS.
       */
      SDL_Delay( graphics_fps_delay  - (SDL_GetTicks() - graphics_time) );
   }
   /*
   scaffold_print_debug( &module, "%d\n", (SDL_GetTicks() - graphics_time) );
   */
}
#endif
