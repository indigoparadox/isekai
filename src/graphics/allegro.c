#include "../graphics.h"

#include "../scaffold.h"

#include <allegro.h>

#include "../algif/algif.h"

#ifdef USE_ALLEGRO_PNG
#include <loadpng.h>
#endif /* USE_ALLEGRO_PNG */

typedef struct _GRAPHICS_FMEM_INFO {
   BYTE* block;
   size_t length;
   size_t alloc;
   size_t offset;
} GRAPHICS_FMEM_INFO;

static int graphics_fmem_fclose( void* userdata ) {
   GRAPHICS_FMEM_INFO* info = userdata;
   assert( info );
   assert( info->offset < info->length );

   /* Shorten the allocation back down to the stated length. */
   if( info->alloc > info->length ) {
      info->block = (BYTE*)realloc( (BYTE*)info->block, sizeof( BYTE ) * info->length );
      assert( NULL !=info->block );
      info->alloc = info->length;
   }

   return 0;
}

static int graphics_fmem_getc( void* userdata ) {
   GRAPHICS_FMEM_INFO* info = userdata;
   assert( info );
   assert( info->offset < info->length );

   if( info->offset >= info->length ) {
      return EOF;
   } else {
      return info->block[info->offset++];
   }
}

static int graphics_fmem_ungetc( int c, void* userdata ) {
   GRAPHICS_FMEM_INFO* info = userdata;
   unsigned char ch = c;

   if( (0 < info->offset) && (info->block[info->offset - 1] == ch) ) {
      return ch;
   } else {
      return EOF;
   }
}

static long graphics_fmem_fread( void* p, long n, void* userdata ) {
   GRAPHICS_FMEM_INFO* info = userdata;
   size_t actual;
   assert( info );
   assert( info->offset <= info->length );

   actual = MIN( n, info->length - info->offset );

   memcpy( p, info->block + info->offset, actual );
   info->offset += actual;

   assert( info->offset <= info->length );

   return actual;
}

static int graphics_fmem_putc( int c, void* userdata ) {
   GRAPHICS_FMEM_INFO* info = userdata;
   assert( info );
   assert( info->offset <= info->length );

   if( NULL == info->block ) {
      info->block = (BYTE*)calloc( info->alloc, sizeof( BYTE ) );
   }

   if( info->offset == info->length ) {
      /* Grab some more memory if's getting longer. */
      if( info->alloc <= info->length ) {
         info->alloc *= 2;
         info->block = (BYTE*)realloc( (BYTE*)info->block, sizeof( BYTE ) * info->alloc );
         assert( NULL !=info->block );
      }
      info->length++;
   }

   info->block[info->offset] = (BYTE)c;

   return info->block[info->offset++];
}

static long graphics_fmem_fwrite( const void* p, long n, void* userdata ) {
   long i;
   BYTE* c = (BYTE*)p;

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
   size_t actual;

   assert( info );
   assert( info->offset < info->length );

   actual = MIN( offset, info->length - info->offset );

   info->offset += actual;

   assert( info->offset < info->length );

   if( offset == actual ) {
      return 0;
   } else {
      return -1;
   }
}

static int graphics_fmem_feof( void* userdata ) {
   GRAPHICS_FMEM_INFO* info = userdata;

   return info->offset >= info->length;
}

static int graphics_fmem_ferror( void* userdata ) {
   /* STUB */
   return FALSE;
}

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

void graphics_screen_init( GRAPHICS* g, gu w, gu h ) {
   int screen_return;

   allegro_init();

#ifdef USE_ALLEGRO_PNG
   loadpng_init();
#endif /* USE_ALLEGRO_PNG */

   algif_init();

   scaffold_error = 0;

   screen_return = set_gfx_mode( GFX_AUTODETECT_WINDOWED, w, h, 0, 0 );
   scaffold_check_nonzero( screen_return );

   /* TODO: Free double buffer. */
   g->surface = create_bitmap( w, h );
   g->w = w;
   g->h = h;
   scaffold_check_null( g->surface );

   clear_bitmap( g->surface );

cleanup:
   return;
}

static void graphics_surface_cleanup( const struct REF *ref ) {
   GRAPHICS* g = scaffold_container_of( ref, struct _GRAPHICS, refcount );
   if( NULL != g->surface ) {
      destroy_bitmap( g->surface );
   }
   /* TODO: Free surface. */
}

void graphics_surface_init( GRAPHICS* g, gu x, gu y, gu w, gu h ) {
   if( 0 < w && 0 < h) {
      g->surface = create_bitmap( w, h );
   } else {
      g->surface = NULL;
   }
   g->x = x;
   g->y = y;
   g->w = w;
   g->h = h;
   g->font = NULL;
   g->color.r = 0;
   g->color.g = 0;
   g->color.b = 0;
   g->color.a = 255;
   ref_init( &(g->refcount), graphics_surface_cleanup );
   return;
}

void graphics_surface_free( GRAPHICS* g ) {
   ref_dec( &(g->refcount) );
}

void graphics_flip_screen( GRAPHICS* g ) {
   blit( g->surface, screen, 0, 0, 0, 0, g->w, g->h );
   clear_bitmap( g->surface );
}

void graphics_shutdown( GRAPHICS* g ) {
   graphics_surface_free( g );
}

