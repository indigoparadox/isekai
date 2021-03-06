
#define MOBILE_C
#include "mobile.h"

#include "proto.h"
#include "chunker.h"
#include "callback.h"
#include "datafile.h"
#include "vm.h"
#include "backlog.h"
#include "channel.h"
#include "ipc.h"
#include "files.h"
#include "twindow.h"
#include "action.h"
#include "audition.h"
#include "plugin.h"

#ifdef USE_MOBILE_FRAME_COUNTER
static uint8_t mobile_frame_counter = 0;
#endif /* USE_MOBILE_FRAME_COUNTER */
#ifdef USE_MOBILE_MOVE_COUNTER
static uint8_t mobile_move_counter = 0;
#endif /* USE_MOBILE_MOVE_COUNTER */

struct MOBILE {
   struct REF refcount;
   SERIAL serial;
   struct CLIENT* owner;
   TILEMAP_COORD_TILE x;
   TILEMAP_COORD_TILE y;
   TILEMAP_COORD_TILE prev_x;
   TILEMAP_COORD_TILE prev_y;
   GFX_COORD_PIXEL sprite_width;
   GFX_COORD_PIXEL sprite_height;
   GFX_COORD_PIXEL sprite_display_width;
   GFX_COORD_PIXEL sprite_display_height;
   GFX_COORD_PIXEL steps_inc;
   GFX_COORD_PIXEL steps_inc_default;
   GFX_COORD_PIXEL steps_remaining;
   bstring sprites_filename;
   GRAPHICS* sprites;
   /* MOBILE_FRAME_ALT frame_alt;
   MOBILE_FRAME frame; */
   uint8_t current_frame;
   MOBILE_FACING facing;
   bool animation_reset;
   bstring display_name;
   bstring def_filename;
   bstring mob_id;
   struct CHANNEL* channel;
   MOBILE_TYPE type;
   struct VECTOR* sprite_defs;
   struct HASHMAP* ani_defs;
   struct HASHMAP* script_defs;
   struct MOBILE_ANI_DEF* current_animation;
   struct VECTOR* walking_queue; /* Only used for certain modes. Think RTS. */
   struct AUDI_CONTEXT* ai;
   bstring mode;
   struct HASHMAP* mode_data;
#ifdef USE_ITEMS
   struct VECTOR* items;
#endif // USE_ITEMS
   bool initialized;
#ifdef USE_VM
   struct VM_CADDY* vm_caddy;
#ifdef USE_TURNS
   SCAFFOLD_SIZE vm_tick_prev;
#endif /* USE_TURNS */
#endif /* USE_VM */
};

void mobile_gen_serial( struct MOBILE* o, struct VECTOR* mobiles ) {
   do {
      o->serial = SERIAL_MIN + rng_max( SERIAL_MAX );
   } while( 0 == o->serial || NULL != vector_get( mobiles, o->serial ) );
}

struct MOBILE* mobile_new(
   bstring mob_id, TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y
) {
   struct MOBILE* o = NULL;

   o = mem_alloc( 1, struct MOBILE );
   lgc_null( o );

   mobile_init( o, mob_id, x, y );

cleanup:
   return o;
}

static void mobile_cleanup( const struct REF* ref ) {
   struct MOBILE* o = scaffold_container_of( ref, struct MOBILE, refcount );

#ifdef ENABLE_LOCAL_CLIENT
   if( NULL != o->sprites ) {
      graphics_surface_free( o->sprites );
   }
#endif /* ENABLE_LOCAL_CLIENT */
   if( NULL != o->owner ) {
      client_clear_puppet( o->owner );
      o->owner = NULL;
   }
   if( NULL != o->channel ) {
      channel_free( o->channel );
      lg_debug(
         __FILE__,
         "Mobile %d decreased refcount of channel %b to: %d\n",
         o->serial, o->channel->name, o->channel->refcount.count
      );
      o->channel = NULL;
   }

   vector_remove_cb( o->sprite_defs, callback_free_generic, NULL );
   vector_free( &(o->sprite_defs) );

   /* vector_remove_cb( &(o->speech_backlog), callback_free_strings, NULL );
   vector_cleanup( &(o->speech_backlog) ); */

   hashmap_remove_cb( o->ani_defs, callback_free_ani_defs, NULL );
   hashmap_free( &(o->ani_defs) );

   audition_node_free( &(o->ai) );

   lg_debug(
      __FILE__,
      "Mobile %b (%d) destroyed.\n",
      o->display_name, o->serial
   );

   bdestroy( o->display_name );
   mem_free( o );
}

void mobile_free( struct MOBILE* o ) {
   refcount_dec( o, "mobile" );
}

void mobile_animation_free( struct MOBILE_ANI_DEF* animation ) {
   vector_free( &(animation->frames) );
   bdestroy( animation->name );
}

