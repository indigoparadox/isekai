
#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "vector.h"

void* callback_search_clients( VECTOR* v, size_t idx, void* iter, void* arg );
void* callback_search_channels( VECTOR* v, size_t idx, void* iter, void* arg );
BOOL callback_free_clients( VECTOR* v, size_t idx, void* iter, void* arg );
BOOL callback_free_channels( VECTOR* v, size_t idx, void* iter, void* arg );

#endif /* CALLBACKS_H */
