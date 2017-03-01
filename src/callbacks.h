
#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "vector.h"
#include "hashmap.h"

void* callback_ingest_commands( const bstring key, void* iter, void* arg );
void* callback_search_clients( const bstring key, void* iter, void* arg );
void* callback_search_channels( const bstring key, void* iter, void* arg );
BOOL callback_free_clients( const bstring key, void* iter, void* arg );
BOOL callback_free_channels( const bstring key, void* iter, void* arg );

#endif /* CALLBACKS_H */
