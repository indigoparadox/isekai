
#include "parser.h"

#include "server.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static void parser_client_printf( CLIENT* c, const char* message, ... ) {
    va_list varg;
    const char* chariter;
    bstring buffer = NULL;
    bstring insert = NULL;
    int bstr_res;
    int i;

    scaffold_error = 0;
    buffer = bfromcstralloc( strlen( message ), "" );
    scaffold_check_null( buffer );

    va_start( varg, message );

    for( chariter = message ; '\0' != *chariter ; chariter++ ) {
        if( '%' != *chariter ) {
            bconchar( buffer, *chariter );
            continue;
		}

        switch( *++chariter ) {
            case 'c':
                i = va_arg( varg, int );
                bstr_res = bconchar( buffer, i );
                scaffold_check_nonzero( bstr_res );
                break;

            case 'd':
                i = va_arg( varg, int );
                insert = bformat( "%d", i );
                scaffold_check_null( insert );
                bstr_res = bconcat( buffer, insert );
                bdestroy( insert );
                insert = NULL;
                scaffold_check_nonzero( bstr_res );
                break;

            case 's':
                insert = bfromcstr( va_arg( varg, char* ) );
                scaffold_check_null( insert );
                bstr_res = bconcat( buffer, insert );
                bdestroy( insert );
                insert = NULL;
                scaffold_check_nonzero( bstr_res );
                break;

            case 'b':
                insert = va_arg( varg, bstring );
                bstr_res = bconcat( buffer, insert );
                insert = NULL;
                scaffold_check_nonzero( bstr_res );
                break;

            case '%':
                bstr_res = bconchar( buffer, '%' );
                scaffold_check_nonzero( bstr_res );
                break;
            }
    }

cleanup:

    bdestroy( insert );

    if( 0 == scaffold_error ) {
        client_send( c, buffer );
    }

    bdestroy( buffer );

    va_end( varg );
}

/* This file contains our (possibly limited, slightly incompatible) version *
 * of the IRC protocol, as it interacts with our server and client objects. */

static void parser_server_reply_welcome( void* local, void* remote ) {
    CLIENT* c = (CLIENT*)remote;
    SERVER* s = (SERVER*)local;

    parser_client_printf(
        c, ":%b 001 %b :Welcome to the Internet Relay Network %b!%b@%b",
        s->self.remote, c->nick, c->nick, c->username, c->remote
    );

    parser_client_printf(
        c, ":%b 002 %b :Your host is %b, running version %b",
        s->self.remote, c->nick, s->servername, s->version
    );

    parser_client_printf(
        c, ":%b 003 %b :This server was created 01/01/1970",
        s->self.remote, c->nick
    );

    parser_client_printf(
        c, ":%b 004 %b :%b %b-%b abBcCFiIoqrRswx abehiIklmMnoOPqQrRstvVz",
        s->self.remote, c->nick, s->self.remote, s->servername, s->version
    );

    parser_client_printf(
        c, ":%b 251 %b :There are %d users and 0 services on 1 servers",
        s->self.remote, c->nick, vector_count( &(s->clients) )
    );

    c->flags |= CLIENT_FLAGS_HAVE_WELCOME;
}

static void parser_server_reply_nick( void* local, void* remote, bstring oldnick ) {
    CLIENT* c = (CLIENT*)remote;
    SERVER* s = (SERVER*)local;

    if( !(c->flags & CLIENT_FLAGS_HAVE_WELCOME) ) {
        goto cleanup;
    }

    if( NULL == oldnick ) {
        oldnick = c->username;
    }

    parser_client_printf(
        c, ":%b %b :%b!%b@%b NICK %b",
        s->self.remote, c->nick, oldnick, c->username, c->remote, c->nick
    );

cleanup:
    return;
}

void parser_server_reply_motd( void* local, void* remote ) {
    CLIENT* c = (CLIENT*)remote;
    SERVER* s = (SERVER*)local;

    if( 1 > blength( c->nick ) ) {
        goto cleanup;
    }

    if( c->flags & CLIENT_FLAGS_HAVE_MOTD ) {
        goto cleanup;
    }

    parser_client_printf(
        c, ":%b 433 %b :MOTD File is missing",
        s->self.remote, c->nick
    );

    c->flags |= CLIENT_FLAGS_HAVE_MOTD;

cleanup:
    return;
}

static void parser_server_user( void* local, void* remote, struct bstrList* args ) {
    CLIENT* c = (CLIENT*)remote;
    SERVER* s = (SERVER*)local;
    int i,
        consumed = 0;

    /* TODO: Error on already registered. */

    /* Start at 1, skipping the command, itself. */
    for( i = 1 ; args->qty > i ; i++ ) {
        if( 0 == consumed ) {
            /* First arg: User */
            scaffold_copy_string( c->username, args->entry[i] );
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
            scaffold_copy_string( c->remote, args->entry[i] );
        } else if( 3 == consumed || 4 == consumed ) {
            /* Fourth or Fifth arg: Real Name */
            scaffold_copy_string( c->realname, args->entry[i] );
            if( 3 == consumed ) {
                consumed++; /* Extra bump for missing host. */
            }
        } else if( 4 < consumed ) {
            /* More real name. */
            bconchar( c->realname, ' ' );
            bconcat( c->realname, args->entry[i] );
        }
        consumed++;
    }

    /* Try to set the nick. */
    if(
        0 == bstrcmp( &scaffold_empty_string, c->nick ) &&
        0 != server_set_client_nick( s, c, c->username )
    ) {
        parser_client_printf(
            c, ":%b 433 %b :Nickname is already in use",
            s->self.remote, c->username
        );
    }

    //scaffold_print_debug( "User: %s, Real: %s, Remote: %s\n", bdata( c->username ), bdata( c->realname ), bdata( c->remote ) );

    c->flags |= CLIENT_FLAGS_HAVE_USER;

    if( ( c->flags & CLIENT_FLAGS_HAVE_NICK ) ) {
        parser_server_reply_welcome( local, remote );
    }
    //parser_server_reply_nick( local, remote, NULL );

    return;
}

