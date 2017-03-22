
#ifdef S_SPLINT_S
#define BYTE uint8_t
#define scaffold_assert( expr )
typedef struct PACKFILE_VTABLE PACKFILE_VTABLE;
#define TRUE 1
#define FALSE 0
#else

#define GRAPHICS_C
#include "../graphics.h"
#include "../scaffold.h"
#include "../font.h"

#include <allegro.h>

#ifdef USE_ALLEGRO_PNG
#include <loadpng.h>
#endif /* USE_ALLEGRO_PNG */

#endif

#define GRAPHICS_FMEM_SENTINAL 2121
#define GRAPHICS_COLOR_DEPTH 8

static volatile uint32_t allegro_ticks = 0;

static void allegro_ticker() {
   allegro_ticks++;
}
END_OF_STATIC_FUNCTION( allegro_ticker )

typedef struct _GRAPHICS_FMEM_INFO {
   BYTE safety1;
   BYTE safety2;
#ifdef DEBUG
   uint16_t sentinal_start;
#endif /* DEBUG */
   BYTE* block;
   SCAFFOLD_SIZE length;
   SCAFFOLD_SIZE alloc;
   SCAFFOLD_SIZE offset;
#ifdef DEBUG
   uint16_t sentinal_end;
#endif /* DEBUG */
} GRAPHICS_FMEM_INFO;

static int graphics_fmem_fclose( void* userdata ) {
   GRAPHICS_FMEM_INFO* info = userdata;
   scaffold_assert( info );
   scaffold_assert( info->offset <= info->length );
   scaffold_assert( GRAPHICS_FMEM_SENTINAL == info->sentinal_start );
   scaffold_assert( GRAPHICS_FMEM_SENTINAL == info->sentinal_end );

   /* Shorten the allocation back down to the stated length. */
   if( info->alloc > info->length ) {
      /* FIXME: Soft realloc. */
      info->block = scaffold_realloc( info->block, info->length, BYTE );
      scaffold_assert( NULL !=info->block );
      info->alloc = info->length;
   }

   return 0;
}

static int graphics_fmem_getc( void* userdata ) {
   int char_out = EOF;
   GRAPHICS_FMEM_INFO* info = userdata;
   scaffold_assert( info );
   scaffold_assert( info->offset < info->length );
   scaffold_assert( GRAPHICS_FMEM_SENTINAL == info->sentinal_start );
   scaffold_assert( GRAPHICS_FMEM_SENTINAL == info->sentinal_end );

   if( 0 < info->alloc && info->offset < info->length ) {
      char_out = info->block[info->offset++];
   }

   return char_out;
}

static int graphics_fmem_ungetc( int c, void* userdata ) {
   GRAPHICS_FMEM_INFO* info = userdata;
   unsigned char ch = (unsigned char)c;
   scaffold_assert( GRAPHICS_FMEM_SENTINAL == info->sentinal_start );
   scaffold_assert( GRAPHICS_FMEM_SENTINAL == info->sentinal_end );

   if( (0 < info->offset) && (info->block[info->offset - 1] == ch) ) {
      return ch;
   } else {
      return EOF;
   }
}

static long graphics_fmem_fread( void* p, long n, void* userdata ) {
   GRAPHICS_FMEM_INFO* info = userdata;
   SCAFFOLD_SIZE actual;
   scaffold_assert( info );
   scaffold_assert( info->offset <= info->length );
   scaffold_assert( GRAPHICS_FMEM_SENTINAL == info->sentinal_start );
   scaffold_assert( GRAPHICS_FMEM_SENTINAL == info->sentinal_end );

   actual = MIN( n, info->length - info->offset );

   memcpy( p, info->block + info->offset, actual );
   info->offset += actual;

   scaffold_assert( info->offset <= info->length );

   return actual;
}

