
#ifndef MAILBOX_H
#define MAILBOX_H

#include "vector.h"

#define MAILBOX_SOCKET_NONE -1

typedef enum _MAILBOX_ENVELOPE_SPECIAL {
   MAILBOX_ENVELOPE_SPECIAL_NONE,
   MAILBOX_ENVELOPE_SPECIAL_CONNECT,
} MAILBOX_ENVELOPE_SPECIAL;

typedef struct _MAILBOX_ENVELOPE {
    size_t socket_src;
    size_t socket_dest;
    bstring contents;
    MAILBOX_ENVELOPE_SPECIAL special;
} MAILBOX_ENVELOPE;

typedef struct _MAILBOX {
   VECTOR envelopes;
   ssize_t last_socket;
   //size_t server_socket;
} MAILBOX;

size_t mailbox_listen( MAILBOX* mailbox );
void* mailbox_envelope_cmp( VECTOR* v, void* iter, void* arg );
size_t mailbox_accept( MAILBOX* mailbox, size_t socket_dest );
void mailbox_send(
   MAILBOX* mailbox, size_t socket_src, size_t socket_dest, bstring message
);
size_t mailbox_connect(
   MAILBOX* mailbox, ssize_t socket_src, size_t socket_dest
);
size_t mailbox_read( MAILBOX* mailbox, size_t socket_dest, bstring buffer );


#endif /* MAILBOX_H */
