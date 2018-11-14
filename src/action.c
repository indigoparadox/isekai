
#include "action.h"
#include "tilemap.h"
#include "channel.h"
#include "plugin.h"

static struct VECTOR* action_queue_v = NULL;

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

/** \brief Apply an update received from a remote client to a local mobile.
 * \param[in] update    Packet containing update information.
 * \param[in] instant   A BOOL indicating whether to force the update instantly
 *                      or allow any relevant animations to take place.
 * \return What the update becomes to send to the clients. MOBILE_UPDATE_NONE
 *         if the update failed to occur.
 */
void action_enqueue( struct ACTION_PACKET* update ) {

   if( NULL == action_queue_v ) {
      action_queue_v = vector_new();
   }

   vector_enqueue( action_queue_v, update );
}

size_t action_queue_proc( bstring mode_id ) {
   size_t handled = 0;
   size_t failed = 0;
   struct ACTION_PACKET* update = NULL;
   PLUGIN_RESULT res = 0;

   while( 0 < vector_count( action_queue_v ) ) {
      update = vector_dequeue( action_queue_v );
      res = plugin_call( PLUGIN_MODE, mode_id, PLUGIN_MOBILE_ACTION, update );
      action_packet_free( update );
      if( PLUGIN_SUCCESS ) {
         handled++;
      } else {
         failed++;
      }
   }

   lg_debug( "Actions processed: %d successful, %d failed.\n", handled, failed );

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
