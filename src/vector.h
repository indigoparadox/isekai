#ifndef VECTOR_H
#define VECTOR_H

#include "scaffold.h"

typedef enum _VECTOR_SORT_ORDER {
   VECTOR_SORT_A_LIGHTER = -1,
   VECTOR_SORT_A_B_EQUAL = 0,
   VECTOR_SORT_A_HEAVIER = 1
} VECTOR_SORT_ORDER;

struct VECTOR {
   uint16_t sentinal;
   void** data;
   SCAFFOLD_SIZE size;
   SCAFFOLD_SIZE count;
   BOOL scalar;
   int32_t* scalar_data;
#if defined( DEBUG ) && !defined( USE_THREADS )
   int lock_count;
#endif /* DEBUG, USE_THREADS */
};

#define VECTOR_SENTINAL 12121

typedef void* (*vector_search_cb)( const bstring res, void* iter, void* arg );
typedef BOOL (*vector_delete_cb)( const bstring res, void* iter, void* arg );
typedef VECTOR_SORT_ORDER (*vector_sorter_cb)( void* a, void* b );

#define vector_ready( v ) \
   (VECTOR_SENTINAL == (v)->sentinal)

#define vector_new( v ) \
   v = (struct VECTOR*)calloc( 1, sizeof( struct VECTOR ) ); \
   scaffold_check_null( v ); \
   vector_init( v );

void vector_init( struct VECTOR* v );
void vector_cleanup( struct VECTOR* v );
void vector_insert( struct VECTOR* v, SCAFFOLD_SIZE index, void* data );
void vector_add( struct VECTOR* v, void* data );
void vector_add_scalar( struct VECTOR* v, int32_t value, BOOL allow_dupe );
void vector_set( struct VECTOR* v, SCAFFOLD_SIZE index, void* data, BOOL force );
void vector_set_scalar( struct VECTOR* v, SCAFFOLD_SIZE index, int32_t value );
void* vector_get( struct VECTOR* v, SCAFFOLD_SIZE index );
int32_t vector_get_scalar( struct VECTOR* v, SCAFFOLD_SIZE index );
int32_t vector_get_scalar_value( struct VECTOR* v, SCAFFOLD_SIZE value );
SCAFFOLD_SIZE vector_remove_cb( struct VECTOR* v, vector_delete_cb callback, void* arg );
void vector_remove( struct VECTOR* v, SCAFFOLD_SIZE index );
void vector_remove_scalar( struct VECTOR* v, SCAFFOLD_SIZE index );
SCAFFOLD_SIZE vector_remove_scalar_value( struct VECTOR* v, int32_t value );
SCAFFOLD_SIZE vector_count( struct VECTOR* v );
void vector_lock( struct VECTOR* v, BOOL lock );
void* vector_iterate( struct VECTOR* v, vector_search_cb callback, void* arg );
void* vector_iterate_r( struct VECTOR* v, vector_search_cb callback, void* arg );
void vector_sort_cb( struct VECTOR* v, vector_sorter_cb );

#ifdef VECTOR_C
SCAFFOLD_MODULE( "vector.c" );
#endif /* VECTOR_C */

#endif /* VECTOR_H */
