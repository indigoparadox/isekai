
#include "action.h"
#include "tilemap.h"
#include "channel.h"
#include "plugin.h"

static struct VECTOR* action_queue[ACTION_QUEUE_COUNT] = { 0 };

struct ACTION_PACKET {
   struct MOBILE* o;
   struct CHANNEL* l;
   enum ACTION_OP update;
   TILEMAP_COORD_TILE x;
   TILEMAP_COORD_TILE y;
   struct MOBILE* target;
};

struct ACTION_PACKET* action_packet_new(
   struct CHANNEL* l,
   struct MOBILE* o,
   enum ACTION_OP op,
   TILEMAP_COORD_TILE tile_x,
   TILEMAP_COORD_TILE tile_y,
   struct MOBILE* target
) {
   struct ACTION_PACKET* update = NULL;

   /* These are the minimum requirements for a valid action packet. */
   lgc_null( l );
   lgc_null( o );

   /* This'll work, so let's make the packet. */
   update = mem_alloc( 1, struct ACTION_PACKET );

   update->l = l;
   update->o = o;
   update->update = op;
   update->x = tile_x;
   update->y = tile_y;
   update->target = target;

   refcount_inc( l, "channel" );
   mobile_add_ref( o );
   if( NULL != target ) {
      mobile_add_ref( o );
   }

cleanup:
   return update;
}

void action_enqueue( struct ACTION_PACKET* update, enum ACTION_QUEUE q ) {

   if( NULL == action_queue[q] ) {
      action_queue[q] = vector_new();
   }

   lg_debug( __FILE__, "Enqueing update in queue %d.\n", q );

   vector_enqueue( action_queue[q], update );
}

size_t action_queue_proc( bstring mode_id, enum ACTION_QUEUE q ) {
   size_t handled = 0;
   size_t failed = 0;
   struct ACTION_PACKET* update = NULL;
   PLUGIN_RESULT res = 0;
   enum PLUGIN_CALL act_side;

   if( ACTION_QUEUE_CLIENT == q ) {
      act_side = PLUGIN_MOBILE_ACTION_CLIENT;
   } else {
      act_side = PLUGIN_MOBILE_ACTION_SERVER;
   }

   while( 0 < vector_count( action_queue[q] ) ) {
      update = vector_dequeue( action_queue[q] );
      res = plugin_call( PLUGIN_MODE, mode_id, act_side, update );
      action_packet_free( &update );
      if( PLUGIN_SUCCESS == res ) {
         handled++;
      } else {
         failed++;
      }
   }

   if( 0 < handled || 0 < failed ) {
      lg_debug(
         __FILE__,
         "Actions processed (Q%d): %d successful, %d failed.\n",
         q, handled, failed
      );
   }

   return handled;
}

struct CHANNEL* action_packet_get_channel( struct ACTION_PACKET* update ) {
   if( NULL == update ) {
      return NULL;
   }
   return update->l;
}

struct MOBILE* action_packet_get_mobile( struct ACTION_PACKET* update ) {
   if( NULL == update ) {
      return NULL;
   }
   return update->o;
}

struct MOBILE* action_packet_get_target( struct ACTION_PACKET* update ) {
   if( NULL == update ) {
      return NULL;
   }
   return update->target;
}

TILEMAP_COORD_TILE action_packet_get_tile_x( struct ACTION_PACKET* update ) {
   if( NULL == update ) {
      return 0;
   }
   return update->x;
}

TILEMAP_COORD_TILE action_packet_get_tile_y( struct ACTION_PACKET* update ) {
   if( NULL == update ) {
      return 0;
   }
   return update->y;
}

enum ACTION_OP action_packet_get_op( struct ACTION_PACKET* update ) {
   if( NULL == update ) {
      return ACTION_OP_NONE;
   }
   return update->update;
}

void action_packet_free( struct ACTION_PACKET** update ) {
   lgc_null( *update );

   mobile_free( (*update)->o );
   channel_free( (*update)->l );
   if( NULL != (*update)->target ) {
      mobile_free( (*update)->target );
   }

   mem_free( *update );
   *update = NULL;

cleanup:
   return;
}

void action_packet_set_op(
   struct ACTION_PACKET* update, enum ACTION_OP op
) {
   lgc_null( update );
   update->update = op;
cleanup:
   return;
}

void action_packet_set_tile_x(
   struct ACTION_PACKET* update, TILEMAP_COORD_TILE x
) {
   lgc_null( update );
   update->x = x;
cleanup:
   return;
}

void action_packet_set_tile_y(
   struct ACTION_PACKET* update, TILEMAP_COORD_TILE y
) {
   lgc_null( update );
   update->y = y;
cleanup:
   return;
}
