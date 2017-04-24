
#define ANIMATE_C
#include "animate.h"

void animate_init() {
   hashmap_init( &animations );
}

void animate_shutdown() {
   hashmap_cleanup( &animations );
}

/** \brief Get the last existing frame (if any) in an animation.
 * \param
 * \param
 * \return The last existing frame in this animation or NULL if no frames
 *         present.
 */
static struct ANIMATION_FRAME* animate_get_last_frame(
   struct ANIMATION* a, GRAPHICS* target
) {
   struct ANIMATION_FRAME* last_frame = NULL;

   if( NULL != a->first_frame ) {
      last_frame = a->first_frame;

      while( NULL != last_frame->next_frame ) {
         last_frame = last_frame->next_frame;
      }
   }

   return last_frame;
}

/** \brief Create a new last frame at the end of the animation.
 * \param
 * \param
 * \return The new last frame created in the animation.
 */
static struct ANIMATION_FRAME* animate_new_last_frame(
   struct ANIMATION* a, GRAPHICS* target, INTERVAL ms_per_frame
) {
   struct ANIMATION_FRAME* last_frame = NULL,
      * new_frame = NULL;

   new_frame = scaffold_alloc( 1, struct ANIMATION_FRAME );
   new_frame->next_frame = NULL;
   new_frame->x = target->virtual_x;
   new_frame->y = target->virtual_y;
   new_frame->duration = ms_per_frame;
   new_frame->width = target->w;
   new_frame->height = target->h;

   last_frame = animate_get_last_frame( a, target );

   if( NULL != last_frame ) {
      last_frame->next_frame = new_frame;
   } else {
      a->first_frame = new_frame;
      a->current_frame = new_frame;
   }

   return new_frame;
}

void animate_create_movement(
   struct ANIMATION* a, GRAPHICS* target,
   GFX_COORD_PIXEL end_x, GFX_COORD_PIXEL end_y,
   INTERVAL ms_per_frame, GFX_COORD_PIXEL inc, BOOL block
) {
   struct ANIMATION_FRAME* last_frame = NULL,
      * new_frame = NULL;
   BOOL x_adjusted = TRUE;

   /* TODO: Copy graphic target. */

   a->blocking = block;

   last_frame = animate_get_last_frame( a, target );
   new_frame = animate_new_last_frame( a, target, ms_per_frame );

   if( NULL == last_frame ) {
      /* We need at least two frames, so recurse and finish. */
      animate_create_resize(
         a, target, end_x, end_y, ms_per_frame, inc, block
      );
      goto cleanup;
   }

   if( last_frame->x < end_x ) {
      last_frame->next_frame->x += inc;
   } else if( last_frame->x > end_x ) {
      last_frame->next_frame->x -= inc;
   } else {
      x_adjusted = FALSE;
   }

   if( last_frame->y < end_y ) {
      last_frame->next_frame->y += inc;
   } else if( last_frame->y > end_y ) {
      last_frame->next_frame->y -= inc;
   } else {
      if( FALSE == x_adjusted ) {
         /* If neither width nor height was adjusted, this is pointless. */
         goto cleanup;
      }
   }

   if( new_frame->x != end_x || new_frame->y != end_y ) {
      animate_create_resize(
         a, target, end_x, end_y, ms_per_frame, inc, block
      );
   }

cleanup:
   return;
}

