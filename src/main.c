
#include "server.h"

int main( int argc, char** argv ) {
    SERVER* server;

    server = calloc( 1, sizeof( SERVER ) );

    server_init( server );

    server_listen( server );

    while( TRUE ) {

        usleep( 2000 );

        //connection_lock( &(server->server_connection) );
        if( !server->running ) {
            //connection_unlock( &(server->server_connection) );
            break;
        }
        //connection_unlock( &(server->server_connection) );
    }

    server_cleanup( server );
    free( server );

    return 0;
}
