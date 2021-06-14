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

typedef void* (*hashmap_search_cb)( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
typedef BOOL (*hashmap_delete_cb)( struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );

/* We need to keep keys and values */
struct HASHMAP_ELEMENT {
   uint16_t sentinal;
   bstring key;
   BOOL in_use;
   void* data;
#ifdef USE_ITERATOR_CACHE
   SCAFFOLD_SIZE iterator_index;
#endif /* USE_ITERATOR_CACHE */
};

/* A hashmap has some maximum size and current size,
 * as well as the data to hold. */
struct HASHMAP {
   uint16_t sentinal;
   SCAFFOLD_SIZE_SIGNED table_size;
   SCAFFOLD_SIZE_SIGNED size; /* Hashmap sizes can also be - error codes. */
   struct HASHMAP_ELEMENT* data;
   uint8_t lock_count;
   SCAFFOLD_ERROR last_error;
#ifdef USE_ITERATOR_CACHE
   struct VECTOR iterators; /*!< List of hashes stored sequentially.  */
#endif /* USE_ITERATOR_CACHE */
};

#define HASHMAP_FULL -2  /* Hashmap is full */

#define hashmap_ready( m ) \
   (HASHMAP_SENTINAL == (m)->sentinal)


#define hashmap_new( m ) \
   m = mem_alloc( 1, struct HASHMAP ); \
   scaffold_check_null( m ); \
   hashmap_init( m );

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
void hashmap_init( struct HASHMAP* m );

/*
 * Iteratively call f with argument (item, data) for
 * each element data in the hashmap. The function must
 * return a map status code. If it returns anything other
 * than MAP_OK the traversal is terminated. f must
 * not reenter any hashmap functions, or deadlock may arise.
 */
void* hashmap_iterate( struct HASHMAP* m, hashmap_search_cb callback, void* arg );

void* hashmap_iterate_nolock(
   struct HASHMAP* m, hashmap_search_cb callback, void* arg
);

SCAFFOLD_SIZE hashmap_remove_cb( struct HASHMAP* m, hashmap_delete_cb callback, void* arg );

/*
 * Add an element to the hashmap. Return MAP_OK or MAP_OMEM.
 */
short hashmap_put(
   struct HASHMAP* m, const bstring key, void* value, BOOL overwrite )
#ifdef USE_GNUC_EXTENSIONS
__attribute__ ((warn_unused_result))
#endif /* USE_GNUC_EXTENSIONS */
;
short hashmap_put_nolock(
   struct HASHMAP* m, const bstring key, void* value, BOOL overwrite )
#ifdef USE_GNUC_EXTENSIONS
__attribute__ ((warn_unused_result))
#endif /* USE_GNUC_EXTENSIONS */
;

/*
 * Get an element from the hashmap. Return MAP_OK or MAP_MISSING.
 */
void* hashmap_get( struct HASHMAP* m, const bstring key );
void* hashmap_get_c( struct HASHMAP* m, const char* key_c );
void* hashmap_get_nolock( const struct HASHMAP* m, const bstring key );
void* hashmap_get_first( struct HASHMAP* m );

/* bstring hashmap_next_key( struct HASHMAP* m, const bstring key ); */
BOOL hashmap_contains_key( struct HASHMAP* m, const bstring key );
BOOL hashmap_contains_key_nolock( struct HASHMAP* m, const bstring key );

/*
 * Remove an element from the hashmap. Return MAP_OK or MAP_MISSING.
 */
BOOL hashmap_remove( struct HASHMAP* m, const bstring key );
SCAFFOLD_SIZE hashmap_remove_all( struct HASHMAP* m );

/*
 * Get any element. Return MAP_OK or MAP_MISSING.
 * remove - should the element be removed from the hashmap
 */
void* hashmap_pop( struct HASHMAP* m, BOOL do_remove );

/*
 * Free the hashmap
 */
void hashmap_cleanup( struct HASHMAP* m );
void hashmap_free( struct HASHMAP** m );

/*
 * Get the current size of a hashmap
 */
SCAFFOLD_SIZE_SIGNED hashmap_count( const struct HASHMAP* m);
/* int hashmap_active_count( struct HASHMAP* m ); */

void hashmap_lock( struct HASHMAP* m, BOOL lock );

BOOL hashmap_is_valid( const struct HASHMAP* m);

#ifdef HASHMAP_C
#define HASHMAP_SENTINAL 12345
SCAFFOLD_MODULE( "hashmap.c" );
#endif /* HASHMAP_C */

#endif /* __HASHMAP_H__ */