void mobile_init(
   struct MOBILE* o, const bstring mob_id, TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y
) {
   int bstr_ret = 0;

   ref_init( &(o->refcount), mobile_cleanup );

   //assert( NULL != mob_id );

   o->ai = audition_context_new();
   o->sprites_filename = NULL;
   o->serial = 0;
   o->channel = NULL;
   o->display_name = bfromcstralloc( CLIENT_NAME_ALLOC, "" );
   o->current_frame = 0;
   o->steps_inc_default = MOBILE_STEPS_INCREMENT;
   o->owner = NULL;
   o->animation_reset = false;
#ifdef USE_TURNS
   o->vm_tick_prev = 0;
#endif /* USE_TURNS */

#ifdef USE_VM
   vm_caddy_new( o->vm_caddy );
#endif /* USE_VM */

   o->sprite_defs = vector_new();
   o->ani_defs = hashmap_new();
   o->script_defs = hashmap_new();
#ifdef USE_ITEMS
   vector_init( &(o->items) );
#endif // USE_ITEMS

   o->mode_data = hashmap_new();

   o->x = x;
   o->prev_x = x;
   o->y = y;
   o->prev_y = y;
   if( NULL != mob_id ) {
      o->mob_id = bstrcpy( mob_id );
   } else {
      o->mob_id = bfromcstr( "" );
   }

   /* We always need the filename to fetch the file with a chunker. */
   o->def_filename = bfromcstr( "mobs" );
   lgc_null( o->def_filename );
   files_join_path( o->def_filename, o->mob_id );

   lg_debug( __FILE__, "scaffold_error: %d\n", lgc_error );
   lgc_nonzero( lgc_error );

#ifdef USE_EZXML
   bstr_ret = bcatcstr( o->def_filename, ".xml" );
   lgc_nonzero( bstr_ret );
#endif /* USE_EZXML */

cleanup:
   return;
}

void mobile_load_local( struct MOBILE* o ) {

   /* Refactored this out of the init because all mobiles need an ID, but
    * client-side mobiles don't need to load from the file system.
    */

   size_t bytes_read = 0,
      mobdata_size = 0;
   BYTE* mobdata_buffer = NULL;
   bstring mobdata_path = NULL;

   assert( NULL != o );
   assert( NULL != o->mob_id );
   assert( NULL != o->def_filename );

   mobdata_path = files_root( o->def_filename );

#ifdef USE_EZXML

   /* TODO: Support other mobile formats. */
   lg_debug(
      __FILE__, "Looking for XML data in: %s\n", bdata( mobdata_path )
   );
   bytes_read = files_read_contents(
      mobdata_path, &mobdata_buffer, &mobdata_size
   );
   lgc_null_msg( mobdata_buffer, "Unable to load mobile data." );
   lgc_zero_msg( bytes_read, "Unable to load mobile data." );

   datafile_parse_ezxml_string(
      o, mobdata_buffer, mobdata_size, false, DATAFILE_TYPE_MOBILE, mobdata_path
   );

#endif /* USE_EZXML */

   o->initialized = true;

cleanup:
   bdestroy( mobdata_path );
   if( NULL != mobdata_buffer ) {
      mem_free( mobdata_buffer );
   }
   return;
}

#ifdef ENABLE_LOCAL_CLIENT

static GFX_COORD_PIXEL mobile_calculate_terrain_sprite_height(
   struct TILEMAP* t, GFX_COORD_PIXEL sprite_height_in,
   TILEMAP_COORD_TILE x_2, TILEMAP_COORD_TILE y_2
) {
   struct VECTOR* tiles_end = NULL;
   struct TILEMAP_POSITION pos_end;
   struct TILEMAP_TILE_DATA* tile_iter = NULL;
   uint8_t i, j;
   struct TILEMAP_TERRAIN_DATA* terrain_iter = NULL;
   SCAFFOLD_SIZE sprite_height_out = sprite_height_in;

   pos_end.x = x_2;
   pos_end.y = y_2;

   assert( TILEMAP_SENTINAL == t->sentinal );

   /* Fetch the destination tile on all layers. */
   tiles_end =
      vector_iterate_v( t->layers, callback_get_tile_stack_l, &pos_end );

   for( i = 0 ; vector_count( tiles_end ) > i ; i++ ) {
      tile_iter = vector_get( tiles_end, i );
      lgc_null( tile_iter );

      for( j = 0 ; 4 > j ; j++ ) {
         /* TODO: Implement terrain slow-down. */
         terrain_iter = tile_iter->terrain[j];
         if( NULL == terrain_iter ) { continue; }
         if(
            terrain_iter->cutoff != 0 &&
            sprite_height_in / terrain_iter->cutoff < sprite_height_out
         ) {
            sprite_height_out = sprite_height_in / terrain_iter->cutoff;
         }
      }
   }

cleanup:
   if( NULL != tiles_end ) {
      vector_cleanup_force( tiles_end );
      mem_free( tiles_end );
   }
   return sprite_height_out;
}

void mobile_animate( struct MOBILE* o ) {

   if( NULL == o || NULL == o->current_animation ) {
      goto cleanup;
   }

   if( o->current_frame >= vector_count( o->current_animation->frames ) - 1 ) {
      o->current_frame = 0;
   } else {
      o->current_frame++;
   }

   /* TODO: Enforce walking speed server-side. */
   if( 0 != o->steps_remaining ) {
      /* If steps_remaining is positive, steps_inc should be negative. */
      o->steps_remaining += o->steps_inc;
      /* Clamp to zero for odd increments. */
      if(
         (0 < o->steps_inc && 0 < o->steps_remaining) ||
         (0 > o->steps_inc && 0 > o->steps_remaining)
      ) {
         o->steps_remaining = 0;
      }

      if(
         (0 < o->steps_inc && MOBILE_STEPS_HALF > o->steps_remaining) ||
         (0 > o->steps_inc && (-1 * MOBILE_STEPS_HALF) < o->steps_remaining)
      ) {
         o->sprite_display_height =
            mobile_calculate_terrain_sprite_height(
               o->channel->tilemap, o->sprite_height,
               o->x, o->y );
      }

      if( 0 == o->steps_remaining ) {
         /* This could have been set to zero by the increment above. */
         o->prev_x = o->x;
         o->prev_y = o->y;
      }
   }

cleanup:
   return;
}