void graphics_set_font( GRAPHICS* g, const bstring name ) {
   /* TODO: Support fonts. */
}

void graphics_set_color( GRAPHICS* g, GRAPHICS_COLOR* color ) {
   memcpy( &(g->color), color, sizeof( GRAPHICS_COLOR ) );
}

void graphics_set_color_ex( GRAPHICS* gr, uint8_t r, uint8_t g, uint8_t b, uint8_t a ) {
   gr->color.r = r;
   gr->color.g = g;
   gr->color.b = b;
   gr->color.a = a;
}

void graphics_set_image_path( GRAPHICS* g, const bstring path ) {
   scaffold_check_null( path );
   if( NULL != g->surface ) {
      destroy_bitmap( g->surface );
   }
   g->surface = load_bitmap( bdata( path ), NULL );
   if( NULL == g->surface ) {
      scaffold_print_error( "Image load error: %s: %s\n", bdata( path ), allegro_error );
   }
   g->w = ((BITMAP*)g->surface)->w;
   g->h = ((BITMAP*)g->surface)->h;
cleanup:
   return;
}

void graphics_set_image_data( GRAPHICS* g, const BYTE* data,
                              size_t length ) {
   GRAPHICS_FMEM_INFO fmem_info;
   PACKFILE* fmem = NULL;
   BOOL close_packfile = TRUE;

   if( NULL != g->surface ) {
      destroy_bitmap( g->surface );
      g->surface = NULL;
   }

   fmem_info.block = (BYTE*)data;
   fmem_info.length = length;
   fmem_info.offset = 0;
   fmem_info.alloc = 0;

   fmem = pack_fopen_vtable( &graphics_fmem_vtable, &fmem_info );
   scaffold_check_null( fmem );

   /* TODO: Autodetect image type. */
   g->surface = load_bmp_pf( fmem, NULL );
   if( NULL == g->surface ) {
#ifdef USE_ALLEGRO_PNG
      g->surface = load_memory_png( data, length, NULL );
      if( NULL == g->surface ) {
#endif /* USE_ALLEGRO_PNG */

         pack_fclose( fmem );
         fmem_info.block = (BYTE*)data;
         fmem_info.length = length;
         fmem_info.offset = 0;
         fmem_info.alloc = 0;
         fmem = pack_fopen_vtable( &graphics_fmem_vtable, &fmem_info );

         g->surface = load_tga_pf( fmem, NULL );
         if( NULL == g->surface ) {

#ifdef USE_ALLEGRO_GIF
            pack_fclose( fmem );
            fmem_info.block = (BYTE*)data;
            fmem_info.length = length;
            fmem_info.offset = 0;
            fmem_info.alloc = 0;
            fmem = pack_fopen_vtable( &graphics_fmem_vtable, &fmem_info );
            g->surface = load_gif_pf( fmem, NULL );
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

cleanup:
   if( NULL != fmem && FALSE != close_packfile ) {
      pack_fclose( fmem );
   }
   return;
}

BYTE* graphics_export_image_data( GRAPHICS* g, size_t* out_len ) {
   GRAPHICS_FMEM_INFO fmem_info;
   PACKFILE* fmem = NULL;

   memset( &fmem_info, '\0', sizeof( GRAPHICS_FMEM_INFO ) );

   scaffold_check_null( g );
   scaffold_check_null( g->surface );
   scaffold_check_null( out_len );

   fmem_info.length = *out_len;
   fmem_info.offset = 0;
   fmem_info.alloc = *out_len;

   fmem = pack_fopen_vtable( &graphics_fmem_vtable, &fmem_info );
   scaffold_check_null( fmem );

/*#ifdef USE_ALLEGRO_PNG
   save_tga_pf( fmem, g->surface, NULL );
#else*/
   save_bmp_pf( fmem, g->surface, NULL );
/*#endif / USE_ALLEGRO_PNG */
   *out_len = fmem_info.length;

cleanup:
   if( NULL != fmem ) {
      pack_fclose( fmem );
   }
   return fmem_info.block;
}

void graphics_draw_text( GRAPHICS* g, gu x, gu y, const bstring text ) {
   textout_centre_ex(
      g->surface, font, bdata( text ), x, y,
      makecol( g->color.r, g->color.g, g->color.b ),
      -1
   );
}

void graphics_draw_rect( GRAPHICS* g, gu x, gu y, gu w, gu h ) {

}

void graphics_measure_text( GRAPHICS* g, GRAPHICS_RECT* r,
                            const bstring text ) {

}

void graphics_transition( GRAPHICS* g, GRAPHICS_TRANSIT_FX fx ) {

}

void graphics_scale( GRAPHICS* g, gu w, gu h ) {

}

void graphics_blit( GRAPHICS* g, gu x, gu y, gu s_w, gu s_h,
                    const GRAPHICS* src ) {
   masked_blit( src->surface, g->surface, 0, 0, x, y, src->w, src->h );
}

void graphics_blit_partial(
   GRAPHICS* g, gu x, gu y, gu s_x, gu s_y, gu s_w, gu s_h, const GRAPHICS* src
) {
   masked_blit( src->surface, g->surface, s_x, s_y, x, y, s_w, s_h );
}

void graphics_sleep( uint16_t milliseconds ) {
   rest( milliseconds );
}
