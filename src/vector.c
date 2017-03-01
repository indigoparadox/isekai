#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vector.h"
#include "ref.h"

void vector_init( VECTOR* v ) {
   v->data = NULL;
   v->size = 0;
   v->count = 0;
   v->scalar_data = NULL;
   v->sentinal = VECTOR_SENTINAL;
}

void vector_free( VECTOR* v ) {
   scaffold_check_null( v );
   assert( VECTOR_SENTINAL == v->sentinal );
   assert( 0 >= vector_count( v ) );

   if( FALSE != v->scalar ) {
      free( v->scalar_data );
   } else {
      free( v->data );
   }

cleanup:
   return;
}

void vector_add( VECTOR* v, void* data ) {
   scaffold_check_null( v );
   assert( VECTOR_SENTINAL == v->sentinal );

   assert( FALSE == v->scalar );

   vector_lock( v, TRUE );

   if( 0 == v->size ) {
      v->size = 10;
      v->data = (void**)calloc( v->size, sizeof(void*) );
      scaffold_check_null( v->data );
   }

   if( v->size == v->count ) {
      v->size *= 2;
      v->data = (void**)realloc( v->data, sizeof(void*) * v->size );
      scaffold_check_null( v->data );
      /* TODO: Clear new vector elements? */
   }

   v->data[v->count] = data;
   v->count++;
   ref_test_inc( data );

cleanup:

   vector_lock( v, FALSE );

   return;
}

void vector_add_scalar( VECTOR* v, int32_t value ) {
   size_t i;
   BOOL ok = FALSE;

   scaffold_check_null( v );
   assert( VECTOR_SENTINAL == v->sentinal );

   vector_lock( v, TRUE );
   ok = TRUE;

   /* Die if we have non-scalar data already. */
   assert( NULL == v->data );

   v->scalar = TRUE;

   for( i = 0 ; NULL != v->scalar_data && v->count > i ; i++ ) {
      if( v->scalar_data[i] == value ) {
         /* TODO: Proper scaffold error. */
         scaffold_error = 99;
         scaffold_print_error( "Attempting to add duplicate %d to scalar vector.\n", value );
         goto cleanup;
      }
   }

   if( 0 == v->size ) {
      v->size = 10;
      v->scalar_data = (int32_t*)calloc( v->size, sizeof( int32_t ) );
      scaffold_check_null( v->scalar_data );
   }

   if( v->size == v->count ) {
      v->size *= 2;
      v->scalar_data =
         (int32_t*)realloc( v->scalar_data, sizeof( int32_t ) * v->size );
      scaffold_check_null( v->scalar_data );
      /* TODO: Clear new vector elements? */
   }

   v->scalar_data[v->count] = value;
   v->count++;

cleanup:
   if( TRUE == ok ) {
      vector_lock( v, FALSE );
   }
   return;
}

void vector_set( VECTOR* v, size_t index, void* data ) {
   scaffold_check_null( v );

   assert( VECTOR_SENTINAL == v->sentinal );
   assert( FALSE == v->scalar );

   vector_lock( v, TRUE );

   scaffold_check_bounds( index, v->count );
   ref_test_dec( v->data[index] );
   v->data[index] = data;
   ref_test_inc( data );

cleanup:
   vector_lock( v, FALSE );
   return;
}

void* vector_get( VECTOR* v, size_t index ) {
   void* retptr = NULL;

   scaffold_check_null( v );
   assert( VECTOR_SENTINAL == v->sentinal );
   assert( FALSE == v->scalar );

   if( v->count <= index ) {
      goto cleanup;
   }

   retptr = v->data[index];

cleanup:

   return retptr;
}

int32_t vector_get_scalar( VECTOR* v, size_t value ) {
   int32_t retval = -1;
   int i;

   scaffold_check_null( v );
   assert( VECTOR_SENTINAL == v->sentinal );
   assert( TRUE == v->scalar );

   if( v->count <= 0 ) {
      goto cleanup;
   }

   for( i = 0 ; v->count > i ; i++ ) {
      if( value == v->scalar_data[i] ) {
         retval = value;
         goto cleanup;
      }
   }

cleanup:

   return retval;
}

