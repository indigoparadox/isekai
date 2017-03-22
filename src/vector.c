#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VECTOR_C
#include "vector.h"

#define ENABLE_REF_TEST
#include "ref.h"

void vector_init( struct VECTOR* v ) {
   v->data = NULL;
   v->size = 0;
   v->count = 0;
   v->scalar_data = NULL;
   v->sentinal = VECTOR_SENTINAL;
}

void vector_cleanup( struct VECTOR* v ) {
   scaffold_check_null( v );
   scaffold_assert( VECTOR_SENTINAL == v->sentinal );
   scaffold_assert( 0 >= vector_count( v ) );

   if( FALSE != v->scalar ) {
      scaffold_free( v->scalar_data );
   } else {
      scaffold_free( v->data );
   }

cleanup:
   return;
}

SCAFFOLD_INLINE
static void vector_reset( struct VECTOR* v ) {
   scaffold_free( v->scalar_data );
   scaffold_free( v->data );
   v->scalar_data = NULL;
   v->scalar = FALSE;
   v->count = 0;
}

SCAFFOLD_INLINE
static void vector_grow( struct VECTOR* v, SCAFFOLD_SIZE new_size ) {
   SCAFFOLD_SIZE old_size = v->size,
      i;
   void* new_data = NULL;

   v->size = new_size;
   new_data = scaffold_realloc( v->data, v->size, void* );
   scaffold_check_null( new_data );
   v->data = new_data;
   for( i = old_size ; i < v->size ; i++ ) {
      v->data[i] = NULL;
   }
cleanup:
   return;
}

SCAFFOLD_INLINE
static void vector_grow_scalar( struct VECTOR* v, SCAFFOLD_SIZE new_size ) {
   SCAFFOLD_SIZE old_size = v->size,
      i;
   int32_t* new_data;

   v->size = new_size;
   new_data = scaffold_realloc( v->scalar_data, v->size, int32_t );
   scaffold_check_null( new_data );
   v->scalar_data = new_data;

   scaffold_check_null( v->scalar_data );
   for( i = old_size ; i < v->size ; i++ ) {
      v->scalar_data[i] = 0;
   }
cleanup:
   return;
}

/** \brief Insert an item into the vector at the specified position and push
 *         subsequent items later.
 * \param[in] v      Vector to manipulate.
 * \param[in] index  Index to push the item at. Vector will be extended to
 *                   include this index if it does not already.
 * \param[in] data   Pointer to the item to add.
 */
void vector_insert( struct VECTOR* v, SCAFFOLD_SIZE index, void* data ) {
   BOOL ok = FALSE;
   SCAFFOLD_SIZE i;

   scaffold_check_null( v );
   scaffold_assert( VECTOR_SENTINAL == v->sentinal );

   scaffold_assert( FALSE == v->scalar );

   vector_lock( v, TRUE );
   ok = TRUE;

   if( 0 == v->size ) {
      v->size = 10;
      v->data = (void**)calloc( v->size, sizeof(void*) );
      scaffold_check_null( v->data );
   }

   while( v->size <= index || v->count == v->size - 1 ) {
      vector_grow( v, v->size * 2 );
      scaffold_check_nonzero( scaffold_error );
   }

   for( i = index ; v->size - 1 > i ; i++ ) {
      v->data[i + 1] = v->data[i];
   }
   v->data[index] = data;
   v->count++;
   refcount_test_inc( data );

cleanup:
   if( FALSE != ok ) {
      vector_lock( v, FALSE );
   }
   scaffold_assert( 0 == v->lock_count );

   return;
}

void vector_add( struct VECTOR* v, void* data ) {
   BOOL ok = FALSE;

   scaffold_check_null( v );
   scaffold_assert( VECTOR_SENTINAL == v->sentinal );

   scaffold_assert( FALSE == v->scalar );

   vector_lock( v, TRUE );
   ok = TRUE;

   if( 0 == v->size ) {
      v->size = 10;
      v->data = (void**)calloc( v->size, sizeof(void*) );
      scaffold_check_null( v->data );
   }

   if( v->size == v->count ) {
      vector_grow( v, v->size * 2 );
      scaffold_check_nonzero( scaffold_error );
   }

   v->data[v->count] = data;
   v->count++;
   refcount_test_inc( data );

cleanup:
   if( FALSE != ok ) {
      vector_lock( v, FALSE );
   }
   scaffold_assert( 0 == v->lock_count );

   return;
}

