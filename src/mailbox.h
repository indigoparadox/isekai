
#ifndef MAILBOX_H
#define MAILBOX_H

#include "vector.h"
#include "ref.h"

#define MAILBOX_SOCKET_NONE -1

struct MAILBOX;
struct MAILBOX_ENVELOPE;

typedef void (*MAILBOX_CALLBACK)( struct MAILBOX* m, struct MAILBOX_ENVELOPE* e );

typedef enum _MAILBOX_ENVELOPE_SPECIAL {
   MAILBOX_ENVELOPE_SPECIAL_NONE,
   MAILBOX_ENVELOPE_SPECIAL_CONNECT,
   MAILBOX_ENVELOPE_SPECIAL_DELETE,
} MAILBOX_ENVELOPE_SPECIAL;

struct MAILBOX_ENVELOPE {
   struct REF refcount;
   size_t socket_src;
   size_t socket_dest;
   bstring contents;
   MAILBOX_ENVELOPE_SPECIAL special;
   MAILBOX_CALLBACK callback;
   void* cb_arg;
};

struct MAILBOX {
   struct VECTOR envelopes;
   ssize_t last_socket;
   struct VECTOR sockets_assigned;
};

size_t mailbox_listen( struct MAILBOX* mailbox );
void* mailbox_envelope_cmp( struct VECTOR* v, void* iter, void* arg );
size_t mailbox_accept( struct MAILBOX* mailbox, size_t socket_dest );
void mailbox_call( struct MAILBOX* mailbox, ssize_t socket_src, MAILBOX_CALLBACK callback, void* arg );
void mailbox_send(
   struct MAILBOX* mailbox, size_t socket_src, size_t socket_dest, bstring message
);
size_t mailbox_connect(
   struct MAILBOX* mailbox, ssize_t socket_src, size_t socket_dest
);
size_t mailbox_read( struct MAILBOX* mailbox, size_t socket_dest, bstring buffer );
void mailbox_close( struct MAILBOX* mailbox, ssize_t socket );
BOOL mailbox_is_alive( struct MAILBOX* mailbox, ssize_t socket );

#endif /* MAILBOX_H */
