#ifndef PARSER_H
#define PARSER_H

#include "bstrlib/bstrlib.h"
#include "client.h"
#include "server.h"
#include "ref.h"
#include "chunker.h"

typedef struct _IRC_COMMAND {
   struct REF refcount;
   const struct tagbstring command;
   void (*callback)( struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line );
   struct SERVER* server;
   struct CLIENT* client;
   const struct bstrList* args;
   bstring line;
} IRC_COMMAND;

#define IRC_COMMAND_TABLE_START( name ) \
   const IRC_COMMAND proto_table_ ## name []
#define IRC_COMMAND_ROW( command, callback ) \
   {REF_DISABLED, bsStatic( command ), callback, NULL, NULL, NULL, NULL}
#define IRC_COMMAND_TABLE_END() \
   {REF_DISABLED, bsStatic( "" ), NULL, NULL, NULL, NULL, NULL}

#ifndef IRC_C
extern IRC_COMMAND_TABLE_START( server );
extern IRC_COMMAND_TABLE_START( client );
#endif /* IRC_C */

#ifdef DEBUG
#define irc_print_args() \
   for( i = 0 ; args->qty > i ; i++ ) { \
      scaffold_print_debug( \
         &module, "GDB %d: %s\n", i, bdata( args->entry[i] ) ); \
   }
#endif

#define IRC_LINE_CMD_SEARCH_RANGE 3

void proto_register( struct CLIENT* c );
void proto_send_chunk(
   struct CLIENT* c, struct CHUNKER* h, SCAFFOLD_SIZE start_pos,
   const bstring filename, const bstring data
);
void proto_abort_chunker( struct CLIENT* c, struct CHUNKER* h );
void proto_request_file( struct CLIENT* c, const bstring filename, DATAFILE_TYPE type );
void proto_send_mob( struct CLIENT* c, struct MOBILE* o );
void proto_send_container( struct CLIENT* c, struct ITEM* e );
void proto_send_tile_cache(
   struct CLIENT* c, struct TILEMAP_ITEM_CACHE* cache
);
void proto_send_tile_cache_channel(
   struct CHANNEL* l, struct TILEMAP_ITEM_CACHE* cache
);
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

void irc_command_free( IRC_COMMAND* cmd );
IRC_COMMAND* irc_dispatch(
   const IRC_COMMAND* table, struct SERVER* s, struct CLIENT* c, const_bstring line
);

#ifdef PROTO_C
SCAFFOLD_MODULE( "proto.c" );
#endif /* PROTO_C */

#endif /* PARSER_H */
