
#include "server.h"

int main( int argc, char** argv ) {
    SERVER server;

    server_init( &server );

    server_listen( &server );

    while( TRUE ) {

        connection_lock( &(server.server_connection) );
        if( !server.running ) {
            break;
        }
        connection_unlock( &(server.server_connection) );
    }

    return 0;
}
