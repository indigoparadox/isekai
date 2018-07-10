
#define MODE_C
#include "mode.h"

#include "channel.h"
#include "tilemap.h"

void client_local_update(
   struct CLIENT* c,
   struct CHANNEL* l
) {
   struct MOBILE* o = NULL;

#ifndef DISABLE_MODE_POV
   o = client_get_puppet( c );
   if( NULL != o ) {
      c->cam_pos.precise_x = o->x;
      c->cam_pos.precise_y = o->y;
   }
#endif /* !DISABLE_MODE_POV */

   switch( c->gfx_mode ) {
   case MODE_TOPDOWN:
      mode_topdown_update( c, l );
      break;
#ifndef DISABLE_MODE_POV
   case MODE_POV:
      mode_pov_update( c, l );
      break;
#endif /* !DISABLE_MODE_POV */
   }
}

void client_local_draw(
   struct CLIENT* c,
   struct CHANNEL* l
) {
   struct TILEMAP* t = NULL;
   if( NULL == l ) {
      goto cleanup; /* Quietly. */
   }
   t = channel_get_tilemap( l );
   switch( c->gfx_mode ) {
   case MODE_TOPDOWN:
      switch( tilemap_get_orientation( t ) ) {
      case TILEMAP_ORIENTATION_ORTHO:
         mode_topdown_draw( c, l );
         break;
      case TILEMAP_ORIENTATION_ISO:
         mode_isometric_draw( c, l );
         break;
      }
      break;
#ifndef DISABLE_MODE_POV
   case MODE_POV:
      mode_pov_draw( c, l );
      break;
#endif /* !DISABLE_MODE_POV */
   }
cleanup:
   return;
}

void client_local_poll_input(
   struct CLIENT* c, struct CHANNEL* l, struct INPUT* p
) {
   switch( c->gfx_mode ) {
   case MODE_TOPDOWN:
      mode_topdown_poll_input( c, l, p );
      break;
#ifndef DISABLE_MODE_POV
   case MODE_POV:
      //mode_pov_poll_input( c, l, p );
      mode_topdown_poll_input( c, l, p );
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
