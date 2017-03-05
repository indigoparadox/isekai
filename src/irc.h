#ifndef PARSER_H
#define PARSER_H

#include "bstrlib/bstrlib.h"
#include "client.h"
#include "server.h"
#include "ref.h"
#include "chunker.h"

typedef enum _IRC_REPLY {
   ERR_NONICKNAMEGIVEN = 431,
   ERR_NICKNAMEINUSE = 433,
} IRC_REPLY;

typedef struct _IRC_COMMAND {
   REF refcount;
   struct tagbstring command;
   void (*callback)( CLIENT* c, SERVER* s, struct bstrList* args );
   SERVER* server;
   CLIENT* client;
   struct bstrList* args;
} IRC_COMMAND;

#define IRC_COMMAND_TABLE_START( name ) \
   const IRC_COMMAND irc_table_ ## name []
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

void irc_request_file(
   CLIENT* c, CHANNEL* l, CHUNKER_DATA_TYPE type, bstring filename
);
void irc_command_free( IRC_COMMAND* cmd );
IRC_COMMAND* irc_dispatch(
   const IRC_COMMAND* table, SERVER* s, CLIENT* c, const_bstring line
);

#endif /* PARSER_H */
