
#define MEM_C
#include "mem.h"

/** \brief Check to make sure the given count/size combination will not result
 *         in an overflow, creating a memory buffer with an unexpected size.
 * \param
 * \param
 * \return TRUE if there is an overflow, otherwise FALSE.
 */
static BOOL mem_check_overflow( SCAFFOLD_SIZE count, SCAFFOLD_SIZE sz ) {
   SCAFFOLD_SIZE sz_check = count * sz;
   if( sz_check < count ) {
      return TRUE;
   }
   return FALSE;
}

void* mem_alloc_internal( SCAFFOLD_SIZE count, SCAFFOLD_SIZE sz ) {
   if( FALSE == mem_check_overflow( count, sz ) ) {
      return calloc( count, sz );
   }
   return NULL;
}

void* mem_realloc_internal( void* ptr, SCAFFOLD_SIZE count, SCAFFOLD_SIZE sz ) {
   if( FALSE == mem_check_overflow( count, sz ) ) {
      return realloc( ptr, count * sz );
   }
   return NULL;
}

void* mem_free_internal( void* ptr ) {
   free( ptr );
   return NULL;
}