static int graphics_fmem_putc( int c, void* userdata ) {
   GRAPHICS_FMEM_INFO* info = userdata;
   scaffold_assert( info );
   scaffold_assert( info->offset <= info->length );
   scaffold_assert( GRAPHICS_FMEM_SENTINAL == info->sentinal_start );
   scaffold_assert( GRAPHICS_FMEM_SENTINAL == info->sentinal_end );

   if( NULL == info->block ) {
      info->block = (BYTE*)calloc( info->alloc, sizeof( BYTE ) );
   }

   if( info->offset == info->length ) {
      /* Grab some more memory if's getting longer. */
      if( info->alloc <= info->length ) {
         info->alloc *= 2;
         /* FIXME: Soft realloc. */
         info->block = scaffold_realloc( info->block, info->alloc, BYTE );
         scaffold_assert( NULL !=info->block );
      }
      info->length++;
   }

   info->block[info->offset] = (BYTE)c;

   return info->block[info->offset++];
}

static long graphics_fmem_fwrite( const void* p, long n, void* userdata ) {
#ifdef DEBUG
   GRAPHICS_FMEM_INFO* info = userdata;
#endif /* DEBUG */
   long i;
   BYTE* c = (BYTE*)p;
   scaffold_assert( GRAPHICS_FMEM_SENTINAL == info->sentinal_start );
   scaffold_assert( GRAPHICS_FMEM_SENTINAL == info->sentinal_end );

   for( i = 0 ; n > i ; i++ ) {
      if( *c != graphics_fmem_putc( *c, userdata ) ) {
         /* Something went wrong. */
         break;
      }
      c++;
   }

   return i;
}

static int graphics_fmem_fseek( void* userdata, int offset ) {
   GRAPHICS_FMEM_INFO* info = userdata;
   SCAFFOLD_SIZE actual;
   scaffold_assert( GRAPHICS_FMEM_SENTINAL == info->sentinal_start );
   scaffold_assert( GRAPHICS_FMEM_SENTINAL == info->sentinal_end );

   scaffold_assert( info );
   scaffold_assert( info->offset < info->length );

   actual = MIN( offset, info->length - info->offset );

   info->offset += actual;

   scaffold_assert( info->offset < info->length );

   if( offset == actual ) {
      return 0;
   } else {
      return -1;
   }
}

static int graphics_fmem_feof( void* userdata ) {
   GRAPHICS_FMEM_INFO* info = userdata;
   scaffold_assert( GRAPHICS_FMEM_SENTINAL == info->sentinal_start );
   scaffold_assert( GRAPHICS_FMEM_SENTINAL == info->sentinal_end );

   return info->offset >= info->length ? 1 : 0;
}

static int graphics_fmem_ferror( void* userdata ) {
   /* STUB */
   return FALSE;
}

/*
AL_METHOD(int, pf_fclose, (void *userdata));
AL_METHOD(int, pf_getc, (void *userdata));
AL_METHOD(int, pf_ungetc, (int c, void *userdata));
AL_METHOD(long, pf_fread, (void *p, long n, void *userdata));
AL_METHOD(int, pf_putc, (int c, void *userdata));
AL_METHOD(long, pf_fwrite, (AL_CONST void *p, long n, void *userdata));
AL_METHOD(int, pf_fseek, (void *userdata, int offset));
AL_METHOD(int, pf_feof, (void *userdata));
AL_METHOD(int, pf_ferror, (void *userdata));
*/

const PACKFILE_VTABLE graphics_fmem_vtable = {
   graphics_fmem_fclose,
   graphics_fmem_getc,
   graphics_fmem_ungetc,
   graphics_fmem_fread,
   graphics_fmem_putc,
   graphics_fmem_fwrite,
   graphics_fmem_fseek,
   graphics_fmem_feof,
   graphics_fmem_ferror,
};

void graphics_screen_new(
   GRAPHICS** g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h,
   SCAFFOLD_SIZE vw, SCAFFOLD_SIZE vh, int32_t arg1, void* arg2
) {
   int screen_return;

   allegro_init();

   LOCK_VARIABLE( allegro_ticks );
   LOCK_FUNCTION( allegro_ticker )

   install_int( allegro_ticker, BPS_TO_TIMER( 1000000 ) );

   graphics_setup();

#ifdef USE_ALLEGRO_PNG
   loadpng_init();
#endif /* USE_ALLEGRO_PNG */

#ifdef USE_ALLEGRO_GIF
   algif_init();
#endif /* USE_ALLEGRO_GIF */

   set_color_conversion( COLORCONV_NONE );

   scaffold_error = 0;

   set_color_depth( GRAPHICS_COLOR_DEPTH );
   screen_return = set_gfx_mode( GFX_AUTODETECT_WINDOWED, w, h, 0, 0 );
   scaffold_check_nonzero( screen_return );

   /* TODO: Free double buffer. */
   *g = scaffold_alloc( 1, GRAPHICS );
   scaffold_check_null( *g );
   (*g)->surface = create_bitmap( w, h );
   (*g)->w = w;
   (*g)->h = h;
   scaffold_check_null( (*g)->surface );

   clear_bitmap( (*g)->surface );

cleanup:
   return;
}

