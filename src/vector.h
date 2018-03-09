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

typedef enum VECTOR_ERR {
   VECTOR_ERR_NONE = 0,
   VECTOR_ERR_FULL = 1
} VECTOR_ERR;

#define VECTOR_SENTINAL 12121

typedef void* (*vector_search_cb)( struct CONTAINER_IDX* idx, void* iter, void* arg );
typedef BOOL (*vector_delete_cb)( struct CONTAINER_IDX* idx, void* iter, void* arg );
typedef VECTOR_SORT_ORDER (*vector_sorter_cb)( void* a, void* b );

#define vector_ready( v ) \
   (VECTOR_SENTINAL == (v)->sentinal)

#define vector_new( v ) \
   v = mem_alloc( 1, struct VECTOR ); \
   scaffold_check_null( v ); \
   vector_init( v );

void vector_init( struct VECTOR* v );
void vector_cleanup( struct VECTOR* v );
void vector_free( struct VECTOR** v );
VECTOR_ERR vector_insert( struct VECTOR* v, SCAFFOLD_SIZE index, void* data )
#ifdef USE_GNUC_EXTENSIONS
__attribute__ ((warn_unused_result))
#endif /* USE_GNUC_EXTENSIONS */
;
VECTOR_ERR vector_add( struct VECTOR* v, void* data )
#ifdef USE_GNUC_EXTENSIONS
__attribute__ ((warn_unused_result))
#endif /* USE_GNUC_EXTENSIONS */
;
void vector_add_scalar( struct VECTOR* v, int32_t value, BOOL allow_dupe );
void vector_set( struct VECTOR* v, SCAFFOLD_SIZE index, void* data, BOOL force );
void vector_set_scalar( struct VECTOR* v, SCAFFOLD_SIZE index, int32_t value );
void* vector_get( const struct VECTOR* v, SCAFFOLD_SIZE index );
int32_t vector_get_scalar( const struct VECTOR* v, SCAFFOLD_SIZE index );
int32_t vector_get_scalar_value( const struct VECTOR* v, int32_t value );
SCAFFOLD_SIZE vector_remove_cb( struct VECTOR* v, vector_delete_cb callback, void* arg );
void vector_remove( struct VECTOR* v, SCAFFOLD_SIZE index );
void vector_remove_scalar( struct VECTOR* v, SCAFFOLD_SIZE index );
SCAFFOLD_SIZE vector_remove_scalar_value( struct VECTOR* v, int32_t value );
SCAFFOLD_SIZE vector_count( struct VECTOR* v );
void vector_lock( struct VECTOR* v, BOOL lock );
void* vector_iterate( struct VECTOR* v, vector_search_cb callback, void* arg );
void* vector_iterate_nolock( struct VECTOR* v, vector_search_cb callback, void* arg );
void* vector_iterate_r( struct VECTOR* v, vector_search_cb callback, void* arg );
struct VECTOR* vector_iterate_v(
   struct VECTOR* v, vector_search_cb callback, void* arg
);
void vector_sort_cb( struct VECTOR* v, vector_sorter_cb );

#ifdef VECTOR_C
SCAFFOLD_MODULE( "vector.c" );
#endif /* VECTOR_C */

#endif /* VECTOR_H */
