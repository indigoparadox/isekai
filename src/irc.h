#ifndef PARSER_H
#define PARSER_H

#include "bstrlib/bstrlib.h"
#include "client.h"
#include "server.h"
#include "ref.h"

//typedef void (*irc_cmd_cb)( CLIENT* c, SERVER* s, struct bstrList* args );

typedef enum _IRC_REPLY {
   ERR_NONICKNAMEGIVEN = 431,
   ERR_NICKNAMEINUSE = 433,
} IRC_REPLY;

typedef struct _IRC_COMMAND {
   REF refcount;
   struct tagbstring command;
   //irc_cmd_cb* callback;
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

IRC_COMMAND_TABLE_START( server );
IRC_COMMAND_TABLE_START( client );

#define IRC_LINE_CMD_SEARCH_RANGE 2

#define PARSER_FILE_XMIT_BUFFER 19
#define PARSER_HS_WINDOW_SIZE 14
#define PARSER_HS_LOOKAHEAD_SIZE 8

void irc_command_free( IRC_COMMAND* cmd );
IRC_COMMAND* irc_dispatch(
   const IRC_COMMAND* table, SERVER* s, CLIENT* c, const_bstring line
);

#endif /* PARSER_H */