void animate_create_resize(
   struct ANIMATION* a, GRAPHICS* target,
   GFX_COORD_PIXEL end_w, GFX_COORD_PIXEL end_h,
   INTERVAL ms_per_frame, GFX_COORD_PIXEL inc, BOOL block
) {
   struct ANIMATION_FRAME* last_frame = NULL,
      * new_frame = NULL;
   BOOL width_adjusted = TRUE;

   /* Copy graphic target. */
   a->target = graphics_copy( target );

   a->blocking = block;

   last_frame = animate_get_last_frame( a, target );
   new_frame = animate_new_last_frame( a, target, ms_per_frame );

   if( NULL == last_frame ) {
      /* We need at least two frames, so recurse and finish. */
      animate_create_resize(
         a, target, end_w, end_h, ms_per_frame, inc, block
      );
      goto cleanup;
   }

   if( last_frame->width < end_w ) {
      last_frame->next_frame->width += inc;
   } else if( last_frame->width > end_w ) {
      last_frame->next_frame->width -= inc;
   } else {
      width_adjusted = FALSE;
   }

   if( last_frame->height < end_h ) {
      last_frame->next_frame->height += inc;
   } else if( last_frame->height > end_h ) {
      last_frame->next_frame->height -= inc;
   } else {
      if( FALSE == width_adjusted ) {
         /* If neither width nor height was adjusted, this is pointless. */
         goto cleanup;
      }
   }

   if( new_frame->width != end_w || new_frame->height != end_h ) {
      animate_create_resize(
         a, target, end_w, end_h, ms_per_frame, inc, block
      );
   }

cleanup:
   return;
}

void animate_add_animation( struct ANIMATION* a, bstring key ) {
   /* TODO: What if key exists? */
   hashmap_put( &animations, key, a );
}

void animate_cancel_animation( struct ANIMATION** a, bstring key ) {
   BOOL removed;
   *a = hashmap_get( &animations, key );
   if( NULL != *a ) {
      removed = hashmap_remove( &animations, key );
      scaffold_assert( TRUE == removed );
   }
}

void animate_free_animation( struct ANIMATION** a ) {
   if( NULL != *a ) {
      scaffold_free( *a );
      *a = NULL;
   }
}

static
BOOL animate_cyc_ani_cb( struct CONTAINER_IDX* idx, void* iter, void* arg ) {
   struct ANIMATION* a = (struct ANIMATION*)iter;
   struct GRAPHICS_TILE_WINDOW* twindow = (struct GRAPHICS_TILE_WINDOW*)arg;
   BOOL remove = FALSE;

   if( TRUE == a->global ) {
      /* Leave this for the global cycler. */
      goto cleanup;
   }

   if( NULL == a->current_frame ) {
      /* The animation has expired. */
      remove = TRUE;
      goto cleanup;
   }

   /* TODO: Use actual milliseconds. */
   if( a->current_frame->current < a->current_frame->duration ) {
      a->current_frame->current++;
   } else {
      /* The frame has expired. */
      a->current_frame = a->current_frame->next_frame;
   }

cleanup:
   return remove;
}

void animate_cycle_animations( struct GRAPHICS_TILE_WINDOW* twindow ) {
   hashmap_remove_cb( &animations, animate_cyc_ani_cb, twindow );
}

static
void* animate_draw_ani_cb( struct CONTAINER_IDX* idx, void* iter, void* arg ) {
   struct ANIMATION* a = (struct ANIMATION*)iter;
   struct GRAPHICS_TILE_WINDOW* twindow = (struct GRAPHICS_TILE_WINDOW*)arg;

   graphics_blit_stretch(
      twindow->g,
      a->current_frame->x,
      a->current_frame->y,
      a->current_frame->width,
      a->current_frame->height,
      a->target
   );
}

void animate_draw_animations( struct GRAPHICS_TILE_WINDOW* twindow ) {
   hashmap_iterate( &animations, animate_draw_ani_cb, twindow );
}

static
void* animate_blocker_cb( struct CONTAINER_IDX* idx, void* iter, void* arg ) {
   struct ANIMATION* a = (struct ANIMATION*)iter;
   if( FALSE != a->blocking ) {
      return a;
   }
   return NULL;
}

BOOL animate_is_blocking() {
   struct ANIMATION* blocker = NULL;
   blocker = hashmap_iterate( &animations, animate_blocker_cb, NULL );
   if( NULL != blocker ) {
      return TRUE;
   } else {
      return FALSE;
   }
}
