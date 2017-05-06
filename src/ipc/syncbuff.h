
#ifndef SYNCBUFF_H
#define SYNCBUFF_H

#include <stddef.h>

#include "../scaffold.h"
#include "../bstrlib/bstrlib.h"

/* TODO: Minimal ifdefs for scaffold. */

/*
*/

/*
SCAFFOLD_SIZE syncbuff_get_count( SYNCBUFF_DEST dest );
SCAFFOLD_SIZE syncbuff_get_allocated( SYNCBUFF_DEST dest );
uint8_t syncbuff_listen();
uint8_t syncbuff_connect();
uint8_t syncbuff_accept();
SCAFFOLD_SIZE_SIGNED syncbuff_write( const bstring line, SYNCBUFF_DEST dest );
SCAFFOLD_SIZE_SIGNED syncbuff_read( bstring buffer, SYNCBUFF_DEST dest );
*/

#ifdef SYNCBUFF_C
SCAFFOLD_MODULE( "syncbuff.c" );
#endif /* SYNCBUFF_C */

#endif /* SYNCBUFF_H */