void vector_add_scalar( struct VECTOR* v, int32_t value, BOOL allow_dupe ) {
   SCAFFOLD_SIZE i;
   BOOL ok = FALSE;

   scaffold_check_null( v );
   scaffold_assert( VECTOR_SENTINAL == v->sentinal );

   vector_lock( v, TRUE );
   ok = TRUE;

   /* Die if we have non-scalar data already. */
   scaffold_assert( NULL == v->data );

   v->scalar = TRUE;

   if( 0 == v->size || NULL == v->scalar_data ) {
      v->size = 10;
      v->scalar_data = (int32_t*)calloc( v->size, sizeof( int32_t ) );
      scaffold_check_null( v->scalar_data );
   }

   if( FALSE == allow_dupe ) {
      for( i = 0 ; NULL != v->scalar_data && v->count > i ; i++ ) {
         if( v->scalar_data[i] == value ) {
            scaffold_error = SCAFFOLD_ERROR_DUPLICATE;
            scaffold_print_error(
               &module,
               "Warning: Attempted to add duplicate %d to scalar vector.\n",
               value
            );
            goto cleanup;
         }
      }
   }

   if( v->size == v->count ) {
      vector_grow_scalar( v, v->size * 2 );
      scaffold_check_nonzero( scaffold_error );
   }

   v->scalar_data[v->count] = value;
   v->count++;

cleanup:
   if( TRUE == ok ) {
      vector_lock( v, FALSE );
   }
   scaffold_assert( 0 == v->lock_count );
   return;
}

void vector_set( struct VECTOR* v, SCAFFOLD_SIZE index, void* data, BOOL force ) {
   SCAFFOLD_SIZE new_size = v->size;

   scaffold_check_null( v );

   scaffold_assert( VECTOR_SENTINAL == v->sentinal );
   scaffold_assert( FALSE == v->scalar );

   vector_lock( v, TRUE );

   if( 0 == v->size ) {
      v->size = 10;
      v->data = (void**)calloc( v->size, sizeof(void*) );
      scaffold_check_null( v->data );
   }

   if( FALSE == force ) {
      scaffold_check_bounds( index, v->count );
   } else if( index + 1 > v->size ) {
      while( v->size < index + 1 ) {
         new_size = v->size * 2;
         vector_grow( v, new_size );
         scaffold_check_nonzero( scaffold_error );
      }
   }

   if( NULL != v->data[index] ) {
      refcount_test_dec( v->data[index] );
   }
   v->data[index] = data;
   refcount_test_inc( data );


   /* TODO: Is this the right thing to do? */
   if( v->count <= index ) {
      v->count = index + 1;
   }

cleanup:
   vector_lock( v, FALSE );
   scaffold_assert( 0 == v->lock_count );
   return;
}

void vector_set_scalar( struct VECTOR* v, SCAFFOLD_SIZE index, int32_t value ) {
   BOOL ok = FALSE;

   scaffold_check_null( v );
   scaffold_assert( VECTOR_SENTINAL == v->sentinal );

   vector_lock( v, TRUE );
   ok = TRUE;

   /* Die if we have non-scalar data already. */
   scaffold_assert( NULL == v->data );

   v->scalar = TRUE;

   if( 0 == v->size ) {
      v->size = index + 1;
      v->scalar_data = (int32_t*)calloc( v->size, sizeof( int32_t ) );
      scaffold_check_null( v->scalar_data );
   }

   if( index >= v->size ) {
      vector_grow_scalar( v, index + 1 );
      scaffold_check_nonzero( scaffold_error );
   }

   v->scalar_data[index] = value;

cleanup:
   if( TRUE == ok ) {
      vector_lock( v, FALSE );
   }
   scaffold_assert( 0 == v->lock_count );
   return;
}

void* vector_get( struct VECTOR* v, SCAFFOLD_SIZE index ) {
   void* retptr = NULL;

   if( NULL == v ) {
      goto cleanup; /* Quietly. */
   }
   scaffold_assert( VECTOR_SENTINAL == v->sentinal );
   scaffold_assert( FALSE == v->scalar );

   if( v->count <= index ) {
      goto cleanup;
   }

   retptr = v->data[index];

cleanup:

   return retptr;
}

int32_t vector_get_scalar( struct VECTOR* v, SCAFFOLD_SIZE index ) {
   int32_t retval = -1;

   scaffold_check_null( v );
   scaffold_assert( VECTOR_SENTINAL == v->sentinal );
   scaffold_assert( TRUE == v->scalar );

   scaffold_check_bounds( index, v->count );

   retval = v->scalar_data[index];

cleanup:

   return retval;
}

