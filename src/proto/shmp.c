
#define PROTO_C
#include "../proto.h"

#include "../channel.h"

extern struct SERVER* main_server;

void proto_register( struct CLIENT* c ) {
   server_add_client( main_server, c );
   c->flags |= CLIENT_FLAGS_HAVE_USER;
   c->flags |= CLIENT_FLAGS_HAVE_NICK;
   c->flags |= CLIENT_FLAGS_HAVE_WELCOME;
}

void proto_client_join( struct CLIENT* c, const bstring name ) {
   struct CHANNEL* l = NULL;

   l = server_add_channel( main_server, name, c );
   lgc_null( l );

   assert( NULL != c );
   assert( NULL != main_server );
   assert( NULL != client_get_nick( c ) );
   assert( NULL != c->username );
   assert( NULL != c->remote );

   /* Get the channel, or create it if it does not exist. */
   l = client_get_channel_by_name( c, name );
   if( NULL == l ) {
      /* Create a new client-side channel mirror. */
      client_add_channel( c, l );
   }

   /* TODO: Call client's join stuff. */

   scaffold_assert( hashmap_count( &(c->channels) ) > 0 );
   scaffold_assert( hashmap_count( &(main_server->self.channels) ) > 0 );

   c->flags |= CLIENT_FLAGS_SENT_CHANNEL_JOIN;

   proto_client_request_mobs( c, l );

cleanup:
   return;
}

void proto_client_leave( struct CLIENT* c, const bstring lname ) {
}

void proto_send_chunk(
   struct CLIENT* c, struct CHUNKER* h, SCAFFOLD_SIZE start_pos,
   const bstring filename, const bstring data
) {
}

void proto_abort_chunker( struct CLIENT* c, struct CHUNKER* h ) {
}

void proto_request_file( struct CLIENT* c, const bstring filename, DATAFILE_TYPE type ) {
}

void proto_send_mob( struct CLIENT* c, struct MOBILE* o ) {
}

void proto_send_container( struct CLIENT* c, struct ITEM* e ) {
}

void proto_send_tile_cache(
   struct CLIENT* c, struct TILEMAP_ITEM_CACHE* cache
) {
}

void proto_send_tile_cache_channel(
   struct CHANNEL* l, struct TILEMAP_ITEM_CACHE* cache
) {
}

void proto_server_send_update( struct CLIENT* c, struct ACTION_PACKET* update ) {
}

void proto_client_send_update( struct CLIENT* c, struct ACTION_PACKET* update ) {
}

void proto_send_msg_channel( struct CLIENT* c, struct CHANNEL* ld, bstring msg ) {
}

void proto_send_msg_client( struct CLIENT* c, struct CLIENT* cd, bstring msg ) {
}

void proto_send_msg( struct CLIENT* c, bstring dest, bstring msg ) {
}

void proto_client_stop( struct CLIENT* c ) {
}

/*
static void* proto_client_mobs_cb( struct CONTAINER_IDX* ctx, void* iter, void* arg ) {
   struct CHANNEL* l = (struct CHANNEL*)arg;
   struct MOBILE* o = (struct MOBILE*)iter;
   if( NULL != o && MOBILE_TYPE_PLAYER == o->type ) {
      client_set_puppet( main_client, o );
   }
}
*/

void proto_client_request_mobs( struct CLIENT* c, struct CHANNEL* l ) {
   //vector_iterate_nolock( &(l->mobiles), proto_client_mobs_cb, l );
}

void proto_server_send_msg_channel(
   struct SERVER* s, struct CHANNEL* l, const bstring nick, const bstring msg
) {
}

#ifdef DEBUG_VM
void proto_client_debug_vm(
   struct CLIENT* c, struct CHANNEL* l, const bstring code
) {
};
#endif /* DEBUG_VM */

bool proto_dispatch( struct CLIENT* c, struct SERVER* s ) {
}

void proto_setup() {
}

void proto_shutdown() {
}

void proto_empty_buffer( struct CLIENT* c ) {
}
