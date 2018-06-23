
#define MODE_C
#include "mode.h"

void client_local_update(
   struct CLIENT* c,
   struct CHANNEL* l,
   struct GRAPHICS_TILE_WINDOW* twindow
) {

#ifndef DISABLE_MODE_POV
   if( NULL != c && NULL != c->puppet ) {
      c->cam_pos.precise_x = c->puppet->x;
      c->cam_pos.precise_y = c->puppet->y;
   }
#endif /* !DISABLE_MODE_POV */

   switch( c->gfx_mode ) {
   case MODE_TOPDOWN:
      mode_topdown_update( c, l, twindow );
      break;
#ifndef DISABLE_MODE_POV
   case MODE_POV:
      mode_pov_update( c, l, twindow );
      break;
#endif /* !DISABLE_MODE_POV */
   }
}

void client_local_draw(
   struct CLIENT* c,
   struct CHANNEL* l,
   struct GRAPHICS_TILE_WINDOW* twindow
) {
   switch( c->gfx_mode ) {
   case MODE_TOPDOWN:
      mode_topdown_draw( c, l, twindow );
      break;
#ifndef DISABLE_MODE_POV
   case MODE_POV:
      mode_pov_draw( c, l, twindow );
      break;
#endif /* !DISABLE_MODE_POV */
   }
}

void client_local_poll_input( struct CLIENT* c, struct CHANNEL* l, struct INPUT* p ) {
   switch( c->gfx_mode ) {
   case MODE_TOPDOWN:
      mode_topdown_poll_input( c, l, p );
      break;
#ifndef DISABLE_MODE_POV
   case MODE_POV:
      mode_pov_poll_input( c, l, p );
      break;
#endif /* !DISABLE_MODE_POV */
   }
}

void client_local_free( struct CLIENT* c ) {
   switch( c->gfx_mode ) {
   case MODE_TOPDOWN:
      mode_topdown_free( c );
      break;
#ifndef DISABLE_MODE_POV
   case MODE_POV:
      mode_pov_free( c );
      break;
#endif /* !DISABLE_MODE_POV */
   }
}
