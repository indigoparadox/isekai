
#include "server.h"

#include "ui.h"
#include "input.h"

#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

static SERVER* server;

static struct tagbstring str_loading = bsStatic( "Loading..." );

/* TODO: Handle SIGINT */
void handle_interrupt( int arg ) {
    server->self.running = FALSE;
}

int main( int argc, char** argv ) {
    CLIENT* client = NULL;
    bstring localhost = NULL,
        buffer = NULL,
        channel = NULL;
    time_t t;
    GRAPHICS g;
    INPUT p;
    GAMEDATA d;

    srand( (unsigned)time(&t) );

    graphics_init_screen( &g, 640, 480 );
    input_init( &p );

    graphics_draw_text( &g, 20, 20, &str_loading );
    graphics_flip_screen( &g );

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

    gamedata_init( &d, channel );

    bdestroy( client->nick );
    client->nick = bfromcstr( "TestNick" );
    bdestroy( client->realname );
    client->realname = bfromcstr( "Tester Tester" );
    bdestroy( client->username );
    client->username = bfromcstr( "TestUser" );
    d.screen = &g;

    do {
        client_connect( client, localhost, 33080 );
        usleep( 1000000 );
    } while( 0 != scaffold_error );

    client_join_channel( client, channel );

    while( TRUE ) {

        usleep( 500000 );

        client_update( client, &d );
        server_service_clients( server );

        if( !server->self.running ) {
            break;
        }
    }

    bdestroy( localhost );
    bdestroy( buffer );
    bdestroy( channel );
    server_cleanup( server );
    free( server );
    graphics_shutdown( &g );

    return 0;
}
