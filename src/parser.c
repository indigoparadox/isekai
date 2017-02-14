
#include "parser.h"

#include "server.h"

#include <ctype.h>
#include <stdlib.h>

/* This file contains our (possibly limited, slightly incompatible) version *
 * of the IRC protocol, as it interacts with our server and client objects. */

static void parser_server_reply_welcome( void* local, void* remote ) {
    CLIENT* c = (CLIENT*)remote;
    SERVER* s = (SERVER*)local;
    bstring buffer;

    buffer = bfromcstr( "" );

    bassignformat(
        buffer,
        ":%s 001 %s :Welcome to the Internet Relay Network %s!%s@%s",
        bdata( s->self.remote ),
        bdata( c->nick ),
        bdata( c->nick ),
        bdata( c->username ),
        bdata( c->remote )
    );
    client_send( c, buffer );

    bassignformat(
        buffer,
        ":%s 002 %s :Your host is %s, running version %s",
        bdata( s->self.remote ),
        bdata( c->nick ),
        bdata( s->servername ),
        bdata( s->version )
    );
    client_send( c, buffer );

    bassignformat(
        buffer,
        ":%s 003 %s :This server was created 01/01/1970",
        bdata( s->self.remote ),
        bdata( c->nick )
    );
    client_send( c, buffer );

    bassignformat(
        buffer,
        ":%s 004 %s :%s %s-%s abBcCFiIoqrRswx abehiIklmMnoOPqQrRstvVz",
        bdata( s->self.remote ),
        bdata( c->nick ),
        bdata( s->self.remote ),
        bdata( s->servername ),
        bdata( s->version )
    );
    client_send( c, buffer );

    bassignformat(
        buffer,
        ":%s 251 %s :There are %d users and 0 services on 1 servers",
        bdata( s->self.remote ),
        bdata( c->nick ),
        vector_count( &(s->clients) )
    );
    client_send( c, buffer );

    c->flags |= CLIENT_FLAGS_HAVE_WELCOME;

    bdestroy( buffer );
}

static void parser_server_reply_nick( void* local, void* remote, bstring oldnick ) {
    CLIENT* c = (CLIENT*)remote;
    SERVER* s = (SERVER*)local;
    bstring buffer;

    if(
        !(c->flags & CLIENT_FLAGS_HAVE_WELCOME)
    ) {
        goto cleanup;
    }

    if( NULL == oldnick ) {
        oldnick = c->username;
    }

    buffer = bformat(
        ":%s %s :%s!%s@%s NICK %s",
        bdata( s->self.remote ),
        bdata( c->nick ),
        bdata( oldnick ),
        bdata( c->username ),
        bdata( c->remote ),
        bdata( c->nick )
    );
    client_send( c, buffer );

    bdestroy( buffer );

cleanup:

    return;
}

void parser_server_reply_motd( void* local, void* remote ) {
    bstring buffer;
    CLIENT* c = (CLIENT*)remote;
    SERVER* s = (SERVER*)local;

    if( 1 > blength( c->nick ) ) {
        goto cleanup;
    }

    if( c->flags & CLIENT_FLAGS_HAVE_MOTD ) {
        goto cleanup;
    }

    buffer = bformat(
        ":%s 433 %s :MOTD File is missing",
        bdata( s->self.remote ),
        bdata( c->nick )
    );
    client_send( c, buffer );

    c->flags |= CLIENT_FLAGS_HAVE_MOTD;

    bdestroy( buffer );

cleanup:

    return;
}

static void parser_server_user( void* local, void* remote, struct bstrList* args ) {
    CLIENT* c = (CLIENT*)remote;
    SERVER* s = (SERVER*)local;
    int i,
        consumed = 0;
    bstring buffer = NULL;

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
        buffer = bformat(
            ":%s 433 %s :Nickname is already in use",
            bdata( s->self.remote ),
            bdata( c->username )
        );
        client_send( c, buffer );
    }

    //scaffold_print_debug( "User: %s, Real: %s, Remote: %s\n", bdata( c->username ), bdata( c->realname ), bdata( c->remote ) );

    c->flags |= CLIENT_FLAGS_HAVE_USER;

    if( ( c->flags & CLIENT_FLAGS_HAVE_NICK ) ) {
        parser_server_reply_welcome( local, remote );
    }
    //parser_server_reply_nick( local, remote, NULL );

cleanup:

    bdestroy( buffer );

    return;
}

static void parser_server_nick( void* local, void* remote, struct bstrList* args ) {
    CLIENT* c = (CLIENT*)remote;
    SERVER* s = (SERVER*)local;
    bstring oldnick = NULL;
    bstring newnick = NULL;
    bstring buffer = NULL;
    int nick_return;

    if( 2 >= args->qty ) {
        newnick = args->entry[1];
    }

    nick_return = server_set_client_nick( s, c, newnick );
    if( ERR_NONICKNAMEGIVEN == nick_return ) {
        buffer = bformat(
            ":%s 431 %s :No nickname given",
            bdata( s->self.remote ),
            bdata( c->nick )
        );
        client_send( c, buffer );
        goto cleanup;
    }

    if( ERR_NICKNAMEINUSE == nick_return ) {
        buffer = bformat(
            ":%s 433 %s :Nickname is already in use",
            bdata( s->self.remote ),
            bdata( c->nick )
        );
        client_send( c, buffer );
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
    bdestroy( buffer );

    return;
}

static void parser_server_quit( void* local, void* remote, struct bstrList* args ) {
    SERVER* s = (SERVER*)local;
    CLIENT* c = (CLIENT*)remote;
    bstring message;
    bstring buffer;
    bstring space;

    /* TODO: Send everyone parting message. */
    space = bfromcstr( " " );
    message = bjoin( args, space );

    buffer = bformat(
        "ERROR :Closing Link: %s (Client Quit)",
        bdata( c->nick )
    );
    client_send( c, buffer );

    server_drop_client( s, c->link.socket );

    bdestroy( buffer );
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

cleanup:

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
    bstring reply = NULL;

    //bdestroy( scaffold_pop_string( args ) );
    msg = bjoin( args, &scaffold_space_string );

    c_dest = server_get_client_by_nick( s, args->entry[1], TRUE );
    if( NULL != c_dest ) {
        reply = bformat(
            ":%s!%s@%s %s",
            bdata( c->nick ),
            bdata( c->username ),
            bdata( c->remote ),
            bdata( msg )
        );
        client_send( c_dest, reply );
    }

    bdestroy( msg );
    bdestroy( reply );

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
