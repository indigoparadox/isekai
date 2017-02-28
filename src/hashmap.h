/* * Generic hashmap manipulation functions * * Originally by Elliot C Back - http://elliottback.com/wp/hashmap-implementation-in-c/ * * Modified by Pete Warden to fix a serious performance problem, support strings as keys * and removed thread synchronization - http://petewarden.typepad.com */#ifndef __HASHMAP_H__#define __HASHMAP_H__#include "bstrlib/bstrlib.h"#include "scaffold.h"/* We need to keep keys and values */typedef struct _HASHMAP_ELEMENT {   bstring key;   int in_use;   void* data;} HASHMAP_ELEMENT;/* A hashmap has some maximum size and current size, * as well as the data to hold. */typedef struct _HASHMAP {   int table_size;   int size;   HASHMAP_ELEMENT* data;} HASHMAP;#define MAP_MISSING -3  /* No such element */#define MAP_FULL -2  /* Hashmap is full */#define MAP_OMEM -1  /* Out of Memory */#define MAP_OK 0  /* OK */#if 0/* * any_t is a pointer.  This allows you to put arbitrary structures in * the hashmap. */typedef void* any_t;/* * PFany is a pointer to a function that can take two any_t arguments * and return an integer. Returns status code.. */typedef int (*PFany)(any_t, any_t);/* * HASHMAP* is a pointer to an internally maintained data structure. * Clients of this package do not need to know how hashmaps are * represented.  They see and manipulate only map_t's. */typedef any_t map_t;#endif/* * Return an empty hashmap. Returns NULL if empty.*/HASHMAP* hashmap_new();//void hashmap_init( HASHMAP* m );/* * Iteratively call f with argument (item, data) for * each element data in the hashmap. The function must * return a map status code. If it returns anything other * than MAP_OK the traversal is terminated. f must * not reenter any hashmap functions, or deadlock may arise. *///extern int hashmap_iterate(HASHMAP* in, PFany f, any_t item);/* * Add an element to the hashmap. Return MAP_OK or MAP_OMEM. */void hashmap_put( HASHMAP* m, bstring key, void* value );/* * Get an element from the hashmap. Return MAP_OK or MAP_MISSING. */void* hashmap_get( HASHMAP* in, bstring key );/* * Remove an element from the hashmap. Return MAP_OK or MAP_MISSING. */void hashmap_remove( HASHMAP* in, bstring key );/* * Get any element. Return MAP_OK or MAP_MISSING. * remove - should the element be removed from the hashmap */void* hashmap_pop( HASHMAP* in, BOOL do_remove );/* * Free the hashmap */void hashmap_cleanup( HASHMAP* in );/* * Get the current size of a hashmap */int hashmap_count( HASHMAP* in);int hashmap_active_count( HASHMAP* in );#endif /* __HASHMAP_H__ */