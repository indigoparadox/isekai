
#ifndef REF_H
#define REF_H

#include "scaffold.h"

#define REF_SENTINAL 170

#ifndef USE_THREADS

typedef struct _REF {
   uint8_t sentinal;
   void (*free)( const struct _REF* );
   int count;
} REF;

static inline void ref_init( struct _REF* ref, void (*free)( const struct _REF* ) ) {
   ref->count = 1;
   ref->free = free;
   ref->sentinal = REF_SENTINAL;
}

static inline void ref_inc( const struct _REF* ref ) {
   assert( REF_SENTINAL == ref->sentinal );
   ((struct _REF*)ref)->count++;
}

static inline BOOL ref_dec( const struct _REF* ref ) {
   assert( REF_SENTINAL == ref->sentinal );
   if( 0 == --((struct _REF*)ref)->count ) {
      ref->free( ref );
      return TRUE;
   }
   return FALSE;
}

static inline BOOL ref_test_inc( void* data ) {
   uint8_t* data_uint = (uint8_t*)data;
   if( REF_SENTINAL == *data_uint ) {
      ref_inc( (REF*)data );
      return TRUE;
   }
   return FALSE;
}

static inline BOOL ref_test_dec( void* data ) {
   uint8_t* data_uint = (uint8_t*)data;
   if( REF_SENTINAL == *data_uint ) {
      return ref_dec( (REF*)data );
   }
   return FALSE;
}

#endif /* USE_THREADS */

#endif /* REF_H */
