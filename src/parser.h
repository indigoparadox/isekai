
#ifndef PARSER_H
#define PARSER_H

#include "bstrlib/bstrlib.h"

typedef struct {
    char* command;
    int command_length;
    void (*callback)( void* caller, void* arg );
} parser_entry;

/* "caller" can be a SERVER or a CLIENT. as they should start with the same *
 * layout.                                                                  */
void parser_dispatch( void* caller, bstring line );

#endif /* PARSER_H */
