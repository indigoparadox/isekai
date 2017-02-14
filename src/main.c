
#include "server.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

static SERVER* server;

/* TODO: Handle SIGINT */
void handle_interrupt( int arg ) {
    server->self.running = FALSE;
}

int main( int argc, char** argv ) {
    CLIENT* client;
    bstring localhost,
        buffer,
        channel;
    time_t t;

    srand( (unsigned)time(&t) );

    localhost = bfromcstr( "127.0.0.1" );
    channel = bfromcstr( "#testchannel" );
    buffer = bfromcstr( "" );

    server_new( server, localhost );
    client_new( client );

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

    client_join_channel( client, channel );

    while( TRUE ) {

        usleep( 2000000 );

        server_service_clients( server );
        client_update( client );


        if( !server->self.running ) {
            break;
        }
    }

    bdestroy( localhost );
    bdestroy( buffer );
    bdestroy( channel );
    server_cleanup( server );
    free( server );

    return 0;
}
