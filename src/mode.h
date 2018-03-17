
#ifndef MODE_H
#define MODE_H

typedef enum {
   MODE_TOPDOWN,
   MODE_POV
}MODE;

#include "input.h"
#include "bstrlib/bstrlib.h"
#include "vector.h"
#include "graphics.h"
#include "client.h"

struct CHANNEL;

void mode_topdown_update(
   struct CLIENT* c,
   struct CHANNEL* l,
   struct GRAPHICS_TILE_WINDOW* twindow
);
void mode_topdown_draw(
   struct CLIENT* c,
   struct CHANNEL* l,
   struct GRAPHICS_TILE_WINDOW* twindow
);
void mode_topdown_poll_input( struct CLIENT* c, struct CHANNEL* l, struct INPUT* p );
void mode_topdown_free( struct CLIENT* c );

void mode_pov_update(
   struct CLIENT* c,
   struct CHANNEL* l,
   struct GRAPHICS_TILE_WINDOW* twindow
);
void mode_pov_draw(
   struct CLIENT* c,
   struct CHANNEL* l,
   struct GRAPHICS_TILE_WINDOW* twindow
);
void mode_pov_poll_input( struct CLIENT* c, struct CHANNEL* l, struct INPUT* p );
void mode_pov_free( struct CLIENT* c );
void client_local_update(
   struct CLIENT* c,
   struct CHANNEL* l,
   struct GRAPHICS_TILE_WINDOW* twindow
);

void client_local_draw(
   struct CLIENT* c,
   struct CHANNEL* l,
   struct GRAPHICS_TILE_WINDOW* twindow
);
void client_local_poll_input( struct CLIENT* c, struct CHANNEL* l, struct INPUT* p );
void client_local_free( struct CLIENT* c );

#ifdef MODE_C
extern struct tagbstring str_client_cache_path;
extern struct tagbstring str_wid_debug_tiles_pos;
extern struct tagbstring str_client_window_id_chat;
extern struct tagbstring str_client_window_title_chat;
extern struct tagbstring str_client_control_id_chat;
extern struct tagbstring str_client_window_id_inv;
extern struct tagbstring str_client_window_title_inv;
extern struct tagbstring str_client_control_id_inv_self;
extern struct tagbstring str_client_control_id_inv_ground;
SCAFFOLD_MODULE( "mode.c" );
#endif /* MODE_C */

#endif /* MODE_H */
