
#define ANIMATE_C
#include "animate.h"

void animate_init() {
   animations = hashmap_new();
}

static VBOOL animate_del_cb( bstring idx, void* iter, void* arg ) {
   bstring key_search = (bstring)arg;
   VBOOL remove = VFALSE;
   struct ANIMATION* a = (struct ANIMATION*)iter;
   if( NULL == arg || 0 == bstrcmp( idx, key_search ) ) {
      if( NULL != a ) {
         animate_free_animation( &a );
      }
      remove = VTRUE;
   }
/* cleanup: */
   return remove;
}

void animate_shutdown() {
   hashmap_remove_cb( animations, animate_del_cb, NULL );
   hashmap_free( &animations );
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
   INTERVAL ms_per_frame, GFX_COORD_PIXEL inc, VBOOL block
) {
   struct ANIMATION_FRAME* last_frame = NULL,
      * new_frame = NULL;
   VBOOL x_adjusted = VTRUE;

   /* TODO: Copy graphic target. */

   a->blocking = block;
   a->indefinite = VFALSE;

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
      x_adjusted = VFALSE;
   }

   if( last_frame->y < end_y ) {
      last_frame->next_frame->y += inc;
   } else if( last_frame->y > end_y ) {
      last_frame->next_frame->y -= inc;
   } else {
      if( VFALSE == x_adjusted ) {
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
   INTERVAL ms_per_frame, GFX_COORD_PIXEL inc, VBOOL block
) {
   struct ANIMATION_FRAME* last_frame = NULL,
      * new_frame = NULL;
   VBOOL width_adjusted = VTRUE;

   /* Copy graphic target. */
   if( NULL == a->target ) {
      a->target = graphics_copy( target );
   }

   a->blocking = block;
   a->indefinite = VFALSE;

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
      width_adjusted = VFALSE;
   }

   if( last_frame->height < end_h ) {
      new_frame->height += inc;
   } else if( last_frame->height > end_h ) {
      new_frame->height -= inc;
   } else {
      if( VFALSE == width_adjusted ) {
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
   INTERVAL ms_per_frame, SCAFFOLD_SIZE reps, GFX_COORD_PIXEL inc, VBOOL block
) {
   struct ANIMATION_FRAME* last_frame = NULL;
      /* * new_frame = NULL; */

   /* TODO: Copy graphic target. */

   a->blocking = block;
   a->indefinite = VFALSE;

   last_frame = animate_get_last_frame( a, target );
   /* new_frame = animate_new_last_frame( a, target, ms_per_frame ); */

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
   if( hashmap_put( animations, key, a, VFALSE ) ) {
      lg_error( __FILE__, "Attempted to double-add animation...\n" );
      return 1;
   }
   return 0;
}

struct ANIMATION* animate_get_animation( bstring key ) {
   return hashmap_get( animations, key );
}

void animate_cancel_animation( struct ANIMATION** a_out, bstring key ) {
   VBOOL removed;
   struct ANIMATION* a = NULL;

   a = hashmap_get( animations, key );

   if( NULL != a ) {
      lg_debug( __FILE__, "Removing \"%b\" animation...\n", key );
      removed = hashmap_remove( animations, key );
      scaffold_assert( 1 == removed );
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

   /* Free the current frame. */
   mem_free( *frame );

   if( NULL != next_frame ) {
      /* Recurse into the next frame we saved above. */
      animate_free_animation_frame( &next_frame );
   }

   return;
}

void animate_cleanup_animation( struct ANIMATION* a ) {
   if( NULL == a ) {
      goto cleanup;
   }
   graphics_surface_free( a->target );
   animate_free_animation_frame( &(a->first_frame) );
cleanup:
   return;
}

void animate_free_animation( struct ANIMATION** a ) {
   if( NULL != *a ) {
      goto cleanup;
   }
   animate_cleanup_animation( *a );
   mem_free( *a );
   *a = NULL;
cleanup:
   return;
}

static VBOOL animate_cyc_ani_cb( bstring idx, void* iter, void* arg ) {
   struct ANIMATION* a = (struct ANIMATION*)iter;
   VBOOL remove = VFALSE;
   struct ANIMATION_FRAME* current_frame = NULL;

   if( VFALSE != a->global ) {
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

   if( NULL == a->current_frame && VFALSE == a->indefinite ) {
      /* The animation has expired. */
      lg_debug(
         __FILE__, "Animation finished: %b", idx
      );
      animate_free_animation( &a );
      remove = VTRUE;
      goto cleanup;
   } else if( NULL == a->current_frame && VFALSE != a->indefinite ) {
      a->current_frame = a->first_frame;
   }

cleanup:
   return remove;
}

void animate_cycle_animations( GRAPHICS* g ) {
   hashmap_remove_cb( animations, animate_cyc_ani_cb, g );
}

static void* animate_draw_ani_cb( bstring idx, void* iter, void* arg ) {
   struct ANIMATION* a = (struct ANIMATION*)iter;
   GRAPHICS* g = (GRAPHICS*)arg;
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

   graphics_blit_stretch(
      g,
      centered_x,
      centered_y,
      a->current_frame->width,
      a->current_frame->height,
      a->target
   );

   return NULL;
}

void animate_draw_animations( GRAPHICS* g ) {
   hashmap_iterate( animations, animate_draw_ani_cb, g );
}

static void* animate_blocker_cb( bstring idx, void* iter, void* arg ) {
   struct ANIMATION* a = (struct ANIMATION*)iter;
   if( VFALSE != a->blocking ) {
      return a;
   }
   return NULL;
}

VBOOL animate_is_blocking() {
   struct ANIMATION* blocker = NULL;
   blocker = hashmap_iterate( animations, animate_blocker_cb, NULL );
   if( NULL != blocker ) {
      return VTRUE;
   } else {
      return VFALSE;
   }
}
