
#include "server.h"

int main( int argc, char** argv ) {
    SERVER* server;
    CLIENT* client;
    bstring localhost,
        buffer;
    int count = 0;

    localhost = bfromcstr( "localhost" );
    buffer = bfromcstr( "" );

    server = calloc( 1, sizeof( SERVER ) );
    client = calloc( 1, sizeof( CLIENT ) );

    server_init( server );
    client_init( client );

    server_listen( server, 33080 );
    client_connect( client, localhost, 33080 );

    while( TRUE ) {

        usleep( 2000000 );

        server_service_clients( server );
        client_update( client );

        if( 3 > count ) {
            bassigncstr( buffer, "foo" );
        } else {
            bassigncstr( buffer, "stop " );
        }

        client_send( client, buffer );

        count++;

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
