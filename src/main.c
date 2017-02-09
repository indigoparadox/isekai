
#include "server.h"

int main( int argc, char** argv ) {
    SERVER server;

    server_init( &server );

    server_listen( &server );

    getchar();

    return 0;
}
