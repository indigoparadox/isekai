
#include "htmlrend.h"

char html_tag_next_char( struct html_tree_tag* tag, int idx, int flags ) {
   bchar( tag->data, idx );
}

char html_tree_next_char( struct html_tree* tree, int idx, int flags ) {
}

void htmlrend_draw( GRAPHICS* g, struct html_tree* tree ) {
   struct html_tree_tag* tag = NULL;

   lgc_null( tree );
   tag = tree->root;

   //for( i = 0 ; blength( tag->data ) > i ; i++ ) {
      graphics_draw_text(
         g, 0, 0, GRAPHICS_TEXT_ALIGN_LEFT, GRAPHICS_COLOR_WHITE,
         GRAPHICS_FONT_SIZE_12, tag->data, VFALSE
      );
   //}

cleanup:
   return;
}
