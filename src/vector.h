
#ifndef VECTOR_H
#define VECTOR_H

#include "scaffold.h"

typedef struct vector_ {
    void** data;
    int size;
    int count;
} VECTOR;

#define vector_count( v ) v->count
#define vector_free( v ) free( v->data )

void vector_init( VECTOR* v );
void vector_add( VECTOR* v, void* data );
void vector_set( VECTOR* v, int index, void* data );
void vector_set( VECTOR* v, int index, void* data );
void* vector_get( VECTOR* v, int index );
void vector_delete( VECTOR* v, int index );

#endif /* VECTOR_H */
