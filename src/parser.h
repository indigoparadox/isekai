
#ifndef PARSER_H
#define PARSER_H

#include "bstrlib/bstrlib.h"

typedef struct {
    const char* command;
    int command_length;
    void (*callback)( void* local, void* remote, struct bstrList* args );
} parser_entry;

/* "caller" can be a SERVER or a CLIENT. as they should start with the same *
 * layout.                                                                  */
void parser_dispatch( void* local, void* remote, const_bstring line );

#endif /* PARSER_H */
