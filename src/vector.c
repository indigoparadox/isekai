
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vector.h"

void vector_init( VECTOR* v ) {
    v->data = NULL;
    v->size = 0;
    v->count = 0;
}

void vector_add( VECTOR* v, void* data ) {
    if( 0 == v->size ) {
        v->size = 10;
        v->data = calloc( v->size, sizeof(void*) );
        scaffold_check_null( v->data );
    }


    if( v->size == v->count ) {
        v->size *= 2;
        v->data = realloc( v->data, sizeof(void*) * v->size );
        scaffold_check_null( v->data );
        /* TODO: Clear new vector elements? */
    }

    v->data[v->count] = data;
    v->count++;

cleanup:

    return;
}

void vector_set( VECTOR* v, int index, void* data ) {
    if( index >= v->count ) {
        goto cleanup;
    }

    v->data[index] = data;

cleanup:

    return;
}

void* vector_get( VECTOR *v, int index ) {
    void* retptr = NULL;

    if( index >= v->count ) {
        goto cleanup;
    }

    retptr = v->data[index];

cleanup:

    return retptr;
}

void vector_delete( VECTOR *v, int index ) {
    int i, j;

    if( index >= v->count ) {
        goto cleanup;
    }

    for( i = index, j = index; i < v->count; i++ ) {
        v->data[j] = v->data[i];
        j++;
    }

    v->count--;

cleanup:

    return;
}
