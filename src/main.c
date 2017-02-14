
#include "server.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

static SERVER* server;

/* TODO: Handle SIGINT */
void handle_interrupt( int arg ) {
    server->self.running = FALSE;
}

int main( int argc, char** argv ) {
    CLIENT* client;
    bstring localhost,
        buffer;

    localhost = bfromcstr( "127.0.0.1" );
    buffer = bfromcstr( "" );

    server = calloc( 1, sizeof( SERVER ) );
    client = calloc( 1, sizeof( CLIENT ) );

    server_init( server, localhost );
    client_init( client );

    do {
        server_listen( server, 33080 );
        usleep( 1000000 );
    } while( 0 != scaffold_error );

    signal( SIGINT, handle_interrupt );

    bdestroy( client->nick );
    client->nick = bfromcstr( "TestNick" );
    bdestroy( client->realname );
    client->realname = bfromcstr( "Tester Tester" );
    bdestroy( client->username );
    client->username = bfromcstr( "TestUser" );

    do {
        client_connect( client, localhost, 33080 );
        usleep( 1000000 );
    } while( 0 != scaffold_error );

    while( TRUE ) {

        usleep( 2000000 );

        server_service_clients( server );
        client_update( client );

#if 0
        if( 3 > count ) {
            bassigncstr( buffer, "foo" );
        } else {
            bassigncstr( buffer, "stop " );
        }

        client_send( client, buffer );

        count++;
#endif

        //connection_lock( &(server->server_connection) );
        if( !server->self.running ) {
            //connection_unlock( &(server->server_connection) );
            break;
        }
        //connection_unlock( &(server->server_connection) );
    }

    bdestroy( localhost );
    bdestroy( buffer );
    server_cleanup( server );
    free( server );

    return 0;
}
