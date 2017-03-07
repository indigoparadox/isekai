
#ifndef REF_H
#define REF_H

#include "scaffold.h"

#define REF_SENTINAL 170
#define REF_DISABLED {0, NULL, 0}

#ifndef USE_THREADS

typedef struct REF {
   uint8_t sentinal;
   void (*gc_free )( const struct REF* );
   int count;
} REF;

static inline void ref_init( struct REF* ref, void (*free)( const struct REF* ) ) {
   ref->count = 1;
   ref->gc_free = free;
   ref->sentinal = REF_SENTINAL;
}

static inline void ref_inc( const struct REF* ref ) {
   assert( REF_SENTINAL == ref->sentinal );
#ifdef DEBUG_REF
   scaffold_print_debug( "Reference count increased: %d\n", ref->count );
#endif /* DEBUG_REF */
   ((struct REF*)ref)->count++;
}

static inline BOOL ref_dec( const struct REF* ref ) {
   assert( REF_SENTINAL == ref->sentinal );
#ifdef DEBUG_REF
   scaffold_print_debug( "Reference count decreased: %d\n", ref->count );
#endif /* DEBUG_REF */
   if( 0 == --((struct REF*)ref)->count ) {
#ifdef DEBUG_REF
      scaffold_print_debug( "Object freed: %d\n", ref->count );
#endif /* DEBUG_REF */
      ref->gc_free( ref );
      return TRUE;
   }
   return FALSE;
}

static inline BOOL ref_test_inc( void* data ) {
   uint8_t* data_uint = (uint8_t*)data;
   if( REF_SENTINAL == *data_uint ) {
      ref_inc( (struct REF*)data );
      return TRUE;
   }
   return FALSE;
}

static inline BOOL ref_test_dec( void* data ) {
   uint8_t* data_uint = (uint8_t*)data;
   if( REF_SENTINAL == *data_uint ) {
      return ref_dec( (struct REF*)data );
   }
   return FALSE;
}

#endif /* USE_THREADS */

#endif /* REF_H */
