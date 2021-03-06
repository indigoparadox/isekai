
#ifndef ANIMATE_H
#define ANIMATE_H

#include "scaffold.h"
#include "graphics.h"
#include "client.h"

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
   bool blocking;
   bool indefinite;
   GRAPHICS* target;
   bool global; /* If true, update frame current centrally. */
};

void animate_init();
void animate_shutdown();
void animate_create_movement(
   struct ANIMATION* a, GRAPHICS* target,
   GFX_COORD_PIXEL end_x, GFX_COORD_PIXEL end_y,
   INTERVAL ms_per_frame, GFX_COORD_PIXEL inc, bool block
);
void animate_create_resize(
   struct ANIMATION* a, GRAPHICS* target,
   GFX_COORD_PIXEL end_w, GFX_COORD_PIXEL end_h,
   INTERVAL ms_per_frame, GFX_COORD_PIXEL inc, bool block
);
void animate_create_blink_color(
   struct ANIMATION* a, GRAPHICS* target, GRAPHICS_COLOR end_color,
   INTERVAL ms_per_frame, SCAFFOLD_SIZE reps, GFX_COORD_PIXEL inc, bool block
);
short animate_add_animation( struct ANIMATION* a, bstring key )
#ifdef USE_GNUC_EXTENSIONS
__attribute__ ((warn_unused_result))
#endif /* USE_GNUC_EXTENSIONS */
;
struct ANIMATION* animate_get_animation( bstring key );
void animate_cancel_animation( struct ANIMATION** a, bstring key );
void animate_cleanup_animation( struct ANIMATION* a );
void animate_free_animation( struct ANIMATION** a );
void animate_cycle_animations( GRAPHICS* g );
void animate_draw_animations( GRAPHICS* g );
bool animate_is_blocking();

#ifdef ANIMATE_C
static struct HASHMAP* animations;
#else

#endif /* ANIMATE_C */

#endif /* ANIMATE_H */
