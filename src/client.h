
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
    int sentinal;
} CLIENT;

#define CLIENT_SENTINAL 254542

void client_init( CLIENT* c );

#endif /* CLIENT_H */