static void parser_server_nick( void* local, void* remote, struct bstrList* args ) {
    CLIENT* c = (CLIENT*)remote;
    SERVER* s = (SERVER*)local;
    bstring oldnick = NULL;
    bstring newnick = NULL;
    int nick_return;

    if( 2 >= args->qty ) {
        newnick = args->entry[1];
    }

    nick_return = server_set_client_nick( s, c, newnick );
    if( ERR_NONICKNAMEGIVEN == nick_return ) {
        parser_client_printf(
            c, ":%b 431 %b :No nickname given",
            s->self.remote, c->nick
        );
        goto cleanup;
    }

    if( ERR_NICKNAMEINUSE == nick_return ) {
        parser_client_printf(
            c, ":%b 433 %b :Nickname is already in use",
            s->self.remote, c->nick
        );
        goto cleanup;
    }

    if( 0 < blength( c->nick ) ) {
        oldnick = bstrcpy( c->nick );
    }

    scaffold_copy_string( c->nick, newnick );

    c->flags |= CLIENT_FLAGS_HAVE_NICK;

    /* Don't reply yet if there's been no USER statement yet. */
    if( !(c->flags & CLIENT_FLAGS_HAVE_USER) ) {
        goto cleanup;
    }

    if( !(c->flags & CLIENT_FLAGS_HAVE_WELCOME) ) {
        parser_server_reply_welcome( local, remote );
    }

    parser_server_reply_nick( local, remote, oldnick );

cleanup:

    bdestroy( oldnick );

    return;
}

static void parser_server_quit( void* local, void* remote, struct bstrList* args ) {
    SERVER* s = (SERVER*)local;
    CLIENT* c = (CLIENT*)remote;
    bstring message;
    bstring space;

    /* TODO: Send everyone parting message. */
    space = bfromcstr( " " );
    message = bjoin( args, space );

    parser_client_printf(
        c, "ERROR :Closing Link: %b (Client Quit)",
        c->nick
    );

    server_drop_client( s, c->link.socket );

    bdestroy( message );
    bdestroy( space );
}

static void parser_server_ison( void* local, void* remote, struct bstrList* args ) {
    SERVER* s = (SERVER*)local;
    CLIENT* c = (CLIENT*)remote;
    int i;
    bstring reply;

    reply = bfromcstralloc( 128, ":" );
    bconcat( reply, s->self.remote );
    bconcat( reply, scaffold_static_string( " 303 " ) );
    bconcat( reply, c->nick );
    bconcat( reply, scaffold_static_string( " :" ) );

    connection_lock( &(s->self.link) );

    for( i = 1 ; args->qty > i ; i++ ) {
        if( NULL != server_get_client_by_nick(
            s, args->entry[i], FALSE
        ) ) {
            bconcat( reply, args->entry[i] );
            bconchar( reply, ' ' );
        }
    }

    connection_unlock( &(s->self.link) );

    client_send( c, reply );
    bdestroy( reply );

    return;
}

static void parser_server_join( void* local, void* remote, struct bstrList* args ) {
}

static void parser_server_part( void* local, void* remote, struct bstrList* args ) {
}

static void parser_server_privmsg( void* local, void* remote, struct bstrList* args ) {
    SERVER* s = (SERVER*)local;
    CLIENT* c = (CLIENT*)remote;
    CLIENT* c_dest = NULL;
    bstring msg = NULL;

    //bdestroy( scaffold_pop_string( args ) );
    msg = bjoin( args, &scaffold_space_string );

    c_dest = server_get_client_by_nick( s, args->entry[1], TRUE );
    if( NULL != c_dest ) {
        parser_client_printf(
            c_dest, ":%b!%b@%b %b", c->nick, c->username, c->remote, msg
        );
    }

    bdestroy( msg );

    return;
}

const parser_entry parser_table_server[] = {
    {bsStatic( "USER" ), parser_server_user},
    {bsStatic( "NICK" ), parser_server_nick},
    {bsStatic( "QUIT" ), parser_server_quit},
    {bsStatic( "ISON" ), parser_server_ison},
    {bsStatic( "JOIN" ), parser_server_join},
    {bsStatic( "PART" ), parser_server_part},
    {bsStatic( "PRIVMSG" ), parser_server_privmsg},
    {bsStatic( "" ), NULL}
};

void parser_dispatch( void* local, void* remote, const_bstring line ) {
    SERVER* s_local = (SERVER*)local;
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
        NULL != command->callback;
        command++
    ) {
        /* scaffold_print_debug( "%s vs %s\n", bdata( line ), bdata( test ) ); */
        if( 0 == bstrncmp(
            line, &(command->command), blength( &(command->command) )
        ) ) {
            command->callback( local, remote, args );
            goto cleanup;
        }
    }

cleanup:

    bstrListDestroy( args );
}
