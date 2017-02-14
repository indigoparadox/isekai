
#ifndef PARSER_H
#define PARSER_H

#include "bstrlib/bstrlib.h"

typedef struct {
    struct tagbstring command;
    void (*callback)( void* local, void* remote, struct bstrList* args );
} parser_entry;

/*
void parser_server_reply_welcome( void* local, void* remote );
void parser_server_reply_nick( void* local, void* remote, bstring oldnick );
void parser_server_reply_motd( void* local, void* remote );
*/

/* "caller" can be a SERVER or a CLIENT. as they should start with the same *
 * layout.                                                                  */
void parser_dispatch( void* local, void* remote, const_bstring line );

#endif /* PARSER_H */
