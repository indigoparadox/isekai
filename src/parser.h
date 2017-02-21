#ifndef PARSER_H
#define PARSER_H

#include "bstrlib/bstrlib.h"

typedef struct {
   struct tagbstring command;
   void (*callback)( void* local, void* remote, struct bstrList* args );
} parser_entry;

#define ERR_NONICKNAMEGIVEN 431
#define ERR_NICKNAMEINUSE 433

/* "caller" can be a SERVER or a CLIENT. as they should start with the same *
 * layout.                                                                  */
void parser_dispatch( void* local, void* remote, const_bstring line );

#endif /* PARSER_H */
