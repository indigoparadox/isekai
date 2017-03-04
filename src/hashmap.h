/*
 * Generic hashmap manipulation functions
 *
 * Originally by Elliot C Back - http://elliottback.com/wp/hashmap-implementation-in-c/
 *
 * Modified by Pete Warden to fix a serious performance problem, support strings as keys
 * and removed thread synchronization - http://petewarden.typepad.com
 */
#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include "bstrlib/bstrlib.h"
#include "scaffold.h"
#include "vector.h"

typedef void* (*hashmap_search_cb)( const bstring key, void* iter, void* arg );
typedef BOOL (*hashmap_delete_cb)( const bstring key, void* iter, void* arg );

/* We need to keep keys and values */
typedef struct _HASHMAP_ELEMENT {
   uint16_t sentinal;
   bstring key;
   int in_use;
   void* data;
} HASHMAP_ELEMENT;

/* A hashmap has some maximum size and current size,
 * as well as the data to hold. */
typedef struct _HASHMAP {
   uint16_t sentinal;
   int table_size;
   int size;
   HASHMAP_ELEMENT* data;
   uint8_t lock_count;
} HASHMAP;

#define HASHMAP_SENTINAL 12345
#define HASHMAP_FULL -2  /* Hashmap is full */

#define hashmap_ready( m ) \
   (HASHMAP_SENTINAL == (m)->sentinal)

#if 0
#define MAP_MISSING -3  /* No such element */
#define MAP_OMEM -1  /* Out of Memory */
#define MAP_OK 0  /* OK */
/*
 * any_t is a pointer.  This allows you to put arbitrary structures in
 * the hashmap.
 */
typedef void* any_t;

/*
 * PFany is a pointer to a function that can take two any_t arguments
 * and return an integer. Returns status code..
 */
typedef int (*PFany)(any_t, any_t);

/*
 * HASHMAP* is a pointer to an internally maintained data structure.
 * Clients of this package do not need to know how hashmaps are
 * represented.  They see and manipulate only map_t's.
 */
typedef any_t map_t;
#endif
/*
 * Return an empty hashmap. Returns NULL if empty.
*/
void hashmap_init( HASHMAP* m );

/*
 * Iteratively call f with argument (item, data) for
 * each element data in the hashmap. The function must
 * return a map status code. If it returns anything other
 * than MAP_OK the traversal is terminated. f must
 * not reenter any hashmap functions, or deadlock may arise.
 */
void* hashmap_iterate( HASHMAP* m, hashmap_search_cb callback, void* arg );

VECTOR* hashmap_iterate_v( HASHMAP* m, hashmap_search_cb callback, void* arg );

size_t hashmap_remove_cb( HASHMAP* m, hashmap_delete_cb callback, void* arg );

/*
 * Add an element to the hashmap. Return MAP_OK or MAP_OMEM.
 */
void hashmap_put( HASHMAP* m, bstring key, void* value );

/*
 * Get an element from the hashmap. Return MAP_OK or MAP_MISSING.
 */
void* hashmap_get( HASHMAP* m, bstring key );

void* hashmap_get_first( HASHMAP* m );

/*
 * Remove an element from the hashmap. Return MAP_OK or MAP_MISSING.
 */
BOOL hashmap_remove( HASHMAP* m, bstring key );

/*
 * Get any element. Return MAP_OK or MAP_MISSING.
 * remove - should the element be removed from the hashmap
 */
void* hashmap_pop( HASHMAP* m, BOOL do_remove );

/*
 * Free the hashmap
 */
void hashmap_cleanup( HASHMAP* m );

/*
 * Get the current size of a hashmap
 */
int hashmap_count( HASHMAP* m);
int hashmap_active_count( HASHMAP* m );

void hashmap_lock( HASHMAP* m, BOOL lock );

#endif /* __HASHMAP_H__ */