int32_t vector_get_scalar_value( struct VECTOR* v, SCAFFOLD_SIZE value ) {
   int32_t retval = -1;
   int i;

   scaffold_check_null( v );
   scaffold_assert( VECTOR_SENTINAL == v->sentinal );
   scaffold_assert( TRUE == v->scalar );

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
SCAFFOLD_SIZE vector_remove_cb( struct VECTOR* v, vector_delete_cb callback, void* arg ) {
   SCAFFOLD_SIZE i;
   SCAFFOLD_SIZE backshift = 0;

   /* FIXME: Delete dynamic arrays and reset when empty. */

   scaffold_check_null( v );
   scaffold_assert( VECTOR_SENTINAL == v->sentinal );
   scaffold_assert( FALSE == v->scalar );

   vector_lock( v, TRUE );

   for( i = 0 ; v->count > i ; i++ ) {

      /* The delete callback should call the object-specific free() function, *
       * which decreases its refcount naturally. So there's no need to do it  *
       * manually here.                                                       */
      if( FALSE != callback( NULL, v->data[i], arg ) ) {
         backshift++;
      }

      if( v->count - backshift > i && 0 < backshift ) {
         /* The callback found a match, so start deleting! */
         v->data[i] = v->data[i + backshift];
      }
   }

   v->count -= backshift;

   vector_lock( v, FALSE );

cleanup:
   scaffold_assert( 0 == v->lock_count );
   return backshift;
}

void vector_remove( struct VECTOR* v, SCAFFOLD_SIZE index ) {
   SCAFFOLD_SIZE i;
   BOOL ok = FALSE;

   /* FIXME: Delete dynamic arrays and reset when empty. */

   scaffold_check_null( v );
   scaffold_assert( VECTOR_SENTINAL == v->sentinal );
   scaffold_assert( FALSE == v->scalar );

   vector_lock( v, TRUE );
   ok = TRUE;

   scaffold_assert( v->count > index );
   scaffold_check_bounds( index, v->count );

   refcount_test_dec( v->data[index] );

   for( i = index; v->count - 1 > i ; i++ ) {
      v->data[i] = v->data[i + 1];
   }

   v->count--;

cleanup:
   if( FALSE != ok ) {
      vector_lock( v, FALSE );
   }
   scaffold_assert( 0 == v->lock_count );
   return;
}

void vector_remove_scalar( struct VECTOR* v, SCAFFOLD_SIZE index ) {
   SCAFFOLD_SIZE i;

   scaffold_check_null( v );
   scaffold_assert( VECTOR_SENTINAL == v->sentinal );
   scaffold_assert( TRUE == v->scalar );

   vector_lock( v, TRUE );

   if( 1 >= v->count ) {
      /* Delete dynamic arrays and reset if now empty. */
      vector_reset( v );
   } else {
      for( i = index; v->count - 1 > i ; i++ ) {
         v->scalar_data[i] = v->scalar_data[i + 1];
      }
      v->count -= 1;
   }

   vector_lock( v, FALSE );

cleanup:
   scaffold_assert( 0 == v->lock_count );
   return;
}

SCAFFOLD_SIZE vector_remove_scalar_value( struct VECTOR* v, int32_t value ) {
   SCAFFOLD_SIZE i;
   SCAFFOLD_SIZE difference = 0;


   scaffold_check_null( v );
   scaffold_assert( VECTOR_SENTINAL == v->sentinal );
   scaffold_assert( TRUE == v->scalar );

   vector_lock( v, TRUE );

   if( 1 >= v->count ) {
      /* Delete dynamic arrays and reset if now empty. */
      vector_reset( v );
   } else {
      for( i = 0; v->count - difference > i ; i++ ) {
         if( v->scalar_data[i] == value ) {
            difference++;
         }
         v->scalar_data[i] = v->scalar_data[i + difference];
      }
      v->count -= difference;
   }

   vector_lock( v, FALSE );

cleanup:
   scaffold_assert( 0 == v->lock_count );
   return difference;
}

SCAFFOLD_SIZE vector_count( struct VECTOR* v ) {
   scaffold_check_null( v );
   if( VECTOR_SENTINAL != v->sentinal ) {
      scaffold_error = SCAFFOLD_ERROR_OUTOFBOUNDS;
      goto cleanup;
   }
   return v->count;
cleanup:
   return 0;
}

void vector_lock( struct VECTOR* v, BOOL lock ) {
   #ifdef USE_THREADS
   #error Locking mechanism undefined!
   #elif defined( DEBUG )
   if( TRUE == lock ) {
      scaffold_assert( 0 == v->lock_count );
      v->lock_count++;
   } else {
      scaffold_assert( 1 == v->lock_count );
      v->lock_count--;
   }
   #endif /* USE_THREADS */
}

/** \brief Iterate through the given vector with the given callback.
 * \param[in] v         The vector through which to iterate.
 * \param[in] callback  The callback to run on each item.
 * \param[in] arg       The argument to pass the callback.
 * \return If one of the callbacks returns an item, the iteration loop will
 *         break and return this item. Otherwise, NULL will be returned.
 */
void* vector_iterate( struct VECTOR* v, vector_search_cb callback, void* arg ) {
   void* cb_return = NULL;
   void* current_iter = NULL;
   SCAFFOLD_SIZE i;

   scaffold_check_null( v );
   scaffold_assert( VECTOR_SENTINAL == v->sentinal );
   /* TODO: This can work for scalars too, can't it? */
   scaffold_assert( FALSE == v->scalar );

   vector_lock( v, TRUE );
   for( i = 0 ; vector_count( v ) > i ; i++ ) {
      current_iter = vector_get( v, i );
      cb_return = callback( NULL, current_iter, arg );
      if( NULL != cb_return ) {
         break;
      }
   }
   vector_lock( v, FALSE );

cleanup:
   return cb_return;
}

void* vector_iterate_nolock( struct VECTOR* v, vector_search_cb callback, void* arg ) {
   void* cb_return = NULL;
   void* current_iter = NULL;
   SCAFFOLD_SIZE i;

   scaffold_check_null( v );
   scaffold_assert( VECTOR_SENTINAL == v->sentinal );
   /* TODO: This can work for scalars too, can't it? */
   scaffold_assert( FALSE == v->scalar );

   for( i = 0 ; vector_count( v ) > i ; i++ ) {
      current_iter = vector_get( v, i );
      cb_return = callback( NULL, current_iter, arg );
      if( NULL != cb_return ) {
         break;
      }
   }

cleanup:
   return cb_return;
}

/** \brief Iterate through the given vector with the given callback in reverse.
 * \param[in] v         The vector through which to iterate.
 * \param[in] callback  The callback to run on each item.
 * \param[in] arg       The argument to pass the callback.
 * \return If one of the callbacks returns an item, the iteration loop will
 *         break and return this item. Otherwise, NULL will be returned.
 */
void* vector_iterate_r( struct VECTOR* v, vector_search_cb callback, void* arg ) {
   void* cb_return = NULL;
   void* current_iter = NULL;
   SCAFFOLD_SIZE_SIGNED i;

   scaffold_check_null( v );
   scaffold_assert( VECTOR_SENTINAL == v->sentinal );
   /* TODO: This can work for scalars too, can't it? */
   scaffold_assert( FALSE == v->scalar );

   vector_lock( v, TRUE );
   for( i = vector_count( v ) - 1 ; 0 <= i ; i-- ) {
      current_iter = vector_get( v, i );
      cb_return = callback( NULL, current_iter, arg );
      if( NULL != cb_return ) {
         break;
      }
   }
   vector_lock( v, FALSE );

cleanup:
   return cb_return;
}

struct VECTOR* vector_iterate_v(
   struct VECTOR* v, vector_search_cb callback, void* arg
) {
   struct VECTOR* found = NULL;
   void* current_iter = NULL;
   void* cb_return = NULL;
   BOOL ok = FALSE;
   int i;

   scaffold_check_null( v );
   scaffold_assert( VECTOR_SENTINAL == v->sentinal );

   vector_lock( v, TRUE );
   ok = TRUE;

   /* Linear probing */
   for( i = 0 ; vector_count( v ) > i ; i++ ) {
      current_iter = vector_get( v, i );
      cb_return = callback( NULL, current_iter, arg );
      if( NULL != cb_return ) {
         if( NULL == found ) {
            vector_new( found );
         }
         vector_add( found, cb_return );
      }
   }

cleanup:
   if( TRUE == ok ) {
      vector_lock( v, FALSE );
   }
   return found;
}

void vector_sort_cb( struct VECTOR* v, vector_sorter_cb callback ) {
   void* previous_iter = NULL;
   void* current_iter = NULL;
   SCAFFOLD_SIZE i;
   VECTOR_SORT_ORDER o;
   BOOL ok = FALSE;

   scaffold_check_null( v );
   scaffold_assert( VECTOR_SENTINAL == v->sentinal );

   if( 2 > vector_count( v ) ) {
      /* Not enough to sort! */
      goto cleanup;
   }

   /* TODO: This can work for scalars too, can't it? */
   scaffold_assert( FALSE == v->scalar );

   vector_lock( v, TRUE );
   ok = TRUE;

   for( i = 1 ; vector_count( v ) > i ; i++ ) {
      current_iter = vector_get( v, i );
      previous_iter = vector_get( v, (i - 1) );
      o = callback( previous_iter, current_iter );
      switch( o ) {
      case VECTOR_SORT_A_B_EQUAL:
         break;

      case VECTOR_SORT_A_LIGHTER:
         break;

      case VECTOR_SORT_A_HEAVIER:
         v->data[i] = previous_iter;
         v->data[i - 1] = current_iter;
         break;
      }
   }

cleanup:
   if( FALSE != ok ) {
      vector_lock( v, FALSE );
   }
   scaffold_assert( 0 == v->lock_count );
   return;
}