void graphics_surface_cleanup( GRAPHICS* g ) {
   if( NULL != g->surface ) {
      destroy_bitmap( g->surface );
      g->surface = NULL;
   }
   if( NULL == g->palette ) {
      scaffold_free( g->palette );
      g->palette = NULL;
   }
   /* TODO: Free surface. */
}

void graphics_surface_init( GRAPHICS* g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h ) {
   if( 0 < w && 0 < h) {
      g->surface = create_bitmap( w, h );
   } else {
      g->surface = NULL;
   }
   g->w = w;
   g->h = h;
   g->palette = NULL;
   return;
}

void graphics_set_window_title( GRAPHICS* g, bstring title, void* icon ) {
   set_window_title( bdata( title ) );
}

void graphics_flip_screen( GRAPHICS* g ) {
   blit( g->surface, screen, g->virtual_x, g->virtual_y, 0, 0, g->w, g->h );
   /* clear_bitmap( g->surface ); */
}

void graphics_shutdown( GRAPHICS* g ) {
   graphics_surface_free( g );
   allegro_exit();
}

void graphics_screen_scroll(
   GRAPHICS* g, SCAFFOLD_SIZE offset_x, SCAFFOLD_SIZE offset_y
) {
   g->virtual_x += offset_x;
   g->virtual_y += offset_y;
}

void graphics_set_image_path( GRAPHICS* g, const bstring path ) {
   scaffold_check_null( path );
   if( NULL != g->surface ) {
      destroy_bitmap( g->surface );
   }
#ifdef USE_ALLEGRO_PALETTE
   if( NULL == g->palette ) {
      g->palette = (RGB*)calloc( 1, sizeof( RGB ) );
   }
#endif /* USE_ALLEGRO_PALETTE */
   g->surface = load_bitmap(
      bdata( path ),
#ifdef USE_ALLEGRO_PALETTE
      (RGB*)(g->palette)
#else
      NULL
#endif /* USE_ALLEGRO_PALETTE */
   );
   if( NULL == g->surface ) {
      scaffold_print_error(
         &module, "Image load error: %s: %s\n", bdata( path ), allegro_error );
   }
   g->w = ((BITMAP*)g->surface)->w;
   g->h = ((BITMAP*)g->surface)->h;
cleanup:
   return;
}

#ifdef ALLEGRO_EXPORT_PALETTE
static void graphics_export_palette() {
   /* 3 * (3 digits + 1 tab) + 8 description + 1 newline */
   /* const SCAFFOLD_SIZE pal_len = (256 * ((3 * (4)) + 9)) + 1;
   char* palette */
   bstring palette_out = NULL;
   bstring palette_path = NULL;
   PALETTE pal_out;
   int i;

   palette_out = bfromcstralloc( 1024, "GIMP Palette\nName: Allegro\nColumns: 16\n#\n" );
   palette_path = bfromcstr( "allegro_palette.gpl" );

   for( i = 0 ; GRAPHICS_COLOR_DEPTH > i ; i++ ) {
      get_color( i, pal_out );
      bformata( palette_out, "%d\t%d\t%d\tUntitled\n", pal_out->r, pal_out->g, pal_out->b );
   }

   scaffold_write_file( palette_path, (BYTE*)bdata( palette_out ), blength( palette_out ), FALSE );

/* cleanup: */
   bdestroy( palette_path );
   bdestroy( palette_out );
   return;
}
#endif /* ALLEGRO_EXPORT_PALETTE */

