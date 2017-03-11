#include "graphics.h"

#include <stdlib.h>

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
         free( bitmap->pixels );
      }
      free( bitmap );
   }
}

SCAFFOLD_INLINE
GRAPHICS_COLOR get_graphics_bitmap_pixel(
   struct GRAPHICS_BITMAP* bmp, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y
) {

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
#endif // USE_BITMAP_DISCREET
   SCAFFOLD_SIZE pixels_sz = 0,
      i,
      output_i = 0;

   file_header = (struct GRAPHICS_BITMAP_FILE_HEADER*)&(data[0]);
   header = (struct GRAPHICS_BITMAP_HEADER*)&(data[14]);

   scaffold_print_debug( "Bitmap: %d x %d\n", header->width, header->height );

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