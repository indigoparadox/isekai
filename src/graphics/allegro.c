
#include "../graphics.h"

#include "../scaffold.h"

#include <allegro.h>

typedef struct _GRAPHICS_FMEM_INFO {
    AL_CONST unsigned char *block;
    long length;
    long alloc;
    long offset;
} GRAPHICS_FMEM_INFO;

static int graphics_fmem_fclose( void* userdata ) {
    GRAPHICS_FMEM_INFO* info = userdata;
    ASSERT( info );
    ASSERT( info->offset <= info->length );

    /* Shorten the allocation back down to the stated length. */
    if( info->alloc > info->length ) {
        info->block = realloc( (unsigned char*)info->block, info->length );
        ASSERT( NULL !=info->block );
        info->alloc = info->length;
    }

    return 0;
}

static int graphics_fmem_getc( void* userdata ) {
   GRAPHICS_FMEM_INFO* info = userdata;
   ASSERT( info );
   ASSERT( info->offset <= info->length );

   if( info->offset == info->length ) {
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
   ASSERT( info );
   ASSERT( info->offset <= info->length );

   actual = MIN( n, info->length - info->offset );

   memcpy( p, info->block + info->offset, actual );
   info->offset += actual;

   ASSERT( info->offset <= info->length );

   return actual;
}

static int graphics_fmem_putc( int c, void* userdata ) {
    GRAPHICS_FMEM_INFO* info = userdata;
    ASSERT( info );
    ASSERT( info->offset <= info->length );

    if( info->offset == info->length ) {
        /* Grab some more memory if's getting longer. */
        if( info->alloc <= info->length ) {
            info->alloc *= 2;
            info->block = realloc( (unsigned char*)info->block, info->alloc );
        }
        info->length++;
    }

    return info->block[info->offset++];
}

static long graphics_fmem_fwrite( const void* p, long n, void* userdata ) {
    long i;
    uint8_t* c = (unsigned char*)p;
    ASSERT( info );
    ASSERT( info->offset <= info->length );

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
   long actual;

   ASSERT( info );
   ASSERT( info->offset <= info->length );

   actual = MIN( offset, info->length - info->offset );

   info->offset += actual;

   ASSERT( info->offset <= info->length );

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

    scaffold_error = 0;

    screen_return = set_gfx_mode( GFX_AUTODETECT_WINDOWED, w, h, 0, 0);
    scaffold_check_nonzero( screen_return );

    g->surface = create_bitmap( w, h );
    scaffold_check_null( g->surface );

cleanup:
    return;
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
}

void graphics_surface_cleanup( GRAPHICS* g ) {
    if( NULL != g->surface ) {
        destroy_bitmap( g->surface );
    }
}

void graphics_flip_screen( GRAPHICS* g ) {
    blit( screen, g->surface, 0, 0, 0, 0, g->w, g->h );
}

void graphics_shutdown( GRAPHICS* g ) {
    graphics_surface_cleanup( g );
}

void graphics_set_font( GRAPHICS* g, const bstring name ) {

}

void graphics_set_color( GRAPHICS* g, GRAPHICS_COLOR color ) {

}

void graphics_set_image_path( GRAPHICS* g, const bstring path ) {
    if( NULL != g->surface ) {
        destroy_bitmap( g->surface );
    }
    load_bitmap( bdata( path ), NULL );
}

void graphics_set_image_data( GRAPHICS* g, const uint8_t* data, uint32_t length ) {
    GRAPHICS_FMEM_INFO fmem_info;
    PACKFILE* fmem = NULL;

    if( NULL != g->surface ) {
        destroy_bitmap( g->surface );
    }

    fmem_info.block = data;
    fmem_info.length = length;
    fmem_info.offset = 0;
    fmem_info.alloc = 0;

    fmem = pack_fopen_vtable( &graphics_fmem_vtable, &fmem_info );
    scaffold_check_null( fmem );

    /* TODO: Autodetect image type. */
    g->surface = load_bmp_pf( fmem, NULL );
    //texture_out = load_memory_png( image_data, entry->unpacked_size, pal );
    scaffold_check_null( g->surface );

cleanup:
    if( NULL != fmem ){
        pack_fclose( fmem );
    }
    return;
}

void graphics_export_image_data( GRAPHICS* g, uint8_t* data, uint32_t* length ) {
    GRAPHICS_FMEM_INFO fmem_info;
    PACKFILE* fmem = NULL;

    scaffold_check_null( g->surface );

    fmem_info.block = data;
    fmem_info.length = *length;
    fmem_info.offset = 0;
    fmem_info.alloc = *length;

    fmem = pack_fopen_vtable( &graphics_fmem_vtable, &fmem_info );
    scaffold_check_null( fmem );

    save_bmp_pf( fmem, g->surface, NULL );
    *length = fmem_info.length;

cleanup:
    if( NULL != fmem ){
        pack_fclose( fmem );
    }
    return;
}

void graphics_draw_text( GRAPHICS* g, gu x, gu y, const bstring text ) {

}

void graphics_draw_rect( GRAPHICS* g, gu x, gu y, gu w, gu h ) {

}

void graphics_measure_text( GRAPHICS* g, GRAPHICS_RECT* r, const bstring text ) {

}

void graphics_transition( GRAPHICS* g, GRAPHICS_TRANSIT_FX fx ) {

}

void graphics_scale( GRAPHICS* g, gu w, gu h ) {

}

void graphics_blit( GRAPHICS* g, gu x, gu y, gu s_w, gu s_h, const GRAPHICS* src ) {
    masked_blit( g->surface, src->surface, 0, 0, 0, 0, src->w, src->h );
}

void graphics_sleep( uint16_t milliseconds ) {
    rest( milliseconds );
}