/* Use a callback to delete items. The callback frees the item or decreases   *
 * its refcount as applicable.                                                */
size_t vector_remove_cb( VECTOR* v, vector_delete_cb callback, void* arg ) {
   size_t i;
   size_t backshift = 0;

   /* FIXME: Delete dynamic arrays and reset when empty. */

   scaffold_check_null( v );
   assert( VECTOR_SENTINAL == v->sentinal );
   assert( FALSE == v->scalar );

   vector_lock( v, TRUE );

   assert( 0 <= v->count );

   for( i = 0; v->count > i ; i++ ) {

      /* The delete callback should call the object-specific free() function, *
       * which decreases its refcount naturally. So there's no need to do it  *
       * manually here.                                                       */
      if( FALSE != callback( v, i, v->data[i], arg ) ) {
         backshift++;
      }

      if( v->count - 1 > i && 0 < backshift ) {
         /* The callback found a match, so start deleting! */
         v->data[i] = v->data[i + backshift];
      }
   }

   v->count -= backshift;

   vector_lock( v, FALSE );

cleanup:
   return backshift;
}

void vector_remove( VECTOR* v, size_t index ) {
   size_t i;

   /* FIXME: Delete dynamic arrays and reset when empty. */

   scaffold_check_null( v );
   assert( VECTOR_SENTINAL == v->sentinal );
   assert( FALSE == v->scalar );

   vector_lock( v, TRUE );

   scaffold_check_bounds( index, v->count );

   ref_test_dec( v->data[index] );

   for( i = index; v->count - 1 > i ; i++ ) {
      v->data[i] = v->data[i + 1];
   }

   v->count--;

cleanup:
   vector_lock( v, FALSE );
   return;
}

size_t vector_delete_scalar( VECTOR* v, int32_t value ) {
   size_t i;
   size_t difference = 0;

   /* FIXME: Delete dynamic arrays and reset when empty. */

   scaffold_check_null( v );
   assert( VECTOR_SENTINAL == v->sentinal );
   assert( TRUE == v->scalar );

   vector_lock( v, TRUE );

   for( i = 0; v->count - difference > i ; i++ ) {
      if( v->scalar_data[i] == value ) {
         difference++;
      }
      v->scalar_data[i] = v->scalar_data[i + difference];
   }

   v->count -= difference;

   vector_lock( v, FALSE );

cleanup:
   return difference;
}

inline size_t vector_count( VECTOR* v ) {
   scaffold_check_null( v );
   if( VECTOR_SENTINAL != v->sentinal ) {
      scaffold_error = SCAFFOLD_ERROR_OUTOFBOUNDS;
      goto cleanup;
   }
   return v->count;
cleanup:
   return 0;
}

inline void vector_lock( VECTOR* v, BOOL lock ) {
   #ifdef USE_THREADS
   #error Locking mechanism undefined!
   #elif defined( DEBUG )
   if( TRUE == lock ) {
      assert( 0 == v->lock_count );
      v->lock_count++;
   } else {
      assert( 1 == v->lock_count );
      v->lock_count--;
   }
   #endif /* USE_THREADS */
}

void* vector_iterate( VECTOR* v, vector_search_cb callback, void* arg ) {
   void* cb_return = NULL;
   void* current_iter = NULL;
   size_t i;

   scaffold_check_null( v );
   assert( VECTOR_SENTINAL == v->sentinal );
   /* TODO: This can work for scalars too, can't it? */
   assert( FALSE == v->scalar );

   vector_lock( v, TRUE );
   for( i = 0 ; vector_count( v ) > i ; i++ ) {
      current_iter = vector_get( v, i );
      cb_return = callback( v, i, current_iter, arg );
      if( NULL != cb_return ) {
         break;
      }
   }
   vector_lock( v, FALSE );

cleanup:
   return cb_return;
}
