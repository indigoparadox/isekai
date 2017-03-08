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
   void (*callback)( struct CLIENT* c, SERVER* s, const struct bstrList* args );
   SERVER* server;
   struct CLIENT* client;
   const struct bstrList* args;
} IRC_COMMAND;

#define IRC_COMMAND_TABLE_START( name ) \
   const IRC_COMMAND proto_table_ ## name []
#define IRC_COMMAND_ROW( command, callback ) \
   {REF_DISABLED, bsStatic( command ), callback, NULL, NULL, NULL}
#define IRC_COMMAND_TABLE_END() \
   {REF_DISABLED, bsStatic( "" ), NULL, NULL, NULL, NULL}

#ifndef IRC_C
extern IRC_COMMAND_TABLE_START( server );
extern IRC_COMMAND_TABLE_START( client );
#endif /* IRC_C */

#ifdef DEBUG
#define irc_print_args() \
   for( i = 0 ; args->qty > i ; i++ ) { \
      scaffold_print_debug( "GDB %d: %s\n", i, bdata( args->entry[i] ) ); \
   }
#endif

#define IRC_LINE_CMD_SEARCH_RANGE 2

void irc_command_free( IRC_COMMAND* cmd );
IRC_COMMAND* irc_dispatch(
   const IRC_COMMAND* table, SERVER* s, struct CLIENT* c, const_bstring line
);

#endif /* PARSER_H */
