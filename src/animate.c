
#define ANIMATE_C
#include "animate.h"

void animate_init() {
   hashmap_init( &animations );
}

static BOOL animate_del_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   bstring key_search = (bstring)arg;
   struct ANIMATION* a = (struct ANIMATION*)iter;
   if( NULL == arg || 0 == bstrcmp( idx->value.key, key_search ) ) {
      if( NULL != a ) {
         animate_free_animation( &a );
      }
      return TRUE;
   }
   return FALSE;
}

void animate_shutdown() {
   hashmap_remove_cb( &animations, animate_del_cb, NULL );
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

   new_frame = mem_alloc( 1, struct ANIMATION_FRAME );
   new_frame->next_frame = NULL;
   new_frame->duration = ms_per_frame;
   new_frame->flood_color = GRAPHICS_COLOR_TRANSPARENT;

   last_frame = animate_get_last_frame( a, target );

   if( NULL != last_frame ) {
      new_frame->x = last_frame->x;
      new_frame->y = last_frame->y;
      new_frame->width = last_frame->width;
      new_frame->height = last_frame->height;
      last_frame->next_frame = new_frame;
   } else {
      new_frame->x = target->virtual_x;
      new_frame->y = target->virtual_y;
      new_frame->width = target->w;
      new_frame->height = target->h;
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
   a->indefinite = FALSE;

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
   if( NULL == a->target ) {
      a->target = graphics_copy( target );
   }

   a->blocking = block;
   a->indefinite = FALSE;

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
      new_frame->width += inc;
   } else if( last_frame->width > end_w ) {
      new_frame->width -= inc;
   } else {
      width_adjusted = FALSE;
   }

   if( last_frame->height < end_h ) {
      new_frame->height += inc;
   } else if( last_frame->height > end_h ) {
      new_frame->height -= inc;
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

void animate_create_blink_color(
   struct ANIMATION* a, GRAPHICS* target, GRAPHICS_COLOR end_color,
   INTERVAL ms_per_frame, SCAFFOLD_SIZE reps, GFX_COORD_PIXEL inc, BOOL block
) {
   struct ANIMATION_FRAME* last_frame = NULL,
      * new_frame = NULL;

   /* TODO: Copy graphic target. */

   a->blocking = block;
   a->indefinite = FALSE;

   last_frame = animate_get_last_frame( a, target );
   new_frame = animate_new_last_frame( a, target, ms_per_frame );

   if( NULL == last_frame ) {
      /* This must be the first frame. Assume a rep is a cycle of 1 on/1 off. */
      reps *= 2;

      /* We need at least two frames, so recurse and finish. */
      animate_create_blink_color(
         a, target, end_color, ms_per_frame, reps, inc, block
      );
      goto cleanup;
   }

   if( 0 != reps % 2 ) {
      last_frame->flood_color = end_color;
   }

   if( 0 < reps ) {
      animate_create_blink_color(
         a, target, end_color, ms_per_frame, reps, inc, block
      );
   }

cleanup:
   return;
}

short animate_add_animation( struct ANIMATION* a, bstring key ) {
   /* TODO: What if key exists? */
   if( hashmap_put( &animations, key, a, FALSE ) ) {
      scaffold_print_error( &module, "Attempted to double-add animation...\n" );
      return 1;
   }
   return 0;
}

struct ANIMATION* animate_get_animation( bstring key ) {
   return hashmap_get( &animations, key );
}

void animate_cancel_animation( struct ANIMATION** a_out, bstring key ) {
   BOOL removed;
   struct ANIMATION* a = NULL;

   a = hashmap_get( &animations, key );

   if( NULL != a ) {
      removed = hashmap_remove( &animations, key );
      scaffold_assert( TRUE == removed );
      if( NULL != a_out ) {
         *a_out = a;
      } else {
         /* Just free it. */
         animate_free_animation( &a );
      }
   }
}

static void animate_free_animation_frame( struct ANIMATION_FRAME** frame ) {
   struct ANIMATION_FRAME* next_frame = (*frame)->next_frame;

   mem_free( *frame );

   if( NULL != next_frame ) {
      animate_free_animation_frame( &next_frame );
   }

cleanup:
   next_frame = NULL;
   return;
}

void animate_free_animation( struct ANIMATION** a ) {
   if( NULL != *a ) {
      graphics_surface_free( (*a)->target );
      animate_free_animation_frame( &((*a)->first_frame) );
      mem_free( *a );
      *a = NULL;
   }
}

static BOOL animate_cyc_ani_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct ANIMATION* a = (struct ANIMATION*)iter;
   struct GRAPHICS_TILE_WINDOW* twindow = (struct GRAPHICS_TILE_WINDOW*)arg;
   BOOL remove = FALSE;
   struct ANIMATION_FRAME* current_frame = NULL;

   if( TRUE == a->global ) {
      /* Leave this for the global cycler. */
      goto cleanup;
   }

   current_frame = a->current_frame;

   /* TODO: Use actual milliseconds. */
   if( current_frame->current < current_frame->duration ) {
      current_frame->current++;
   } else {
      /* The frame has expired. */
      a->current_frame = current_frame->next_frame;
   }

   if( NULL == a->current_frame && FALSE == a->indefinite ) {
      /* The animation has expired. */
      scaffold_print_debug(
         &module, "Animation finished: %b", idx->value.key
      );
      remove = TRUE;
      goto cleanup;
   } else if( NULL == a->current_frame && FALSE != a->indefinite ) {
      a->current_frame = a->first_frame;
   }

cleanup:
   return remove;
}

void animate_cycle_animations( struct GRAPHICS_TILE_WINDOW* twindow ) {
   hashmap_remove_cb( &animations, animate_cyc_ani_cb, twindow );
}

static void* animate_draw_ani_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct ANIMATION* a = (struct ANIMATION*)iter;
   struct GRAPHICS_TILE_WINDOW* twindow = (struct GRAPHICS_TILE_WINDOW*)arg;
   GFX_COORD_PIXEL
      centered_x,
      centered_y,
      x_adjustment,
      y_adjustment,
      target_w = a->target->w,
      target_h = a->target->h;

   x_adjustment = (target_w / 2) - (a->current_frame->width / 2);
   y_adjustment = (target_h / 2) - (a->current_frame->height / 2);

   centered_x = a->current_frame->x + x_adjustment;
   centered_y = a->current_frame->y + y_adjustment;

   /* TODO: Blit fill color masked with shape. */

//#ifdef GRAPHICS_C
   graphics_blit_stretch(
      twindow->g,
      centered_x,
      centered_y,
      a->current_frame->width,
      a->current_frame->height,
      a->target
   );
//#endif /* GRAPHICS_C */
}

void animate_draw_animations( struct GRAPHICS_TILE_WINDOW* twindow ) {
   hashmap_iterate( &animations, animate_draw_ani_cb, twindow );
}

static void* animate_blocker_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
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
