
#ifndef MEM_H
#define MEM_H

#include "scaffold.h"

#define mem_alloc( count, type ) \
   (type*)mem_alloc_internal( count, sizeof( type ) )
#define mem_free( ptr ) free( ptr )
#define mem_realloc( ptr, count, type ) \
   (type*)mem_realloc_internal( ptr, count, sizeof( type ) )

void* mem_alloc_internal( SCAFFOLD_SIZE count, SCAFFOLD_SIZE sz );
void* mem_realloc_internal( void* ptr, SCAFFOLD_SIZE count, SCAFFOLD_SIZE sz );
BOOL scaffold_buffer_grow(
   BYTE** buffer, SCAFFOLD_SIZE* len, SCAFFOLD_SIZE new_len
)
#ifdef __GNUC__
__attribute__ ((warn_unused_result))
#endif /* __GNUC__ */
;

#ifdef MEM_C
SCAFFOLD_MODULE( "mem.c" );
#endif /* MEM_C */

#endif /* MEM_H */