GFX_COORD_PIXEL
mobile_get_steps_remaining_x( const struct MOBILE* o, bool reverse ) {
   GFX_COORD_PIXEL steps_out = 0;
   if( o->prev_x != o->x ) {
      if( !reverse ) {
         steps_out = o->steps_remaining;
      } else {
         steps_out = -1 * o->steps_remaining;
      }
   }
   return steps_out;
}

GFX_COORD_PIXEL
mobile_get_steps_remaining_y( const struct MOBILE* o, bool reverse ) {
   GFX_COORD_PIXEL steps_out = 0;
   if( o->prev_y != o->y ) {
      if( !reverse ) {
         steps_out = o->steps_remaining;
      } else {
         steps_out = -1 * o->steps_remaining;
      }
   }
   return steps_out;
}

/** \brief Put the spritesheet position for gid on the spritesheet g_sprites in
 *         the rectangle sprite_frame. The rectangle must already have the
 *         correct sprite width and height set when passed.
 * \param
 * \param
 */
void mobile_get_spritesheet_pos_ortho(
   struct MOBILE* o, GRAPHICS_RECT* sprite_frame, size_t gid
) {
   int tiles_wide = 0;
   GRAPHICS* g_sprites = NULL;

   lgc_null( o );
   g_sprites = o->sprites;
   lgc_null( g_sprites );

   tiles_wide = g_sprites->w / sprite_frame->w;

   sprite_frame->y = ((gid) / tiles_wide) * sprite_frame->h;
   sprite_frame->x = ((gid) % tiles_wide) * sprite_frame->w;

cleanup:
   return;
}

void mobile_draw_ortho( struct MOBILE* o, struct CLIENT* local_client, struct TWINDOW* twindow ) {
   GFX_COORD_PIXEL
      pix_x,
      pix_y,
      steps_remaining_x,
      steps_remaining_y;
   struct MOBILE_SPRITE_DEF* current_frame = NULL;
   struct MOBILE* o_player = NULL;
   GRAPHICS_RECT sprite_rect;
   struct TILEMAP* active_tilemap = NULL;

   scaffold_assert_client();

   assert( NULL != o );
   if( NULL == o->sprites_filename ) {
      /* Nothing to load, nothing to see here. */
      goto cleanup;
   }

   if( NULL == o->sprites ) {
      client_request_file_later( local_client, DATAFILE_TYPE_MOBILE_SPRITES, o->sprites_filename );
      goto cleanup;
   }

   if(
      o->x > twindow_get_max_x( twindow ) ||
      o->y > twindow_get_max_y( twindow ) ||
      o->x < twindow_get_min_x( twindow ) ||
      o->y < twindow_get_min_y( twindow ) ||
      NULL == o->current_animation
   ) {
      goto cleanup;
   }

   /* Figure out the window position to draw to. */
   /* TODO: Support variable sprite size. */
   /* TODO: This should use tilemap tile size. */
   pix_x = (MOBILE_SPRITE_SIZE * (o->x - twindow_get_x( twindow )));
   pix_y = (MOBILE_SPRITE_SIZE * (o->y - twindow_get_y( twindow )));

   o_player = client_get_puppet( local_client );

   if(
      mobile_is_local_player( o, local_client ) &&
      (TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN !=
         tilemap_inside_window_deadzone_x( o->x + 1, twindow ) &&
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP !=
         tilemap_inside_inner_map_x( o->x - 1, twindow ))
   ) {
      steps_remaining_x = mobile_get_steps_remaining_x( o, false );
      pix_x += steps_remaining_x;
   } else if( !mobile_is_local_player( o, local_client ) ) {
      steps_remaining_x = mobile_get_steps_remaining_x( o, false );
      pix_x += steps_remaining_x;
   }

   if(
      mobile_is_local_player( o, local_client ) &&
      (TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN !=
         tilemap_inside_window_deadzone_y( o->y + 1, twindow ) &&
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP !=
         tilemap_inside_window_deadzone_y( o->y - 1, twindow ))
   ) {
      steps_remaining_y = mobile_get_steps_remaining_y( o, false );
      pix_y += steps_remaining_y;
   } else if( !mobile_is_local_player( o, local_client ) ) {
      steps_remaining_y = mobile_get_steps_remaining_y( o, false );
      pix_y += steps_remaining_y;
   }

   /* Offset the player's  movement. */
   if(
      !mobile_is_local_player( o, local_client ) &&
      (TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN ==
         tilemap_inside_window_deadzone_x( o_player->x + 1, twindow ) ||
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP ==
         tilemap_inside_inner_map_x( o_player->x - 1, twindow ))
   ) {
      steps_remaining_x = mobile_get_steps_remaining_x( o_player, true );
      pix_x += steps_remaining_x;
   }

   if(
      !mobile_is_local_player( o, local_client ) &&
      (TILEMAP_EXCLUSION_OUTSIDE_RIGHT_DOWN ==
         tilemap_inside_window_deadzone_y( o_player->y + 1, twindow ) ||
      TILEMAP_EXCLUSION_OUTSIDE_LEFT_UP ==
         tilemap_inside_window_deadzone_y( o_player->y - 1, twindow ))
   ) {
      steps_remaining_y = mobile_get_steps_remaining_y( o_player, true );
      pix_y += steps_remaining_y;
   }

   /* Figure out the graphical sprite to draw from. */
   /* TODO: Support varied spritesheets. */
   current_frame = (struct MOBILE_SPRITE_DEF*)
      vector_get( o->current_animation->frames, o->current_frame );
   lgc_null( current_frame );

   sprite_rect.w = o->sprite_width;
   sprite_rect.h = o->sprite_height;
   mobile_get_spritesheet_pos_ortho(
      o, &sprite_rect, current_frame->id
   );

   /* Add dirty tiles to list before drawing. */
   active_tilemap = client_get_channel_active( local_client )->tilemap;
   tilemap_add_dirty_tile( active_tilemap, o->x, o->y );
   tilemap_add_dirty_tile( active_tilemap, o->prev_x, o->prev_y );

   graphics_blit_partial(
      twindow_get_screen( twindow ),
      pix_x, pix_y,
      sprite_rect.x, sprite_rect.y,
      o->sprite_display_width, o->sprite_display_height,
      o->sprites
   );

cleanup:
   return;
}

