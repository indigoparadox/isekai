#ifndef PARSER_H
#define PARSER_H

#include "libvcol.h"
#include "client.h"
#include "server.h"
#include "ref.h"
#include "chunker.h"

void proto_register( struct CLIENT* c );
void proto_client_join( struct CLIENT* c, const bstring name );
void proto_client_leave( struct CLIENT* c, const bstring lname );
void proto_send_chunk(
   struct CLIENT* c, struct CHUNKER* h, SCAFFOLD_SIZE start_pos,
   const bstring filename, const bstring data
);
void proto_abort_chunker( struct CLIENT* c, struct CHUNKER* h );
void proto_request_file( struct CLIENT* c, const bstring filename, DATAFILE_TYPE type );
void proto_send_mob( struct CLIENT* c, struct MOBILE* o );
void proto_send_container( struct CLIENT* c, struct ITEM* e );
#ifdef USE_ITEMS
void proto_send_tile_cache(
   struct CLIENT* c, struct TILEMAP_ITEM_CACHE* cache
);
void proto_send_tile_cache_channel(
   struct CHANNEL* l, struct TILEMAP_ITEM_CACHE* cache
);
#endif // USE_ITEMS
void proto_server_send_update( struct CLIENT* c, struct MOBILE_UPDATE_PACKET* update );
void proto_client_send_update( struct CLIENT* c, struct MOBILE_UPDATE_PACKET* update );
void proto_send_msg_channel( struct CLIENT* c, struct CHANNEL* ld, bstring msg );
void proto_send_msg_client( struct CLIENT* c, struct CLIENT* cd, bstring msg );
void proto_send_msg( struct CLIENT* c, bstring dest, bstring msg );
void proto_client_stop( struct CLIENT* c );
void proto_client_request_mobs( struct CLIENT* c, struct CHANNEL* l );
void proto_server_send_msg_channel(
   struct SERVER* s, struct CHANNEL* l, const bstring nick, const bstring msg
);
#ifdef DEBUG_VM
void proto_client_debug_vm(
   struct CLIENT* c, struct CHANNEL* l, const bstring code
);
#endif /* DEBUG_VM */
BOOL proto_dispatch( struct CLIENT* c, struct SERVER* s );
void proto_setup();
void proto_shutdown();
void proto_empty_buffer( struct CLIENT* c );

/*
void irc_command_free( IRC_COMMAND* cmd );
IRC_COMMAND* irc_dispatch(
   const IRC_COMMAND* table, struct SERVER* s, struct CLIENT* c, const_bstring line
);
*/

#endif /* PARSER_H */
