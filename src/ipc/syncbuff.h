
#ifndef SYNCBUFF_H
#define SYNCBUFF_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "../bstrlib/bstrlib.h"

typedef enum _SYNCBUFF_DEST {
   SYNCBUFF_DEST_SERVER,
   SYNCBUFF_DEST_CLIENT
} SYNCBUFF_DEST;

uint8_t syncbuff_listen();
uint8_t syncbuff_connect();
uint8_t syncbuff_accept();
ssize_t syncbuff_write( const bstring line, SYNCBUFF_DEST dest );
ssize_t syncbuff_read( bstring buffer, SYNCBUFF_DEST dest );

#endif /* SYNCBUFF_H */
