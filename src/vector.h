#ifndef VECTOR_H
#define VECTOR_H

#include "scaffold.h"

typedef struct vector_ {
   void** data;
   size_t size;
   size_t count;
   BOOL scalar;
   int32_t* scalar_data;
#if defined( DEBUG ) && !defined( USE_THREADS )
   int lock_count;
#endif /* DEBUG, USE_THREADS */
} VECTOR;

typedef void* (*vector_callback)( VECTOR* v, size_t idx, void* iter, void* arg );

void vector_init( VECTOR* v );
void vector_free( VECTOR* v );
void vector_add( VECTOR* v, void* data );
void vector_add_scalar( VECTOR* v, int32_t value );
void vector_set( VECTOR* v, size_t index, void* data );
void* vector_get( VECTOR* v, size_t index );
int32_t vector_get_scalar( VECTOR* v, size_t value );
size_t vector_delete_cb( VECTOR* v, vector_callback callback, void* arg, BOOL free );
void vector_delete( VECTOR* v, size_t index );
size_t vector_delete_scalar( VECTOR* v, int32_t value );
inline size_t vector_count( VECTOR* v );
inline void vector_lock( VECTOR* v, BOOL lock );
void* vector_iterate( VECTOR* v, vector_callback, void* arg );

#endif /* VECTOR_H */
