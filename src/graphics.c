
#define GRAPHICS_C
#include "graphics.h"

#include <stdlib.h>
#include <time.h>

static uint32_t graphics_time = 0;
static uint32_t graphics_fps_delay = 0;

#ifdef DEBUG_FPS

static struct tagbstring str_wid_debug_fps = bsStatic( "debug_fps" );

static bstring graphics_fps = NULL;

#endif /* DEBUG_FPS */

#pragma pack(push,1)
struct GRAPHICS_BITMAP_FILE_HEADER {
    int8_t type[2];
    int32_t size;
    int16_t reserved1;
    int16_t reserved2;
    int32_t offset;
};

struct GRAPHICS_BITMAP_HEADER {
    int32_t header_sz;
    int32_t width;
    int32_t height;
    int16_t planes;
    int16_t bpp;
    int32_t compression;
    int32_t image_sz;
    int32_t ppi_x;
    int32_t ppi_y;
    int32_t colors_used;
    int32_t colors_sig;
};
#pragma pack(pop)

void graphics_free_bitmap( struct GRAPHICS_BITMAP* bitmap ) {
   if( NULL != bitmap ) {
      if( NULL != bitmap->pixels ) {
         scaffold_free( bitmap->pixels );
      }
      scaffold_free( bitmap );
   }
}

void graphics_bitmap_load(
   const BYTE* data, SCAFFOLD_SIZE data_sz, struct GRAPHICS_BITMAP** bitmap_out
) {
#ifdef USE_BITMAP_DISCREET
   uint32_t* header_data_sz_ptr = (uint32_t*)&(data[2]);
   uint32_t* header_offset_ptr = (uint32_t*)&(data[10]);
#else
   struct GRAPHICS_BITMAP_FILE_HEADER* file_header = NULL;
   struct GRAPHICS_BITMAP_HEADER* header = NULL;
#endif /* USE_BITMAP_DISCREET */
   SCAFFOLD_SIZE pixels_sz = 0,
      i,
      output_i = 0;

   file_header = (struct GRAPHICS_BITMAP_FILE_HEADER*)&(data[0]);
   header = (struct GRAPHICS_BITMAP_HEADER*)&(data[14]);

   scaffold_print_debug(
      &module, "Bitmap: %d x %d\n", header->width, header->height
   );

   pixels_sz = ((header->width * header->height) / 2);

   scaffold_assert( 'B' == data[0] && 'M' == data[1] );
   scaffold_assert( file_header->size == data_sz );
   scaffold_assert( 4 == header->bpp );
   scaffold_assert( 0 == header->compression );
   scaffold_assert( (file_header->offset + pixels_sz) <= data_sz );

   *bitmap_out =
      (struct GRAPHICS_BITMAP*)calloc( 1, sizeof( struct GRAPHICS_BITMAP ) );
   scaffold_check_null( *bitmap_out );
   (*bitmap_out)->w = header->width;
   (*bitmap_out)->h = header->height;
   (*bitmap_out)->pixels_sz = (header->width * header->height);
   (*bitmap_out)->pixels = (GRAPHICS_COLOR*)calloc(
      (*bitmap_out)->pixels_sz, sizeof( GRAPHICS_COLOR )
   );
   scaffold_check_null( (*bitmap_out)->pixels );

   for( i = file_header->offset ; (file_header->offset + pixels_sz) > i ; i++ ) {
      (*bitmap_out)->pixels[output_i++] = (data[i] | 0x00) >> 4;
      (*bitmap_out)->pixels[output_i++] = (data[i] | 0xf0) & 0x0f;
   }

cleanup:
   return;
}

void graphics_setup() {
   graphics_fps_delay = 1000 / GRAPHICS_TIMER_FPS;
}

void graphics_start_fps_timer() {
#ifdef USE_POSIX_TIMER
   graphics_time = clock();
#else
   graphics_time = graphics_get_ticks();
#endif /* USE_POSIX_TIMER */
}

void graphics_wait_for_fps_timer() {
#ifdef USE_POSIX_TIMER
   double elapsed;

   elapsed = ((double)(clock() - graphics_time)) / CLOCKS_PER_SEC;
   if( GRAPHICS_TIMER_FPS > elapsed ) {
      graphics_sleep( graphics_fps_delay - elapsed );
   }
#else
   uint32_t ticks;
   int32_t difference,
      rest_time;

   ticks = graphics_get_ticks();
   difference = ticks - graphics_time;
   rest_time = graphics_fps_delay - (ticks - graphics_time);

   if( GRAPHICS_TIMER_FPS > difference ) {
      /* Subtract the time since graphics_Start_fps_timer() was last called
       * from the nominal delay required to maintain our FPS.
       */
      if( rest_time < 0 ) { goto cleanup; }
      graphics_sleep( rest_time );
   }

#ifdef DEBUG_FPS
   if( NULL == graphics_fps ) {
      graphics_fps = bfromcstr( "" );
   }
   bassignformat( graphics_fps, "FPS: %d\n", rest_time );
#endif /* DEBUG_FPS */
#endif /* USE_POSIX_TIMER */
cleanup:
   return;
}

#ifdef DEBUG_FPS
void graphics_debug_fps( struct UI* ui ) {
   if( NULL != graphics_fps ) {
      ui_debug_window( ui, &str_wid_debug_fps, graphics_fps );
   }
}
#endif /* DEBUG_FPS */

void graphics_draw_text(
   GRAPHICS* g, SCAFFOLD_SIZE x_start, SCAFFOLD_SIZE y_start,
   GRAPHICS_TEXT_ALIGN align, GRAPHICS_COLOR color, GRAPHICS_FONT_SIZE size,
   const bstring text
) {
   SCAFFOLD_SIZE x = x_start,
      y = y_start;
   char c;
   int i;
   GRAPHICS_RECT text_size;

   scaffold_assert( NULL != g );
   scaffold_assert( NULL != text );

   switch( align ) {
   case GRAPHICS_TEXT_ALIGN_CENTER:
      graphics_measure_text( g, &text_size, size, text );
      x -= (text_size.w / 2);
      break;
   case GRAPHICS_TEXT_ALIGN_LEFT:
      break;
   case GRAPHICS_TEXT_ALIGN_RIGHT:
      /* TODO */
      break;
   }

   for( i = 0 ; text->slen > i ; i++ ) {
      c = text->data[i];
      graphics_draw_char( g, x + (size * i), y, color, size, c );
   }
}

void graphics_measure_text(
   GRAPHICS* g, GRAPHICS_RECT* r, GRAPHICS_FONT_SIZE size, const bstring text
) {
   r->w = size;
   r->h = size;
   r->w *= blength( text );
}
