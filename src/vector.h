#ifndef VECTOR_H
#define VECTOR_H

#include "scaffold.h"

typedef struct vector_ {
   void** data;
   int size;
   int count;
} VECTOR;

void vector_init( VECTOR* v );
void vector_free( VECTOR* v );
void vector_add( VECTOR* v, void* data );
void vector_set( VECTOR* v, int index, void* data );
void* vector_get( VECTOR* v, int index );
void vector_delete( VECTOR* v, int index );
int vector_count( VECTOR* v );

#endif /* VECTOR_H */
