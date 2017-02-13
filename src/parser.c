
#include "parser.h"

#include "server.h"

void parser_server_stop( void* local, void* remote, struct bstrList* args ) {
    SERVER* s = (SERVER*)local;

    server_stop( s );
}

void parser_server_user( void* local, void* remote, struct bstrList* args ) {
    SERVER* s = (SERVER*)local;
    CLIENT* c = (CLIENT*)remote;
}

const parser_entry parser_table_server[] = {
    {"stop", 4, parser_server_stop},
    {"USER", 4, parser_server_user},
};

#define PARSER_TABLE_SERVER_LEN 1

void parser_dispatch( void* local, void* remote, bstring line ) {
    int i;
    SERVER* s_local = (SERVER*)local;
    bstring test = bfromcstr( "" );
    const parser_entry* parser_table;
    int parser_table_len;
    struct bstrList* args;

    if( SERVER_SENTINAL == s_local->self.sentinal ) {
        parser_table = parser_table_server;
        parser_table_len = PARSER_TABLE_SERVER_LEN;
    }

    args = bsplit( line, ' ' );

    for( i = 0 ; parser_table_len > i ; i++ ) {
        bassigncstr( test, parser_table[i].command );
        if( 0 == bstrncmp( test, line, blength( test ) ) ) {
            parser_table[i].callback( local, remote, args );
        }
    }

cleanup:

    bstrListDestroy( args );
    bdestroy( test );
}
