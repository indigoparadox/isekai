
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

static SCAFFOLD_INLINE void ref_init( struct REF* ref, void (*free)( const struct REF* ) ) {
   ref->count = 1;
   ref->gc_free = free;
   ref->sentinal = REF_SENTINAL;
}

#define refcount_inc( obj, type ) \
   ref_inc( &((obj)->refcount), type, __FUNCTION__ )
#define refcount_dec( obj, type ) \
   ref_dec( &((obj)->refcount), type, __FUNCTION__ )
#define refcount_test_inc( obj ) ref_test_inc( obj, __FUNCTION__ )
#define refcount_test_dec( obj ) ref_test_dec( obj, __FUNCTION__ )

static SCAFFOLD_INLINE void ref_inc( const struct REF* ref, const char* type, const char* func ) {
   scaffold_assert( REF_SENTINAL == ref->sentinal );
   ((struct REF*)ref)->count++;
#ifdef DEBUG_REF
   if( NULL != type && NULL != func ) {
      scaffold_print_debug(
         &module,
         "%s: Reference count for %s increased: %d\n", func, type, ref->count
      );
   }
#endif /* DEBUG_REF */
}

static SCAFFOLD_INLINE BOOL ref_dec( const struct REF* ref, const char* type, const char* func ) {
   scaffold_assert( REF_SENTINAL == ref->sentinal );
   --((struct REF*)ref)->count;
#ifdef DEBUG_REF
   if( NULL != type && NULL != func ) {
      scaffold_print_debug(
         &module,
         "%s: Reference count for %s decreased: %d\n", func, type, ref->count
      );
   }
#endif /* DEBUG_REF */
   if( 0 >= ((struct REF*)ref)->count ) {
#ifdef DEBUG_REF
      if( NULL != type && NULL != func ) {
         scaffold_print_debug( &module, "%s: %s freed.\n", func, type );
      }
#endif /* DEBUG_REF */
      ref->gc_free( ref );
      return TRUE;
   }
   return FALSE;
}

#ifdef ENABLE_REF_TEST

static SCAFFOLD_INLINE BOOL ref_test_inc( void* data, const char* func ) {
   uint8_t* data_uint = (uint8_t*)data;
   if( REF_SENTINAL == *data_uint ) {
      ref_inc( (struct REF*)data, "void", func );
      return TRUE;
   }
   return FALSE;
}

static SCAFFOLD_INLINE BOOL ref_test_dec( void* data, const char* func ) {
   uint8_t* data_uint = (uint8_t*)data;
   if( REF_SENTINAL == *data_uint ) {
      return ref_dec( (struct REF*)data, "void", func );
   }
   return FALSE;
}

#endif /* ENABLE_REF_TEST */

#endif /* USE_THREADS */

#endif /* REF_H */