#endif /* ENABLE_LOCAL_CLIENT */

void mobile_set_channel( struct MOBILE* o, struct CHANNEL* l ) {
   if( NULL != o->channel ) {
      channel_free( o->channel );
      lg_debug(
         __FILE__,
         "Mobile %d decreased refcount of channel %b to: %d\n",
         o->serial, l->name, l->refcount.count
      );
   }
   o->channel = l;
   if( NULL != o->channel ) {
      refcount_inc( l, "channel" );
      lg_debug(
         __FILE__,
         "Mobile %d increased refcount of channel %b to: %d\n",
         o->serial, l->name, l->refcount.count
      );
   }
}

#if 0 // XXX
/** \brief Calculate the movement success between two tiles on the given
 *         mobile's present tilemap based on all available layers.
 * \param[in] x_1 Starting X.
 * \param[in] y_1 Starting Y.
 * \param[in] x_2 Finishing X.
 * \param[in] y_2 Finishing Y.
 * \return A MOBILE_UPDATE indicating action resulting.
 */
void mobile_calculate_terrain_result(
   struct TILEMAP* t, struct ACTION_PACKET* update,
   TILEMAP_COORD_TILE x_1, TILEMAP_COORD_TILE y_1, TILEMAP_COORD_TILE x_2, TILEMAP_COORD_TILE y_2
) {
   struct VECTOR* tiles_end = NULL;
   struct TILEMAP_POSITION pos_end;
   struct TILEMAP_TILE_DATA* tile_iter = NULL;
   uint8_t i, j;
   struct TILEMAP_TERRAIN_DATA* terrain_iter = NULL;

   if( ACTION_OP_NONE == update_in ) {
      update_out = ACTION_OP_NONE;
      goto cleanup;
   }

   if( x_2 >= t->width || y_2 >= t->height || x_2 < 0 || y_2 < 0 ) {
      update_out = ACTION_OP_NONE;
      goto cleanup;
   }

   pos_end.x = x_2;
   pos_end.y = y_2;

   /* Fetch the source tile on all layers. */
   tiles_end =
      vector_iterate_v( t->layers, callback_get_tile_stack_l, &pos_end );
   if( NULL == tiles_end ) {
#ifdef DEBUG_VERBOSE
      lg_error(
         __FILE__, "No tileset loaded; unable to process move.\n" );
#endif /* DEBUG_VERBOSE */
      update_out = ACTION_OP_NONE;
      goto cleanup;
   }

   for( i = 0 ; vector_count( tiles_end ) > i ; i++ ) {
      tile_iter = vector_get( tiles_end, i );
      lgc_null( tile_iter );

      for( j = 0 ; 4 > j ; j++ ) {
         /* TODO: Implement terrain slow-down. */
         terrain_iter = tile_iter->terrain[j];
         if( NULL == terrain_iter ) { continue; }
         if( TILEMAP_MOVEMENT_BLOCK == terrain_iter->movement ) {
            update_out = ACTION_OP_NONE;
         }
      }
   }

cleanup:
   if( NULL != tiles_end ) {
      /* Force the count to 0 so we can delete it. */
      vector_cleanup_force( tiles_end );
      mem_free( tiles_end );
   }
   return;
}
#endif // 0

/** \brief This inserts a line of speech into the local speech buffer.
 *         It should be called by AI speech routines or the receiver for
 *         network chat. See the proto_send_msg* family for sending network
 *         chat.
 * \param[in] o      The mobile to be associated with the line of speech.
 * \param[in] speech The line of speech.
 */
void mobile_speak( struct MOBILE* o, bstring speech ) {
   bstring line_nick = NULL;

   lgc_null_msg(
      o->channel, "Mobile without channel cannot speak.\n"
   );

   if( NULL != o->owner ) {
      line_nick = client_get_nick( o->owner );
   } else {
      line_nick = o->display_name;
   }
   assert( NULL != line_nick );

#ifndef DISABLE_BACKLOG
   backlog_speak( line_nick, speech );
#endif /* !DISABLE_BACKLOG */

cleanup:
   return;
}

