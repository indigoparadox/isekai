
#include "parser.h"

#include "server.h"

#include <ctype.h>

void parser_server_stop( void* local, void* remote, struct bstrList* args ) {
    SERVER* s = (SERVER*)local;

    server_stop( s );
}

void parser_server_user( void* local, void* remote, struct bstrList* args ) {
    CLIENT* c = (CLIENT*)remote;
    int i,
        consumed = 0;
    //BOOL has_nick

    /* Start at 1, skipping the command, itself. */
    for( i = 1 ; args->qty > i ; i++ ) {
        if( 0 == consumed ) {
            /* First arg: User */
            c->username = bstrcpy( args->entry[i] );
        } else if( 1 == consumed && scaffold_is_numeric( args->entry[i] ) ) {
            /* Second arg: Mode */
            c->mode = atoi( bdata( args->entry[i] ) );
        } else if( 1 == consumed || 2 == consumed ) {
            /* Second or Third arg: * */
            if( 1 == consumed ) {
                consumed++;
            }
        } else if( 3 == consumed && ':' != args->entry[i]->data[0] ) {
            /* Third or Fourth arg: Remote Host */
            c->remote = bstrcpy( args->entry[i] );
        } else if( 3 == consumed || 4 == consumed ) {
            /* Fourth or Fifth arg: Real Name */
            c->realname = bstrcpy( args->entry[i] );
            if( 4 == consumed ) {
                consumed++; /* Extra bump for missing host. */
            }
        } else if( 4 < consumed ) {
            /* More real name. */
            bconchar( c->realname, ' ' );
            bconcat( c->realname, args->entry[i] );
        }
        consumed++;
    }

    scaffold_print_debug( "User: %s, Real: %s, Remote: %s\n", bdata( c->username ), bdata( c->realname ), bdata( c->remote ) );
}

void parser_server_nick( void* local, void* remote, struct bstrList* args ) {
    CLIENT* c = (CLIENT*)remote;

    c->nick = bstrcpy( args->entry[1] );
}

const parser_entry parser_table_server[] = {
    {"stop", 4, parser_server_stop},
    {"USER", 4, parser_server_user},
    {"NICK", 4, parser_server_nick},
    {NULL, 0, NULL}
};

void parser_dispatch( void* local, void* remote, const_bstring line ) {
    SERVER* s_local = (SERVER*)local;
    bstring test = bfromcstr( "" );
    const parser_entry* parser_table;
    struct bstrList* args;
    const parser_entry* command;

    if( SERVER_SENTINAL == s_local->self.sentinal ) {
        parser_table = parser_table_server;
    }

    args = bsplit( line, ' ' );
    scaffold_check_null( args );

    for(
        command = &(parser_table[0]);
        NULL != command->command;
        command++
    ) {
        bassigncstr( test, command->command );
        scaffold_print_debug( "%s vs %s\n", bdata( line ), bdata( test ) );
        if( 0 == bstrncmp( line, test, command->command_length ) ) {
            command->callback( local, remote, args );
        }
    }

cleanup:

    bstrListDestroy( args );
    bdestroy( test );
}
