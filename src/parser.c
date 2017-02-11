
#include "parser.h"

#include "server.h"

void parser_server_stop( void* caller, void* arg ) {
    SERVER* s = (SERVER*)caller;

    server_stop( s );
}

const parser_entry parser_table_server[] = {
    {"stop", 4, parser_server_stop},
};

#define PARSER_TABLE_SERVER_LEN 1

void parser_dispatch( void* caller, bstring line ) {
    int i;
    SERVER* s_caller = (SERVER*)caller;
    bstring test = bfromcstr( "" );
    const parser_entry* parser_table;
    int parser_table_len;

    if( SERVER_SENTINAL == s_caller->sentinal ) {
        parser_table = parser_table_server;
        parser_table_len = PARSER_TABLE_SERVER_LEN;
    }

    for( i = 0 ; parser_table_len > i ; i++ ) {
        bassigncstr( test, parser_table[i].command );
        if( 0 == bstrncmp( test, line, blength( test ) ) ) {
            parser_table[i].callback( caller, NULL );
        }
    }

cleanup:

    bdestroy( test );
}
