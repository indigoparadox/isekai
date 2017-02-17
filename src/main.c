
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
    time_t tm;
    GRAPHICS g;
    INPUT p;
    GAMEDATA d;
    UI ui;

    srand( (unsigned)time( &tm ) );

    graphics_init_screen( &g, 640, 480 );
    input_init( &p );
    ui_init( &ui, &g );

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

    gamedata_init_client( &d, &ui, channel );

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

        usleep( 500000 );

        client_update( client, &d );
        server_service_clients( server );

        if( 'q' == input_get_char( &p ) ) {
            server_stop( server );
        }

        if( 0 < vector_count( &(client->channels) ) ) {
            graphics_draw_text( &g, 20, 20, ((CHANNEL*)(vector_get( &(client->channels), 0 )))->name );
            graphics_draw_text( &g, 20, 40, ((CHANNEL*)(vector_get( &(client->channels), 0 )))->topic );
        }

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