void graphics_set_image_data( GRAPHICS* g, const BYTE* data,
                              SCAFFOLD_SIZE length ) {
#ifdef USE_ALLEGRO_BITMAP
   GRAPHICS_FMEM_INFO* fmem_info = NULL;
   PACKFILE* fmem = NULL;
   BOOL close_packfile = TRUE;

   fmem_info = (GRAPHICS_FMEM_INFO*)calloc( 1, sizeof( GRAPHICS_FMEM_INFO ) );
   scaffold_check_null( fmem_info );
#else
   struct GRAPHICS_BITMAP* bitmap = NULL;
   SCAFFOLD_SIZE_SIGNED x, y, i = 0;
#endif /* USE_ALLEGRO_BITMAP */

   if( NULL != g->surface ) {
      destroy_bitmap( g->surface );
      g->surface = NULL;
   }

#ifdef USE_ALLEGRO_BITMAP
   if( NULL == g->palette ) {
      g->palette = (RGB*)calloc( 1, sizeof( RGB ) );
   }

#ifdef DEBUG
   fmem_info->sentinal_start = GRAPHICS_FMEM_SENTINAL;
   fmem_info->sentinal_end = GRAPHICS_FMEM_SENTINAL;
#endif /* DEBUG */
   fmem_info->block = (BYTE*)data;
   fmem_info->length = length;
   fmem_info->offset = 0;
   fmem_info->alloc = length;

   fmem = pack_fopen_vtable( &graphics_fmem_vtable, fmem_info );
   scaffold_check_null( fmem );

   /* Autodetect image type. */
   g->surface = load_bmp_pf( fmem, (RGB*)g->palette );

#ifdef ALLEGRO_EXPORT_PALETTE
   graphics_export_palette();splitdebug
#endif /* ALLEGRO_EXPORT_PALETTE */

   if( NULL == g->surface ) {
#ifdef USE_ALLEGRO_PNG
      g->surface = load_memory_png( data, length, (RGB*)g->palette );
      if( NULL == g->surface ) {
#endif /* USE_ALLEGRO_PNG */
         g->surface = load_tga_pf( fmem, (RGB*)g->palette );
         if( NULL == g->surface ) {

#ifdef USE_ALLEGRO_GIF
            pack_fclose( fmem );
            fmem_info.block = (BYTE*)data;
            fmem_info.length = length;
            fmem_info.offset = 0;
            fmem_info.alloc = 0;
            fmem = pack_fopen_vtable( &graphics_fmem_vtable, &fmem_info );
            g->surface = load_gif_pf( fmem, (PALETTE*)g->palette );
            close_packfile = FALSE;
#endif /* USE_ALLEGRO_GIF */

         }
#ifdef USE_ALLEGRO_PNG
      }
#endif /* USE_ALLEGRO_PNG */
   }
   scaffold_check_null( g->surface );
   g->w = ((BITMAP*)g->surface)->w;
   g->h = ((BITMAP*)g->surface)->h;

#ifdef USE_BITMAP_PALETTE
   set_pallete( g->palette );
#endif /* USE_BITMAP_PALETTE */

cleanup:
   if( NULL != fmem && FALSE != close_packfile ) {
      pack_fclose( fmem );
   }
   if( NULL != fmem_info ) {
      scaffold_free( fmem_info );
   }
#else
   graphics_bitmap_load( data, length, &bitmap );
   scaffold_check_null( bitmap );
   scaffold_check_null( bitmap->pixels );
   g->surface = create_bitmap( bitmap->w, bitmap->h );
   g->w = bitmap->w;
   g->h = bitmap->h;
   for( y = bitmap->h - 1 ; 0 <= y ; y-- ) {
      for( x = 0 ; bitmap->w > x ; x++ ) {
         putpixel( g->surface, x, y, (int)(bitmap->pixels[i++]) );
      }
   }
   scaffold_check_null( g->surface );

cleanup:
   graphics_free_bitmap( bitmap );
#endif /* USE_ALLEGRO_BITMAP */
   return;
}