#ifdef ENABLE_LOCAL_CLIENT

/** \return true if the mobile is owned by the client that is currently
 *          locally connected in a hosted coop or single-player instance.
 */
SCAFFOLD_INLINE
bool mobile_is_local_player( struct MOBILE* o, struct CLIENT* c ) {
   if( NULL != o->owner && 0 == bstrcmp( client_get_nick( o->owner ), client_get_nick( c ) ) ) {
      return true;
   }
   return false;
}

/** \return true if the mobile is currently doing something (moving, etc).
 */
bool mobile_is_occupied( struct MOBILE* o ) {
   return NULL != o && o->steps_remaining > 0;
}

#endif /* ENABLE_LOCAL_CLIENT */

#ifdef USE_ITEMS

void mobile_add_item( struct MOBILE* o, struct ITEM* e ) {
   vector_add( &(o->items), e );
}

#endif // USE_ITEMS

struct CHANNEL* mobile_get_channel( const struct MOBILE* o ) {
   if( NULL == o ) {
      return NULL;
   }
   return o->channel;
}

/** \brief The next time the mobile is drawn, start from the first frame of the
 *         animation named by its current_animation. Useful when changing
 *         directions or attacking.
 */
void mobile_call_reset_animation( struct MOBILE* o ) {
   o->animation_reset = true;
}

/** \brief Perform a requested reset in a 2D (topdown/iso) mode. */
void mobile_do_reset_2d_animation( struct MOBILE* o ) {
   bstring ani_key_buffer = NULL;

   /* A mobile should almost always be playing an animation (walking, if      *
    * nothing else).                                                          */
   if( NULL != o->current_animation ) {
      /* The mobile should play an animation, so build its hash key. */
      ani_key_buffer = bformat(
         "%s-%s",
         bdata( o->current_animation->name ),
         str_mobile_facing[o->facing].data
      );
      assert( NULL != ani_key_buffer );
      /* Grab the animation from the mobiles linked animation list, if any. */
      if( NULL != hashmap_get( o->ani_defs, ani_key_buffer ) ) {
         o->current_animation = hashmap_get( o->ani_defs, ani_key_buffer );
      }
   }

   /* No more need to reset. It's done. */
   o->animation_reset = false;

/* cleanup: */
   bdestroy( ani_key_buffer );
}

#ifdef USE_VM

void mobile_vm_start( struct MOBILE* o ) {

   lgc_null( o->vm_caddy );

   lg_debug(
      __FILE__, "Starting script VM for mobile: %d (%b)\n",
      o->serial, o->mob_id
   );

   /* Setup the script caddy. */
   ((struct VM_CADDY*)o->vm_caddy)->exec_start = 0;
   ((struct VM_CADDY*)o->vm_caddy)->caller = o;
   ((struct VM_CADDY*)o->vm_caddy)->caller_type = VM_CALLER_MOBILE;

   vm_caddy_start( o->vm_caddy );

cleanup:
   return;
}

#endif /* USE_VM */

void mobile_update_coords(
   struct MOBILE* o, TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y
) {
   o->prev_x = o->x;
   o->prev_y = o->y;
   o->x = x;
   o->y = y;
}

int mobile_set_display_name( struct MOBILE* o, const bstring name ) {
   return bassign( o->display_name, name );
}


void mobile_set_owner( struct MOBILE* o, struct CLIENT* c ) {
   refcount_inc( o, "mobile" ); /* Add first, to avoid deletion. */
   if( NULL != o->owner ) {
      client_clear_puppet( o->owner );
   }
   o->owner = c;
}

void mobile_set_serial( struct MOBILE* o, SERIAL serial ) {
   lgc_null( o );
   o->serial = serial;
cleanup:
   return;
}

struct TILEMAP* mobile_get_tilemap( const struct MOBILE* o ) {
   if( NULL == o || NULL == o->channel ) {
      return NULL;
   }
   return o->channel->tilemap;
}

size_t mobile_set_sprite(
   struct MOBILE* o, size_t id, struct MOBILE_SPRITE_DEF* sprite
) {
   assert( NULL != o );
   return vector_set( o->sprite_defs, id, sprite, true );
}
bool mobile_add_animation(
   struct MOBILE* o, bstring name_dir, struct MOBILE_ANI_DEF* animation
) {
   if(
      HASHMAP_ERROR_NONE !=
      hashmap_put( o->ani_defs, name_dir, animation, false )
   ) {
      return false;
   }
   return true;
}

int mobile_set_id( struct MOBILE* o, bstring mob_id ) {
   int bstr_retval = 0;
   bstr_retval = bassign( o->mob_id, mob_id );
   return bstr_retval;
}

void mobile_set_sprite_width( struct MOBILE* o, GFX_COORD_PIXEL w ) {
   lgc_null( o );
   o->sprite_width = w;
   o->sprite_display_width = o->sprite_width;
cleanup:
   return;
}

void mobile_set_sprite_height( struct MOBILE* o, GFX_COORD_PIXEL h ) {
   lgc_null( o );
   o->sprite_height = h;
   o->sprite_display_height = o->sprite_height;
cleanup:
   return;
}

