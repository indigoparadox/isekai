#ifndef VECTOR_H
#define VECTOR_H

#include "scaffold.h"

typedef enum _VECTOR_SORT_ORDER {
   VECTOR_SORT_A_LIGHTER = -1,
   VECTOR_SORT_A_B_EQUAL = 0,
   VECTOR_SORT_A_HEAVIER = 1,
} VECTOR_SORT_ORDER;

typedef struct _VECTOR {
   uint16_t sentinal;
   void** data;
   size_t size;
   size_t count;
   BOOL scalar;
   int32_t* scalar_data;
#if defined( DEBUG ) && !defined( USE_THREADS )
   int lock_count;
#endif /* DEBUG, USE_THREADS */
} VECTOR;

#define VECTOR_SENTINAL 12121

typedef void* (*vector_search_cb)( const bstring res, void* iter, void* arg );
typedef BOOL (*vector_delete_cb)( const bstring res, void* iter, void* arg );
typedef VECTOR_SORT_ORDER (*vector_sorter_cb)( void* a, void* b );

#define vector_ready( v ) \
   (VECTOR_SENTINAL == (v)->sentinal)

#define vector_new( v ) \
   v = (VECTOR*)calloc( 1, sizeof( VECTOR ) ); \
   scaffold_check_null( v ); \
   vector_init( v );

void vector_init( VECTOR* v );
void vector_free( VECTOR* v );
void vector_add( VECTOR* v, void* data );
void vector_add_scalar( VECTOR* v, int32_t value, BOOL allow_dupe );
void vector_set( VECTOR* v, size_t index, void* data );
void vector_set_scalar( VECTOR* v, size_t index, int32_t value );
void* vector_get( VECTOR* v, size_t index );
int32_t vector_get_scalar( VECTOR* v, size_t value );
size_t vector_remove_cb( VECTOR* v, vector_delete_cb callback, void* arg );
void vector_remove( VECTOR* v, size_t index );
size_t vector_remove_scalar( VECTOR* v, int32_t value );
inline size_t vector_count( VECTOR* v );
inline void vector_lock( VECTOR* v, BOOL lock );
void* vector_iterate( VECTOR* v, vector_search_cb callback, void* arg );
void vector_sort_cb( VECTOR* v, vector_sorter_cb );

#endif /* VECTOR_H */
