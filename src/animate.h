
#ifndef ANIMATE_H
#define ANIMATE_H

#include "scaffold.h"
#include "graphics.h"
#include "hashmap.h"

struct ANIMATION_FRAME {
   INTERVAL duration;
   INTERVAL current; /* Number of ms displayed so far. */
   GFX_COORD_PIXEL x;
   GFX_COORD_PIXEL y;
   GFX_COORD_PIXEL width;
   GFX_COORD_PIXEL height;
   GRAPHICS_COLOR flood_color;
   struct ANIMATION_FRAME* next_frame;
};

struct ANIMATION {
   struct ANIMATION_FRAME* first_frame;
   struct ANIMATION_FRAME* current_frame;
   BOOL blocking;
   BOOL indefinite;
   GRAPHICS* target;
   BOOL global; /* If TRUE, update frame current centrally. */
};

void animate_init();
void animate_shutdown();
void animate_create_movement(
   struct ANIMATION* a, GRAPHICS* target,
   GFX_COORD_PIXEL end_x, GFX_COORD_PIXEL end_y,
   INTERVAL ms_per_frame, GFX_COORD_PIXEL inc, BOOL block
);
void animate_create_resize(
   struct ANIMATION* a, GRAPHICS* target,
   GFX_COORD_PIXEL end_w, GFX_COORD_PIXEL end_h,
   INTERVAL ms_per_frame, GFX_COORD_PIXEL inc, BOOL block
);
void animate_create_blink_color(
   struct ANIMATION* a, GRAPHICS* target, GRAPHICS_COLOR end_color,
   INTERVAL ms_per_frame, SCAFFOLD_SIZE reps, GFX_COORD_PIXEL inc, BOOL block
);
void animate_add_animation( struct ANIMATION* a, bstring key );
struct ANIMATION* animate_get_animation( bstring key );
void animate_cancel_animation( struct ANIMATION** a, bstring key );
void animate_free_animation( struct ANIMATION** a );
void animate_cycle_animations( struct GRAPHICS_TILE_WINDOW* twindow );
void animate_draw_animations( struct GRAPHICS_TILE_WINDOW* twindow );
BOOL animate_is_blocking();

#ifdef ANIMATE_C
SCAFFOLD_MODULE( "animate.c" );
static struct HASHMAP animations;
#else

#endif /* ANIMATE_C */

#endif /* ANIMATE_H */