void mobile_set_sprites_filename( struct MOBILE* o, const bstring filename ) {
   lgc_null( o );
   lgc_null( filename );
   if( NULL != o->sprites_filename ) {
      bassign( o->sprites_filename, filename );
   } else {
      o->sprites_filename = bstrcpy( filename );
   }
cleanup:
   return;
}

void mobile_set_facing( struct MOBILE* o, MOBILE_FACING facing ) {
   lgc_null( o );
   lg_debug(
      __FILE__, "Setting mobile %b (%d) facing direction: %d\n",
      o->display_name, o->serial, facing
   );
   o->facing = facing;
cleanup:
   return;
}

void mobile_set_animation( struct MOBILE* o, bstring ani_key ) {
   struct MOBILE_ANI_DEF* ani_def = NULL;
   lgc_null( o );
   ani_def = (struct MOBILE_ANI_DEF*)hashmap_get( o->ani_defs, ani_key );
   lgc_null( ani_def );
   o->current_animation = ani_def;
   /* TODO: Don't die if this fails. */
   //assert( NULL != o->current_animation );
cleanup:
   return;
}

TILEMAP_COORD_TILE mobile_get_x( const struct MOBILE* o ) {
   if( NULL == o ) {
      return -1;
   }
   return o->x;
}

TILEMAP_COORD_TILE mobile_get_y( const struct MOBILE* o ) {
   if( NULL == o ) {
      return -1;
   }
   return o->y;
}

TILEMAP_COORD_TILE mobile_get_prev_x( const struct MOBILE* o ) {
   if( NULL == o ) {
      return -1;
   }
   return o->prev_x;
}

TILEMAP_COORD_TILE mobile_get_prev_y( const struct MOBILE* o ) {
   if( NULL == o ) {
      return -1;
   }
   return o->prev_y;
}

bstring mobile_get_sprites_filename( const struct MOBILE* o ) {
   if( NULL == o ) {
      return NULL;
   }
   return o->sprites_filename;
}

bstring mobile_get_id( const struct MOBILE* o ) {
   if( NULL == o ) {
      return NULL;
   }
   return o->mob_id;
}

struct GRAPHICS* mobile_get_sprites( const struct MOBILE* o ) {
   if( NULL == o ) {
      return NULL;
   }
   return o->sprites;
}

bstring mobile_get_def_filename( const struct MOBILE* o ) {
   if( NULL == o ) {
      return NULL;
   }
   return o->def_filename;
}

SERIAL mobile_get_serial( const struct MOBILE* o ) {
   if( NULL == o ) {
      return 0;
   }
   return o->serial;
}

struct MOBILE_SPRITE_DEF* mobile_get_sprite(
   const struct MOBILE* o, size_t id
) {
   if( NULL == o ) {
      return NULL;
   }
   return vector_get( o->sprite_defs, id );
}

GFX_COORD_PIXEL mobile_get_steps_remaining( const struct MOBILE* o ) {
   if( NULL == o ) {
      return 0;
   }
   return o->steps_remaining;
}

struct CLIENT* mobile_get_owner( const struct MOBILE* o ) {
   if( NULL == o ) {
      return NULL;
   }
   return o->owner;
}

void mobile_set_initialized( struct MOBILE* o, bool init ) {
   if( NULL == o ) {
      return;
   }
   o->initialized = init;
}

void mobile_set_type( struct MOBILE* o, MOBILE_TYPE type ) {
   lgc_null( o );
   o->type = type;
cleanup:
   return;
}

void mobile_set_sprites( struct MOBILE* o, struct GRAPHICS* sheet ) {
   lgc_null( o );
   assert( NULL == o->sprites );
   o->sprites = sheet;
cleanup:
   return;
}

void mobile_add_ref( struct MOBILE* o ) {
   refcount_inc( o, "mobile" );
}

/** \brief Calculate the movement success between two tiles on the given
 *         mobile's present tilemap based on neighboring mobiles.
 * \param[in] x_1 Starting X.
 * \param[in] y_1 Starting Y.
 * \param[in] x_2 Finishing X.
 * \param[in] y_2 Finishing Y.
 * \return true if the mobile can pass and false if not.
 */
bool mobile_calculate_mobile_collision(
   struct MOBILE* o,
   SCAFFOLD_SIZE x_1, SCAFFOLD_SIZE y_1, SCAFFOLD_SIZE x_2, SCAFFOLD_SIZE y_2
) {
   struct TILEMAP* t = NULL;
   struct CHANNEL* l = NULL;
   struct TILEMAP_POSITION pos;
   struct MOBILE* o_test = NULL;
   bool update_out = true; /* Pass by default. */

   l = mobile_get_channel( o );
   lgc_null( l );

   t = channel_get_tilemap( l );
   lgc_null( t );

   pos.x = x_2;
   pos.y = y_2;

   o_test = channel_search_mobiles( l, &pos );

   if( NULL != o_test ) {
      update_out = false;
   }

cleanup:
   return update_out;
}

bool mobile_is_walking( const struct MOBILE* o ) {
   bool res = false;

   if(
      mobile_get_x( o ) != mobile_get_prev_x( o ) ||
      mobile_get_y( o ) != mobile_get_prev_y( o )
   ) {
      res = true;
      lg_error(
         __FILE__,
         "Mobile (%d) already walking.\n", mobile_get_serial( o )
      );
   }

   return res;
}

