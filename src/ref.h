
#ifndef REF_H
#define REF_H

#include "scaffold.h"

#define REF_SENTINAL 170
#define REF_DISABLED {0, NULL, 0}

/* This is a hack to turn on inline for the ref_* static functions to avoid
 * -Wunused-function in gcc. as it ignores inline functions. Still lets us
 * turn off inlines for ancient compilers without messing with other inlines,
 * though.
 */
#define REF_INLINE inline

#ifndef USE_THREADS

struct REF {
   uint8_t sentinal;
   void (*gc_free )( const struct REF* );
   SCAFFOLD_SIZE count;
};

static REF_INLINE void ref_init( struct REF* ref, void (*free)( const struct REF* ) ) {
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

static REF_INLINE void ref_inc( const struct REF* ref, const char* type, const char* func ) {
   assert( REF_SENTINAL == ref->sentinal );
   ((struct REF*)ref)->count++;
#ifdef DEBUG_REF
   if( NULL != type && NULL != func ) {
      lg_debug(
         __FILE__,
         "%s: Reference count for %s increased: %d\n", func, type, ref->count
      );
   }
#endif /* DEBUG_REF */
}

static REF_INLINE bool ref_dec( const struct REF* ref, const char* type, const char* func ) {
   assert( REF_SENTINAL == ref->sentinal );

   if( 1 == ((struct REF*)ref)->count ) {
#ifdef DEBUG_REF
      if( NULL != type && NULL != func ) {
         lg_debug( __FILE__, "%s: %s freed.\n", func, type );
      }
#endif /* DEBUG_REF */
      ((struct REF*)ref)->count = 0;
      ref->gc_free( ref );
      return true;
   } else if( 0 == ((struct REF*)ref)->count ) {
      lg_error( __FILE__, "Reference count negative!\n" );
      goto cleanup;
   }

   --((struct REF*)ref)->count;

#ifdef DEBUG_REF
   if( NULL != type && NULL != func ) {
      lg_debug(
         &ref_module,
         "%s: Reference count for %s decreased: %d\n", func, type, ref->count
      );
   }
#endif /* DEBUG_REF */

cleanup:
   return false;
}

#ifdef ENABLE_REF_TEST

static SCAFFOLD_INLINE bool ref_test_inc( void* data, const char* func ) {
   uint8_t* data_uint = (uint8_t*)data;
   if( REF_SENTINAL == *data_uint ) {
      ref_inc( (struct REF*)data, "void", func );
      return true;
   }
   return false;
}

static SCAFFOLD_INLINE bool ref_test_dec( void* data, const char* func ) {
   uint8_t* data_uint = (uint8_t*)data;
   if( REF_SENTINAL == *data_uint ) {
      return ref_dec( (struct REF*)data, "void", func );
   }
   return false;
}

#endif /* ENABLE_REF_TEST */

#endif /* USE_THREADS */

#endif /* REF_H */