BYTE* graphics_export_image_data( GRAPHICS* g, SCAFFOLD_SIZE* out_len ) {
   GRAPHICS_FMEM_INFO* fmem_info = NULL;
   PACKFILE* fmem = NULL;

   fmem_info = (GRAPHICS_FMEM_INFO*)calloc( 1, sizeof( GRAPHICS_FMEM_INFO ) );

   scaffold_check_null( g );
   scaffold_check_null( g->surface );
   scaffold_check_null( out_len );

   fmem_info->length = *out_len;
   fmem_info->offset = 0;
   fmem_info->alloc = *out_len;

   fmem = pack_fopen_vtable( &graphics_fmem_vtable, fmem_info );
   scaffold_check_null( fmem );

/*#ifdef USE_ALLEGRO_PNG
   save_tga_pf( fmem, g->surface, NULL );
#else*/
   save_bmp_pf( fmem, g->surface, NULL );
/*#endif / USE_ALLEGRO_PNG */
   *out_len = fmem_info->length;

cleanup:
   if( NULL != fmem ) {
      pack_fclose( fmem );
   }
   if( NULL != fmem_info ) {
      scaffold_free( fmem_info );
   }
   return fmem_info->block;
}

void graphics_draw_rect(
   GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE w, SCAFFOLD_SIZE h,
   GRAPHICS_COLOR color
) {
   rectfill( g->surface, x, y, x + w, y + h, color );
}

void graphics_draw_char(
   GRAPHICS* g, SCAFFOLD_SIZE_SIGNED x_start, SCAFFOLD_SIZE_SIGNED y_start,
   GRAPHICS_COLOR color, GRAPHICS_FONT_SIZE size, char c
) {
   SCAFFOLD_SIZE_SIGNED x, y, bit;
   uint8_t* font_char;
   float divisor;

   divisor = size / 8.0f;

   lock_bitmap( g->surface );
   for( y = 0 ; size > y ; y++ ) {
      for( x = 0 ; size > x ; x++ ) {
         if( font8x8_basic[c][(uint8_t)(y / divisor)] & 1 << (uint8_t)(x / divisor) ) {
            putpixel( g->surface, x_start + x, y_start + y, color );
         }
      }
   }
   release_bitmap( g->surface );
}

void graphics_transition( GRAPHICS* g, GRAPHICS_TRANSIT_FX fx ) {
   /* TODO: Graphical transitions not implemented. */
}

void graphics_scale( GRAPHICS* g, SCAFFOLD_SIZE w, SCAFFOLD_SIZE h ) {
   BITMAP* temp = NULL;

   temp = create_bitmap( w, h );
   scaffold_check_null( temp );

   blit( g->surface, temp, 0, 0, 0, 0, g->w, g->h );

   if( NULL != g->surface ) {
      destroy_bitmap( g->surface );
   }

   g->surface = temp;
   scaffold_check_null( g->surface );

   g->w = w;
   g->h = h;

cleanup:
   return;
}

void graphics_blit( GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE s_w, SCAFFOLD_SIZE s_h,
                    const GRAPHICS* src ) {
   masked_blit( src->surface, g->surface, 0, 0, x, y, src->w, src->h );
}

void graphics_blit_partial(
   GRAPHICS* g, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE s_x, SCAFFOLD_SIZE s_y, SCAFFOLD_SIZE s_w, SCAFFOLD_SIZE s_h, const GRAPHICS* src
) {
   masked_blit( src->surface, g->surface, s_x, s_y, x, y, s_w, s_h );
}

void graphics_sleep( uint16_t milliseconds ) {
   rest( milliseconds );
}

uint32_t graphics_get_ticks() {
   return allegro_ticks;
}

#if 0
void graphics_wait_for_fps_timer() {
   //SDL_Delay( 1000 / GRAPHICS_TIMER_FPS );
   if( GRAPHICS_TIMER_FPS > (allegro_ticks - graphics_time) ) {
      /* Subtract the time since graphics_Start_fps_timer() was last called
       * from the nominal delay required to maintain our FPS.
       */
      //SDL_Delay( graphics_fps_delay  - (SDL_GetTicks() - graphics_time) );
      rest( 10000.0f / ((graphics_fps_delay) - (allegro_ticks - graphics_time)) );
   }
   scaffold_print_debug( &module, "%d\n", (allegro_ticks - graphics_time) );

}
#endif