GFX_COORD_PIXEL mobile_calculate_terrain_steps_inc(
   struct TILEMAP* t, GFX_COORD_PIXEL steps_inc_default,
   TILEMAP_COORD_TILE x_2, TILEMAP_COORD_TILE y_2
) {
   struct VECTOR* tiles_end = NULL;
   struct TILEMAP_POSITION pos_end;
   struct TILEMAP_TILE_DATA* tile_iter = NULL;
   uint8_t i, j;
   struct TILEMAP_TERRAIN_DATA* terrain_iter = NULL;
   GFX_COORD_PIXEL steps_inc_out = steps_inc_default;

   pos_end.x = x_2;
   pos_end.y = y_2;

   /* Fetch the destination tile on all layers. */
   tiles_end =
      vector_iterate_v( t->layers, callback_get_tile_stack_l, &pos_end );

   for( i = 0 ; vector_count( tiles_end ) > i ; i++ ) {
      tile_iter = vector_get( tiles_end, i );
      lgc_null( tile_iter );

      /* FIXME: Actually use the correct orientation movement info for the direction being moved in. */
      for( j = 0 ; 4 > j ; j++ ) {
         /* Implement terrain slow-down. */
         terrain_iter = tile_iter->terrain[j];
         if( NULL == terrain_iter ) { continue; }
         if(
            terrain_iter->movement != 0 &&
            steps_inc_default / terrain_iter->movement < steps_inc_out
         ) {
            steps_inc_out = steps_inc_default / terrain_iter->movement;
         } else if( 0 == terrain_iter->movement ) {
            steps_inc_out = 0;
         }
      }
   }

cleanup:
   if( NULL != tiles_end ) {
      vector_cleanup_force( tiles_end );
      mem_free( tiles_end );
   }
   return steps_inc_out;
}

void mobile_set_steps_inc( struct MOBILE* o, GFX_COORD_PIXEL inc ) {
   o->steps_inc = inc;
}

GFX_COORD_PIXEL mobile_get_steps_inc_default( struct MOBILE* o ) {
   if( NULL == o ) {
      return 0;
   }
   return o->steps_inc_default;
}

void mobile_update_x( struct MOBILE* o, TILEMAP_COORD_TILE x ) {
   lgc_null( o );
   o->prev_x = o->x;
   o->x = x;
cleanup:
   return;
}

void mobile_update_y( struct MOBILE* o, TILEMAP_COORD_TILE y ) {
   lgc_null( o );
   o->prev_y = o->y;
   o->y = y;
cleanup:
   return;
}

void mobile_set_x( struct MOBILE* o, TILEMAP_COORD_TILE x ) {
   lgc_null( o );
   o->x = x;
cleanup:
   return;
}

void mobile_set_y( struct MOBILE* o, TILEMAP_COORD_TILE y ) {
   lgc_null( o );
   o->y = y;
cleanup:
   return;
}

void mobile_set_prev_x( struct MOBILE* o, TILEMAP_COORD_TILE x ) {
   lgc_null( o );
   o->prev_x = x;
cleanup:
   return;
}

void mobile_set_prev_y( struct MOBILE* o, TILEMAP_COORD_TILE y ) {
   lgc_null( o );
   o->prev_y = y;
cleanup:
   return;
}

void mobile_set_steps_remaining( struct MOBILE* o, GFX_COORD_PIXEL sr ) {
   lgc_null( o );
   o->steps_remaining = sr;
cleanup:
   return;
}

/** \brief Trigger a smooth walking animation for the given mobile. This
 *         function should only ever be called client-side, as it relies on
 *         client-side animation routines.
 */
bool mobile_walk(
   struct MOBILE* o,
   TILEMAP_COORD_TILE dest_x,
   TILEMAP_COORD_TILE dest_y,
   GFX_COORD_PIXEL mobile_steps_increment
) {
   TILEMAP_COORD_TILE start_x = 0,
      start_y = 0;
   int diff_x = 0,
      diff_y = 0;
   bool success = false;
   GFX_COORD_PIXEL steps = 0;

   scaffold_assert_client();

   lgc_null( o );

   start_x = mobile_get_x( o );
   start_y = mobile_get_y( o );

   /* Setup walking starting conditions. Update callbacks will handle these
    * later. */
   if( dest_x != start_x ) {
      mobile_update_x( o, mobile_get_x( o ) ); /* Forceful reset. */
   }
   if( dest_y != start_y ) {
      mobile_update_y( o, mobile_get_y( o ) ); /* Forceful reset. */
   }
   mobile_set_x( o, dest_x );
   mobile_set_y( o, dest_y );

   diff_x = (dest_x - start_x);
   diff_y = (dest_y - start_y);

   /* Setup facing direction and animation.*/
   switch( diff_x ) {
   case -1:
      mobile_set_facing( o, MOBILE_FACING_LEFT );
      break;

   case 0:
      switch( diff_y ) {
      case -1:
         mobile_set_facing( o, MOBILE_FACING_UP );
         break;
      case 1:
         mobile_set_facing( o, MOBILE_FACING_DOWN );
         break;
      default:
         /* ERROR */
         break;
      }
      break;

   case 1:
      mobile_set_facing( o, MOBILE_FACING_RIGHT );
      break;
   default:
      /* ERROR */
      break;
   }
   /* We'll calculate the actual animation frames to use in the per-mode
    * update() function, where we have access to the current camera rotation
    * and stuff like that. */
   mobile_call_reset_animation( o );

   lg_debug( __FILE__, "Movement diff: (X) %d, (Y) %d\n", diff_x, diff_y );
   if( 0 != diff_x ) {
      steps = MOBILE_STEPS_MAX * diff_x * -1;
      mobile_steps_increment *= diff_x;
   }
   if( 0 != diff_y ) {
      steps = MOBILE_STEPS_MAX * diff_y * -1;
      mobile_steps_increment *= diff_y;
   }
   mobile_set_steps_remaining( o, steps );
   mobile_set_steps_inc( o, mobile_steps_increment );

   lg_debug(
      __FILE__, "Setting steps: %d (+%d)\n", steps, mobile_steps_increment );

   /* Everything seemed to work out. */
   success = true;

cleanup:
   return success;
}

