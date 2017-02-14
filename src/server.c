
#include "server.h"

#include <stdlib.h>

void server_init( SERVER* s, bstring myhost ) {
    client_init( &(s->self) );
    vector_init( &(s->clients) );
    s->self.remote = bstrcpy( myhost );
    s->servername =  blk2bstr( bsStaticBlkParms( "ProCIRCd" ) );
    s->version = blk2bstr(  bsStaticBlkParms( "0.1" ) );
    s->self.sentinal = SERVER_SENTINAL;
}

void server_cleanup( SERVER* s ) {
    int i;
    CLIENT* c;

    /* TODO: Remove clients. */
    for( i = 0 ; vector_count( &(s->clients) ) > i ; i++ ) {
        c = (CLIENT*)vector_get( &(s->clients), i );
        client_cleanup( c );
    }
    vector_free( &(s->clients) );

    client_cleanup( &(s->self) );
}

void server_add_client( SERVER* s, CLIENT* n ) {
    connection_lock( &(s->self.link) );
    vector_add( &(s->clients), n );
    connection_unlock( &(s->self.link) );
}

CLIENT* server_get_client( SERVER* s, int index ) {
    CLIENT* c;
    connection_lock( &(s->self.link) );
    c = vector_get( &(s->clients), index );
    connection_unlock( &(s->self.link) );
    return c;
}

static int server_get_client_index_by_socket( SERVER* s, int socket, BOOL lock ) {
    CLIENT* c = NULL;
    int i;

    if( lock ) {
        connection_lock( &(s->self.link) );
    }
    for( i = 0 ; vector_count( &(s->clients) ) > i ; i++ ) {
        c = vector_get( &(s->clients), i );
        if( socket == c->link.socket ) {
            /* Skip the reset below. */
            goto cleanup;
        }
    }
    i = -1;

cleanup:

    if( lock ) {
        connection_unlock( &(s->self.link) );
    }

    return i;
}

CLIENT* server_get_client_by_nick( SERVER* s, bstring nick, BOOL lock ) {
    CLIENT* c = NULL;
    int i;

    if( lock ) {
        connection_lock( &(s->self.link) );
    }

    for( i = 0 ; vector_count( &(s->clients) ) > i ; i++ ) {
        c = vector_get( &(s->clients), i );
        /* scaffold_print_debug( "%s vs %s: %d\n", bdata( c->nick ), bdata( nick ), bstrcmp( c->nick, nick ) ); */
        if( 0 == bstrcmp( c->nick, nick ) ) {
            /* Skip the reset below. */
            goto cleanup;
        }
    }

    c = NULL;

cleanup:

    if( lock ) {
        connection_unlock( &(s->self.link) );
    }

    return c;
}

void server_drop_client( SERVER* s, int socket ) {
    CLIENT* c;
    int index;

    connection_lock( &(s->self.link) );

    index = server_get_client_index_by_socket( s, socket, FALSE );

    c = vector_get( &(s->clients), index );

    //connection_cleanup_socket( &(c->link) );
    client_cleanup( c );

    vector_delete( &(s->clients), index );

    connection_unlock( &(s->self.link) );
}

void server_listen( SERVER* s, int port ) {
    s->self.link.arg = s;
    connection_listen( &(s->self.link), port );
    if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
        scaffold_print_error( "Unable to bind to specified port. Exiting.\n" );
    }
}

void server_service_clients( SERVER* s ) {
    ssize_t last_read_count = 0;
    CLIENT* c = NULL;
    CONNECTION* n_client = NULL;
    int i = 0;

    /* Check for new clients. */
    n_client = connection_register_incoming( &(s->self.link) );
    if( NULL != n_client ) {
        client_new( c );
        memcpy( &(c->link), n_client, sizeof( CONNECTION ) );
        free( n_client ); /* Don't clean up, because data is still valid. */

        /* Add some details to c before stowing it. */
        connection_assign_remote_name( &(c->link), c->remote );

        server_add_client( s, c );
    }

    /* Check for commands from existing clients. */
    for( i = 0 ; vector_count( &(s->clients) ) > i ; i++ ) {
        c = vector_get( &(s->clients), i );

        btrunc( s->self.buffer, 0 );
        last_read_count = connection_read_line( &(c->link), s->self.buffer );
        btrimws( s->self.buffer );

        if( 0 >= last_read_count ) {
            /* TODO */
            continue;
        }

        scaffold_print_debug(
            "Server: Line received from %d: %s\n",
            c->link.socket, bdata( s->self.buffer )
        );

        parser_dispatch( s, c, s->self.buffer );

#if 0
        /* Send a reply if we need to. */

        /* WELCOME not sent, yet. */
        if( !(c->flags & CLIENT_FLAGS_HAVE_WELCOME) ) {

            /* USER and NICK received. */
            if(
                (c->flags & CLIENT_FLAGS_HAVE_USER &&
                    c->flags & CLIENT_FLAGS_LAST_NICK)
            ) {
                parser_server_reply_welcome( s, c );
                goto cleanup;
            }

            goto cleanup;
        }

        /* NICK received. */
        if( c->flags & CLIENT_FLAGS_LAST_NICK ) {
            parser_server_reply_nick( s, c );
            c->flags &= ~CLIENT_FLAGS_LAST_NICK;
            goto cleanup;
        }

        if( !(c->flags & CLIENT_FLAGS_HAVE_MOTD) ) {
            parser_server_reply_motd( s, c );
            goto cleanup;
        }
#endif
    }

cleanup:

    return;
}

void server_stop( SERVER* s ) {
    s->self.running = FALSE;
}
