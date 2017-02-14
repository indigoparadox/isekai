
#ifndef CLIENT_H
#define CLIENT_H

#include "bstrlib/bstrlib.h"
#include "vector.h"
#include "connection.h"

typedef struct {
    /* "Parent class" */
    CONNECTION link;

    /* Items shared between server and client. */
    BOOL running;
    bstring buffer;
    bstring nick;
    bstring username;
    bstring realname;
    bstring remote;
    uint8_t mode;
    uint8_t flags;
    int sentinal;
} CLIENT;

#define CLIENT_FLAGS_HAVE_USER 0x01
#define CLIENT_FLAGS_HAVE_WELCOME 0x02
#define CLIENT_FLAGS_HAVE_MOTD 0x04
#define CLIENT_FLAGS_HAVE_NICK 0x08

#define CLIENT_SENTINAL 254542

void client_init( CLIENT* c );
void client_cleanup( CLIENT* c );
void client_connect( CLIENT* c, bstring server, int port );
void client_update( CLIENT* c );
void client_send( CLIENT* c, bstring buffer );

#endif /* CLIENT_H */