void mobile_ai_add_global( struct MOBILE* o, const bstring key, const bstring val ) {
   lg_debug( __FILE__, "Added global to %d: %b: %b\n",
      mobile_get_serial( o ), key, val );
   audition_context_add_var( o->ai, key, val );
}

enum MOBILE_FACING mobile_get_facing( const struct MOBILE* o ) {
   if( NULL == o ) {
      return MOBILE_FACING_DOWN;
   }
   return o->facing;
}

bool mobile_get_animation_reset( const struct MOBILE* o ) {
   if( NULL == o ) {
      return false;
   }
   return o->animation_reset;
}

struct MOBILE_ANI_DEF* mobile_get_animation_current(
   const struct MOBILE* o
) {
   if( NULL == o ) {
      return NULL;
   }
   return o->current_animation;
}

struct MOBILE_SPRITE_DEF* mobile_get_animation_frame(
   const struct MOBILE* o, size_t index
) {
   if( NULL == o || NULL == o->sprite_defs ) {
      return NULL;
   }
   return vector_get( o->sprite_defs, index );
}

struct MOBILE_SPRITE_DEF* mobile_get_animation_frame_current(
   const struct MOBILE* o
) {
   struct MOBILE_ANI_DEF* ani = NULL;
   if( NULL == o || NULL == o->sprite_defs ) {
      return NULL;
   }
   ani = o->current_animation;
   return vector_get( ani->frames, o->current_frame );
}

void* mobile_get_mode_data( struct MOBILE* o, bstring mode, struct CHANNEL* l ) {
   struct HASHMAP* channels = NULL;
   void* mode_data = NULL;

   assert( NULL != l && NULL != l->name );
   assert( NULL != mode );

   lgc_null( o );
   assert( NULL != o->mode_data );

   channels = hashmap_get( o->mode_data, mode );
   if( NULL == channels ) {
      lg_debug( __FILE__, "Creating channel list for %b for mobile %d...\n", mode, o->serial );
      channels = hashmap_new();
      hashmap_put( o->mode_data, mode, channels, false );
   }

   mode_data = hashmap_get( channels, l->name );
   if( NULL == mode_data ) {
      plugin_call( PLUGIN_MODE, mode, PLUGIN_MOBILE_INIT, o, l );
   }
   mode_data = hashmap_get( channels, l->name );

cleanup:
   return mode_data;
}

void* mobile_set_mode_data( struct MOBILE* o, bstring mode, struct CHANNEL* l, void* mode_data ) {
   struct HASHMAP* channels = NULL;

   assert( NULL != l && NULL != l->name );
   assert( NULL != mode );

   lgc_null( o );
   assert( NULL != o->mode_data );

   channels = hashmap_get( o->mode_data, mode );
   if( NULL == channels ) {
      lg_debug( __FILE__, "Creating channel list for %b for mobile %d...\n", mode, o->serial );
      channels = hashmap_new();
      hashmap_put( o->mode_data, mode, channels, false );
   }

   assert( NULL == hashmap_get( channels, l->name ) );
   hashmap_put( channels, l->name, mode_data, false );

cleanup:
   return mode_data;
}

GRAPHICS_COLOR mobile_spritesheet_get_pixel(
   struct MOBILE* o, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y
) {
   GRAPHICS_COLOR pixel = GRAPHICS_COLOR_CHARCOAL;
   if( NULL != o && NULL != o->sprites ) {
      pixel = graphics_get_pixel( o->sprites, x, y );
   }
   return pixel;
}

void mobile_spritesheet_set_pixel(
   struct MOBILE* o, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y, GRAPHICS_COLOR pixel
) {
   if( NULL != o && NULL != o->sprites ) {
      graphics_set_pixel( o->sprites, x, y, pixel );
   }
}

void mobile_set_animation_reset( struct MOBILE* o, bool reset ) {
   lgc_null( o );
   o->animation_reset = reset;
cleanup:
   return;
}

GFX_COORD_PIXEL mobile_get_sprite_width( struct MOBILE* o ) {
   if( NULL == o ) {
      return 0;
   }
   return o->sprite_width;
}

GFX_COORD_PIXEL mobile_get_sprite_height( struct MOBILE* o ) {
   if( NULL == o ) {
      return 0;
   }
   return o->sprite_height;
}

