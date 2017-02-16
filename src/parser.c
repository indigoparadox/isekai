
#include "parser.h"

#include "server.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* This file contains our (possibly limited, slightly incompatible) version *
 * of the IRC protocol, as it interacts with our server and client objects. oopen game datapen game data*/

static void parser_server_reply_welcome( void* local, void* remote ) {
    CLIENT* c = (CLIENT*)remote;
    SERVER* s = (SERVER*)local;

    client_printf(
        c, ":%b 001 %b :Welcome to the Internet Relay Network %b!%b@%b",
        s->self.remote, c->nick, c->nick, c->username, c->remote
    );

    client_printf(
        c, ":%b 002 %b :Your host is %b, running version %b",
        s->self.remote, c->nick, s->servername, s->version
    );

    client_printf(
        c, ":%b 003 %b :This server was created 01/01/1970",
        s->self.remote, c->nick
    );

    client_printf(
        c, ":%b 004 %b :%b %b-%b abBcCFiIoqrRswx abehiIklmMnoOPqQrRstvVz",
        s->self.remote, c->nick, s->self.remote, s->servername, s->version
    );

    client_printf(
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

    client_printf(
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

    client_printf(
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
    char* c_mode = NULL;

    /* TODO: Error on already registered. */

    /* Start at 1, skipping the command, itself. */
    for( i = 1 ; args->qty > i ; i++ ) {
        if( 0 == consumed ) {
            /* First arg: User */
            scaffold_copy_string( c->username, args->entry[i] );
        } else if( 1 == consumed && scaffold_is_numeric( args->entry[i] ) ) {
            /* Second arg: Mode */
            c_mode = bdata( args->entry[i] );
            c->mode = atoi( c_mode );
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
        client_printf(
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
        client_printf(
            c, ":%b 431 %b :No nickname given",
            s->self.remote, c->nick
        );
        goto cleanup;
    }

    if( ERR_NICKNAMEINUSE == nick_return ) {
        client_printf(
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

    client_printf(
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

    server_lock_clients( s, TRUE );

    for( i = 1 ; args->qty > i ; i++ ) {
        if( NULL != server_get_client_by_nick(
            s, args->entry[i], FALSE
        ) ) {
            bconcat( reply, args->entry[i] );
            if( args->qty - 1 != i ) {
                bconchar( reply, ' ' );
            }
        }
    }

    server_lock_clients( s, FALSE );

    client_send( c, reply );
    bdestroy( reply );

    return;
}

static void parser_server_join( void* local, void* remote, struct bstrList* args ) {
    SERVER* s = (SERVER*)local;
    CLIENT* c = (CLIENT*)remote;
    CHANNEL* l = NULL;
    int i;
    CLIENT* c_iter = NULL;
    bstring namehunt = NULL;
    int bstr_result = 0;
    bstring names = NULL;
    int clients_count;

    if( 2 > args->qty ) {
        client_printf(
            c, ":%b 461 %b %b :Not enough parameters",
            s->self.remote, c->username, args->entry[0]
        );
        goto cleanup;
    }

    namehunt = bstrcpy( args->entry[1] );
    bstr_result = btrimws( namehunt );
    scaffold_check_nonzero( bstr_result );

    if( TRUE != scaffold_string_is_printable( namehunt ) ) {
        client_printf(
            c, ":%b 403 %b %b :No such channel",
            s->self.remote, c->username, namehunt
        );
        goto cleanup;
    }

    /* Get the channel, or create it if it does not exist. */
    l = server_get_channel_by_name( s, namehunt );
    if( NULL == l ) {
        l = calloc( 1, sizeof( CHANNEL ) );
        scaffold_check_null( l );
        channel_init( l, namehunt );
        server_add_channel( s, l );
        scaffold_print_info( "Channel created: %s\n", bdata( l->name ) );
    }

    /* Make sure the user is not already in the channel. If they are, then  *
     * just shut up and explode.                                            */
    if( NULL != channel_get_client_by_name( l, c->nick ) ) {
        scaffold_print_debug(
            "%s already in channel %s; ignoring.\n",
            bdata( c->nick ), bdata( l->name )
        );
        goto cleanup;
    }

    /* Announce the new join. */
    channel_printf(
        l, c, ":%b!%b@%b JOIN %b", c->nick, c->username, c->remote, l->name
    );

    channel_add_client( l, c );

    /* Now tell the joining client. */
    client_printf(
        c, ":%b!%b@%b JOIN %b",
        c->nick, c->username, c->remote, l->name
    );

    names = bfromcstr( "" );
    channel_lock_clients( l, TRUE );
    clients_count = vector_count( &(l->clients) );
    for( i = 0 ; clients_count > i ; i++ ) {
        c_iter = (CLIENT*)vector_get( &(l->clients), i );
        bconcat( names, c_iter->nick );
        if( clients_count - 1 != i ) {
            bconchar( names, ' ' );
        };
    }
    channel_lock_clients( l, FALSE );

    client_printf(
        c, ":%b 332 %b %b :%b",
        s->self.remote, c->nick, l->name, l->topic
    );

    client_printf(
        c, ":%b 353 %b = %b :%b",
        s->self.remote, c->nick, l->name, names
    );

    client_printf(
        c, ":%b 366 %b %b :End of NAMES list",
        s->self.remote, c->nick, l->name
    );

cleanup:
    bdestroy( names );
    bdestroy( namehunt );
    return;
}

static void parser_server_part( void* local, void* remote, struct bstrList* args ) {
}

static void parser_server_privmsg( void* local, void* remote, struct bstrList* args ) {
    SERVER* s = (SERVER*)local;
    CLIENT* c = (CLIENT*)remote;
    CLIENT* c_dest = NULL;
    CHANNEL* l_dest = NULL;
    bstring msg = NULL;

    //bdestroy( scaffold_pop_string( args ) );
    msg = bjoin( args, &scaffold_space_string );

    c_dest = server_get_client_by_nick( s, args->entry[1], TRUE );
    if( NULL != c_dest ) {
        client_printf(
            c_dest, ":%b!%b@%b %b", c->nick, c->username, c->remote, msg
        );
        goto cleanup;
    }

    /* Maybe it's for a channel, instead? */
    l_dest = server_get_channel_by_name( s, args->entry[1] );
    if( NULL != l_dest ) {
        channel_printf(
            l_dest, c, ":%b!%b@%b %b", c->nick, c->username, c->remote, msg
        );
        goto cleanup;
    }

    /* TODO: Handle error? */

cleanup:
    bdestroy( msg );
    return;
}

static void parser_server_who( void* local, void* remote, struct bstrList* args ) {
    SERVER* s = (SERVER*)local;
    CLIENT* c = (CLIENT*)remote;
    CHANNEL* l = NULL;
    CLIENT* c_iter = NULL;
    int i;

    l = server_get_channel_by_name( s, args->entry[1] );
    scaffold_check_null( l );

    /* Announce the new join. */
    channel_lock_clients( l, TRUE );
    for( i = 0 ; vector_count( &(l->clients) ) > i ; i++ ) {
        c_iter = (CLIENT*)vector_get( &(l->clients), i );
        client_printf(
            c, ":%b RPL_WHOREPLY %b %b",
            s->self.remote, c_iter->nick, l->name
        );
    }
    channel_lock_clients( l, FALSE );

cleanup:
    return;
}

/* GU #channel px+1y+2z+3 */
static void parser_server_gu( void* local, void* remote, struct bstrList* args ) {
    SERVER* s = (SERVER*)local;
    CLIENT* c = (CLIENT*)remote;
    CHANNEL* l = NULL;
    bstring reply_c = NULL;
    bstring reply_l = NULL;
    struct bstrList gu_args;

    /* Strip off the command "header". */
    memcpy( &gu_args, args, sizeof( struct bstrList ) );
    scaffold_pop_string( &gu_args ); /* Source */
    scaffold_pop_string( &gu_args ); /* Channel */

    /* Find out if this command affects a certain channel. */
    if( 2 <= args->qty && '#' == bdata( args->entry[1] )[0] ) {
        l = server_get_channel_by_name( s, args->entry[1] );
        scaffold_check_null( l );
    }

    gamedata_update( &(l->gamedata), c, args, &reply_c, &reply_l );

    /* TODO: Make sure client is actually in the channel requested. */
    c = channel_get_client_by_name( l, c->nick );
    scaffold_check_null( c );

    if( NULL != reply_c ) {
        client_printf( c, "%b", reply_c );
    }

    if( NULL != reply_l ) {
        channel_printf( l, c, "%b", reply_l );
    }

cleanup:
    bdestroy( reply_l );
    bdestroy( reply_c );
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
    {bsStatic( "WHO" ), parser_server_who},
    {bsStatic( "GU" ), parser_server_gu },
    {bsStatic( "" ), NULL}
};

void parser_dispatch( void* local, void* remote, const_bstring line ) {
    SERVER* s_local = (SERVER*)local;
    const parser_entry* parser_table = NULL;
    struct bstrList* args = NULL;
    const parser_entry* command = NULL;

    if( SERVER_SENTINAL == s_local->self.sentinal ) {
        parser_table = parser_table_server;
    } else {
        scaffold_print_error( "ERROR: Client dispatch table not implemented.\n" );
        goto cleanup;
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
