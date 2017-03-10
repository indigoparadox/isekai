
#ifndef SYNCBUFF_H
#define SYNCBUFF_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef _WIN32
#include "../scaffold.h"
#endif /* _WIN32 */

#include "../bstrlib/bstrlib.h"

/* TODO: Minimal ifdefs for scaffold. */

#ifdef USE_SIZET
#else
typedef uint32_t SCAFFOLD_SIZE;
typedef int32_t SCAFFOLD_SIZE_SIGNED;
#endif /* USE_SIZET */

typedef enum _SYNCBUFF_DEST {
   SYNCBUFF_DEST_SERVER,
   SYNCBUFF_DEST_CLIENT
} SYNCBUFF_DEST;

SCAFFOLD_SIZE syncbuff_get_count( SYNCBUFF_DEST dest );
SCAFFOLD_SIZE syncbuff_get_allocated( SYNCBUFF_DEST dest );
uint8_t syncbuff_listen();
uint8_t syncbuff_connect();
uint8_t syncbuff_accept();
ssize_t syncbuff_write( const bstring line, SYNCBUFF_DEST dest );
ssize_t syncbuff_read( bstring buffer, SYNCBUFF_DEST dest );

#endif /* SYNCBUFF_H */
