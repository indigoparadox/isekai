/*
 * Generic map implementation.
 */
#include "hashmap.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ref.h"

#define INITIAL_SIZE (256)
#define MAX_CHAIN_LENGTH (8)

/*
 * Return an empty hashmap, or NULL on failure.
 */
void hashmap_init( struct HASHMAP* m ) {
   scaffold_check_null( m );

   m->data = (struct HASHMAP_ELEMENT*)calloc( INITIAL_SIZE, sizeof( struct HASHMAP_ELEMENT ) );
   scaffold_check_null( m->data );

   m->table_size = INITIAL_SIZE;
   m->size = 0;
   m->sentinal = HASHMAP_SENTINAL;
   m->lock_count = 0;
   m->last_error = SCAFFOLD_ERROR_NONE;

cleanup:
   return;
}

/* The implementation here was originally done by Gary S. Brown.  I have
   borrowed the tables directly, and made some minor changes to the
   crc32-function (including changing the interface). //ylo */

/* ============================================================= */
/*  COPYRIGHT (C) 1986 Gary S. Brown.  You may use this program, or       */
/*  code or tables extracted from it, as desired without restriction.     */
/*                                                                        */
/*  First, the polynomial itself and its table of feedback terms.  The    */
/*  polynomial is                                                         */
/*  X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0   */
/*                                                                        */
/*  Note that we take it "backwards" and put the highest-order term in    */
/*  the lowest-order bit.  The X^32 term is "implied"; the LSB is the     */
/*  X^31 term, etc.  The X^0 term (usually shown as "+1") results in      */
/*  the MSB being 1.                                                      */
/*                                                                        */
/*  Note that the usual hardware shift register implementation, which     */
/*  is what we're using (we're merely optimizing it by doing eight-bit    */
/*  chunks at a time) shifts bits into the lowest-order term.  In our     */
/*  implementation, that means shifting towards the right.  Why do we     */
/*  do it this way?  Because the calculated CRC must be transmitted in    */
/*  order from highest-order term to lowest-order term.  UARTs transmit   */
/*  characters in order from LSB to MSB.  By storing the CRC this way,    */
/*  we hand it to the UART in the order low-byte to high-byte; the UART   */
/*  sends each low-bit to hight-bit; and the result is transmission bit   */
/*  by bit from highest- to lowest-order term without requiring any bit   */
/*  shuffling on our part.  Reception works similarly.                    */
/*                                                                        */
/*  The feedback terms table consists of 256, 32-bit entries.  Notes:     */
/*                                                                        */
/*      The table can be generated at runtime if desired; code to do so   */
/*      is shown later.  It might not be obvious, but the feedback        */
/*      terms simply represent the results of eight shift/xor opera-      */
/*      tions for all combinations of data and CRC register values.       */
/*                                                                        */
/*      The values must be right-shifted by eight bits by the "updcrc"    */
/*      logic; the shift must be unsigned (bring in zeroes).  On some     */
/*      hardware you could probably optimize the shift in assembler by    */
/*      using byte-swap instructions.                                     */
/*      polynomial $edb88320                                              */
/*                                                                        */
/*  --------------------------------------------------------------------  */

static unsigned long crc32_tab[] = {
   0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
   0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
   0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
   0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
   0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
   0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
   0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
   0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
   0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
   0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
   0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
   0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
   0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
   0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
   0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
   0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
   0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
   0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
   0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
   0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
   0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
   0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
   0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
   0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
   0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
   0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
   0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
   0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
   0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
   0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
   0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
   0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
   0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
   0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
   0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
   0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
   0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
   0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
   0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
   0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
   0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
   0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
   0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
   0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
   0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
   0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
   0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
   0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
   0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
   0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
   0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
   0x2d02ef8dL
};

/* Return a 32-bit CRC of the contents of the buffer. */

static uint32_t hashmap_crc32( bstring string ) {
   unsigned int i = 0;
   uint32_t crc32val = 0;
   const unsigned char* s = NULL;

   scaffold_check_null( string );
   s = (const unsigned char*)bdata( string );
   scaffold_check_null( s );

   crc32val = 0;
   for (i = 0;  i < blength(string);  i ++) {
      crc32val =
         crc32_tab[(crc32val ^ s[i]) & 0xff] ^
         (crc32val >> 8);
   }

cleanup:
   return crc32val;
}

/*
 * Hashing function for a string
 */
static uint32_t hashmap_hash_int( struct HASHMAP* m, const bstring keystring ) {
   uint32_t key = 0;

   scaffold_check_null( m );
   scaffold_check_null( keystring );
   scaffold_assert( HASHMAP_SENTINAL == m->sentinal );

   key = hashmap_crc32( keystring );

   /* Robert Jenkins' 32 bit Mix Function */
   key += (key << 12);
   key ^= (key >> 22);
   key += (key << 4);
   key ^= (key >> 9);
   key += (key << 10);
   key ^= (key >> 2);
   key += (key << 7);
   key ^= (key >> 12);

   /* Knuth's Multiplicative Method */
   key = (key >> 3) * 2654435761u;
   key %= m->table_size;

cleanup:
   return key;
}

/*
 * Return the integer of the location in data
 * to store the point to the item, or MAP_FULL.
 */
static int hashmap_hash( struct HASHMAP* m, const bstring key ) {
   int curr;
   int i;
   int out = HASHMAP_FULL;

   scaffold_check_null( m );
   scaffold_assert( HASHMAP_SENTINAL == m->sentinal );
   scaffold_assert( NULL != key );

   /* If full, return immediately */
   if( m->size >= (m->table_size / 2) ) {
      goto cleanup;
   }

   /* Find the best index */
   curr = hashmap_hash_int( m, key );

   /* Linear probing */
   for( i = 0 ; i < MAX_CHAIN_LENGTH ; i++ ) {
      if( m->data[curr].in_use == 0) {
         out = curr;
         goto cleanup;
      }

      if( 1 == m->data[curr].in_use && (0 == bstrcmp( m->data[curr].key, key )) ) {
         out = curr;
         goto cleanup;
      }

      curr = (curr + 1) % m->table_size;
   }

cleanup:
   return out;
}

#ifdef DEBUG

static void hashmap_verify_size( struct HASHMAP* m ) {
   SCAFFOLD_SIZE size_check = 0;
   SCAFFOLD_SIZE i;

   for( i = 0 ; i < m->table_size ; i++ ) {
      if( 0 == m->data[i].in_use ) {
         continue;
      }

      size_check++;
   }

   scaffold_assert( m->size == size_check );
}

#endif /* DEBUG */

void hashmap_rehash( struct HASHMAP* m );

/*
 * Add a pointer to the hashmap without touching its refcount.
 */
static void hashmap_put_internal( struct HASHMAP* m, const bstring key, void* value, BOOL lock ) {
   int index;
   BOOL ok = FALSE;
   BOOL pre_existing = FALSE;

   scaffold_check_null( m );
   scaffold_assert( HASHMAP_SENTINAL == m->sentinal );
   ok = TRUE;

   if( FALSE != lock ) {
      hashmap_lock( m, TRUE );
   }

   /* Find a place to put our value */
   index = hashmap_hash( m, key );
   while( HASHMAP_FULL == index ) {
      hashmap_rehash( m );
      scaffold_check_nonzero( scaffold_error );
      index = hashmap_hash( m, key );
   }

   /* Set the data */
   m->data[index].data = value;
   if( NULL != m->data[index].key ) {
      scaffold_assert( 1 == m->data[index].in_use );
      bdestroy( m->data[index].key );
      pre_existing = TRUE;
   }
   m->data[index].key = bstrcpy( key );
   bwriteprotect( (*m->data[index].key) );
   m->data[index].in_use = 1;
   if( TRUE != pre_existing ) {
      m->size++;
   }

cleanup:
   if( FALSE != ok && FALSE != lock ) {
      hashmap_lock( m, FALSE );
   }

   hashmap_verify_size( m );

   return;
}

/*
 * Doubles the size of the hashmap, and rehashes all the elements
 */
void hashmap_rehash( struct HASHMAP* m ) {
   int i;
   int old_size;
   struct HASHMAP_ELEMENT* curr;
   struct HASHMAP_ELEMENT* temp;

   scaffold_check_null( m );
   scaffold_assert( HASHMAP_SENTINAL == m->sentinal );

   /* Setup the new elements */
   temp = (struct HASHMAP_ELEMENT*)
                           calloc(2 * m->table_size, sizeof(struct HASHMAP_ELEMENT));
   scaffold_check_null( temp );

   /* Update the array */
   curr = m->data;
   m->data = temp;

   /* Update the size */
   old_size = m->table_size;
   m->table_size = 2 * m->table_size;
   m->size = 0;

   /* Rehash the elements */
   for( i = 0; i < old_size; i++ ) {
      if( 0 == curr[i].in_use ) {
         continue;
      }

      /* Never lock when calling recursively! */
      hashmap_put_internal( m, curr[i].key, curr[i].data, FALSE );
      scaffold_check_nonzero( scaffold_error );
   }

   free( curr );

cleanup:
   hashmap_verify_size( m );
   return;
}

/*
 * Add a pointer to the hashmap with some key
 */
void hashmap_put( struct HASHMAP* m, const bstring key, void* value ) {
   hashmap_put_internal( m, key, value, TRUE );
   if( NULL != value ) {
      refcount_test_inc( value );
   }
   m->last_error = SCAFFOLD_ERROR_NONE;
}

void hashmap_put_nolock( struct HASHMAP* m, const bstring key, void* value ) {
   hashmap_put_internal( m, key, value, FALSE );
   if( NULL != value ) {
      refcount_test_inc( value );
   }
   m->last_error = SCAFFOLD_ERROR_NONE;
}

/*
 * Get your pointer out of the hashmap with a key
 */
static void* hashmap_get_internal( struct HASHMAP* m, const bstring key, BOOL lock ) {
   int curr;
   int i;
   int in_use;
   void* element_out = NULL;
   BOOL ok = FALSE;

   scaffold_check_null( m );
   scaffold_assert( HASHMAP_SENTINAL == m->sentinal );
   scaffold_check_zero_against( m->last_error, hashmap_count( m ) );

   if( FALSE != lock ) {
      hashmap_lock( m, TRUE );
   }
   ok = TRUE;

   /* Find data location */
   curr = hashmap_hash_int( m, key );

   /* Linear probing, if necessary */
   for( i = 0 ; MAX_CHAIN_LENGTH > i ; i++ ) {
      in_use = m->data[curr].in_use;
      if( 1 == in_use ) {
#ifdef DEBUG_MATCHING
         scaffold_print_debug( "Hashmap: %s vs %s\n", bdata( m->data[curr].key ), bdata( key ) );
#endif /* DEBUG_MATCHING */
         if( 0 == bstrcmp( m->data[curr].key, key ) ) {
            element_out = (m->data[curr].data);
            goto cleanup;
         }
      }

      curr = (curr + 1) % m->table_size;
   }

cleanup:
   if( FALSE != lock && FALSE != ok ) {
      hashmap_lock( m, FALSE );
   }
   return element_out;
}

void* hashmap_get( struct HASHMAP* m, const bstring key ) {
   return hashmap_get_internal( m, key, TRUE );
}

void* hashmap_get_nolock( struct HASHMAP* m, const bstring key ) {
   return hashmap_get_internal( m, key, FALSE );
}

void* hashmap_get_first( struct HASHMAP* m ) {
   SCAFFOLD_SIZE i;
   void* found = NULL;
   void* data = NULL;
   BOOL ok = FALSE;

   scaffold_check_null( m );
   scaffold_assert( HASHMAP_SENTINAL == m->sentinal );
   scaffold_check_zero_against( m->last_error, hashmap_count( m ) );

   hashmap_lock( m, TRUE );
   ok = TRUE;

   /* Linear probing */
   for( i = 0; m->table_size > i ; i++ ) {
      if( 0 != m->data[i].in_use ) {
         data = (void*)(m->data[i].data);
         if( NULL != data ) {
            found = data;
            goto cleanup;
         }
      }
   }

cleanup:
   if( ok ) {
      hashmap_lock( m, FALSE );
   }
   return found;
}

static BOOL hashmap_contains_key_internal( struct HASHMAP* m, const bstring key, BOOL lock ) {
   int curr;
   int i;
   int in_use;
   BOOL ok = FALSE;

   scaffold_check_null( m );
   scaffold_assert( HASHMAP_SENTINAL == m->sentinal );
   scaffold_check_zero_against( m->last_error, hashmap_count( m ) );

   if( FALSE != lock ) {
      hashmap_lock( m, TRUE );
   }
   ok = TRUE;

   /* Find data location */
   curr = hashmap_hash_int( m, key );

   /* Linear probing, if necessary */
   for( i = 0 ; MAX_CHAIN_LENGTH > i ; i++ ) {
      in_use = m->data[curr].in_use;
      if( 1 == in_use ) {
#ifdef DEBUG_MATCHING
         scaffold_print_debug( "Hashmap: %s vs %s\n", bdata( m->data[curr].key ), bdata( key ) );
#endif /* DEBUG_MATCHING */
         if( 0 == bstrcmp( m->data[curr].key, key ) ) {
            return TRUE;
         }
      }

      curr = (curr + 1) % m->table_size;
   }

cleanup:
   if( FALSE != lock && FALSE != ok ) {
      hashmap_lock( m, FALSE );
   }
   return FALSE;
}

BOOL hashmap_contains_key( struct HASHMAP* m, const bstring key ) {
   return hashmap_contains_key_internal( m, key, TRUE );
}

BOOL hashmap_contains_key_nolock( struct HASHMAP* m, const bstring key ) {
   return hashmap_contains_key_internal( m, key, FALSE );
}

/*
 * Iterate the function parameter over each element in the hashmap.  The
 * additional any_t argument is passed to the function as its first
 * argument and the hashmap element is the second.
 */
void* hashmap_iterate( struct HASHMAP* m, hashmap_search_cb callback, void* arg ) {
   SCAFFOLD_SIZE i;
   void* found = NULL;
   void* data = NULL;
   void* test = NULL;
   BOOL ok = FALSE;

   scaffold_check_null( m );
   scaffold_assert( HASHMAP_SENTINAL == m->sentinal );
   scaffold_check_zero_against( m->last_error, hashmap_count( m ) );

   hashmap_lock( m, TRUE );
   ok = TRUE;

   /* Linear probing */
   for( i = 0; m->table_size > i ; i++ ) {
      if( 0 != m->data[i].in_use ) {
         data = (void*)(m->data[i].data);
         test = callback( m->data[i].key, data, arg );
         if( NULL != test ) {
            found = test;
            goto cleanup;
         }
      }
   }

cleanup:
   if( TRUE == ok ) {
      hashmap_lock( m, FALSE );
   }
   return found;
}

struct VECTOR* hashmap_iterate_v( struct HASHMAP* m, hashmap_search_cb callback, void* arg ) {
   struct VECTOR* found = NULL;
   void* data = NULL;
   void* test = NULL;
   BOOL ok = FALSE;
   int i;

   scaffold_check_null( m );
   scaffold_assert( HASHMAP_SENTINAL == m->sentinal );
   scaffold_check_zero_against( m->last_error, hashmap_count( m ) );

   hashmap_lock( m, TRUE );
   ok = TRUE;

   /* Linear probing */
   for( i = 0; m->table_size > i ; i++ ) {
      if( 0 != m->data[i].in_use ) {
         data = (void*)(m->data[i].data);
         test = callback( m->data[i].key, data, arg );
         if( NULL != test ) {
            if( NULL == found ) {
               vector_new( found );
            }
            vector_add( found, test );
         }
      }
   }

cleanup:
   hashmap_verify_size( m );
   if( TRUE == ok ) {
      hashmap_lock( m, FALSE );
   }
   return found;
}

/** \brief Use a callback to delete items. The callback frees the item or
 *         decreases its refcount as applicable.
 *
 * \return Number of items deleted.
 *
 */
SCAFFOLD_SIZE hashmap_remove_cb( struct HASHMAP* m, hashmap_delete_cb callback, void* arg ) {
   SCAFFOLD_SIZE i;
   SCAFFOLD_SIZE removed = 0;
   void* data;
   BOOL locked = FALSE;

   /* FIXME: Delete dynamic arrays and reset when empty. */

   scaffold_check_null( m );
   scaffold_assert( HASHMAP_SENTINAL == m->sentinal );
   scaffold_check_zero_against( m->last_error, hashmap_count( m ) );

   hashmap_lock( m, TRUE );
   locked = TRUE;

   if( 0 >= hashmap_count( m ) ) {
      goto cleanup; /* Quietly. */
   }

   /* Linear probing */
   for( i = 0 ; m->table_size > i ; i++ ) {
      if( 0 != m->data[i].in_use ) {
         data = (void*)(m->data[i].data);
         if( FALSE != callback( m->data[i].key, data, arg ) ) {
            /* Blank out the fields */
            m->data[i].in_use = 0;
            m->data[i].data = NULL;
            bwriteallow( (*m->data[i].key) );
            bdestroy( m->data[i].key );
            m->data[i].key = NULL;

            /* Reduce the size */
            m->size--;

            removed++;
         }
      }
   }

cleanup:
   hashmap_verify_size( m );
   if( TRUE == locked ) {
      hashmap_lock( m, FALSE );
   }
   return removed;
}

/*
 * Remove an element with that key from the map
 */
BOOL hashmap_remove( struct HASHMAP* m, const bstring key ) {
   int i;
   int curr;
   int in_use;
   BOOL removed = FALSE;

   scaffold_check_null( m );
   scaffold_assert( HASHMAP_SENTINAL == m->sentinal );
   scaffold_check_zero_against( m->last_error, hashmap_count( m ) );

   /* Find key */
   curr = hashmap_hash_int(m, key);

   /* Linear probing, if necessary */
   for( i = 0 ; i < MAX_CHAIN_LENGTH ; i++) {

      in_use = m->data[curr].in_use;
      if( 1 == in_use ) {
         if( 0 == bstrcmp( m->data[curr].key, key ) ) {
            refcount_test_dec( m->data[curr].data );

            /* Blank out the fields */
            m->data[curr].in_use = 0;
            m->data[curr].data = NULL;
            bwriteallow( (*m->data[curr].key) );
            bdestroy( m->data[curr].key );
            m->data[curr].key = NULL;

            /* Reduce the size */
            m->size--;
            removed = TRUE;
            goto cleanup;
         }
      }
      curr = (curr + 1) % m->table_size;
   }
cleanup:
   hashmap_verify_size( m );
   return removed;
}

/* Deallocate the hashmap */
void hashmap_cleanup( struct HASHMAP* m ) {
   scaffold_check_null( m );
   scaffold_assert( HASHMAP_SENTINAL == m->sentinal );
   scaffold_assert( 0 >= hashmap_count( m ) );
   free( m->data );
   m->sentinal = 0;
cleanup:
   return;
}

/* Return the length of the hashmap */
int hashmap_count( struct HASHMAP* m ) {
   scaffold_check_null( m );
   scaffold_assert( HASHMAP_SENTINAL == m->sentinal );
   return m->size;
cleanup:
   return 0;
}

#if 0
static int hashmap_active_length( struct HASHMAP* m ) {
   int i;
   int count = 0;
   scaffold_check_null( m );
   scaffold_assert( HASHMAP_SENTINAL == m->sentinal );

   /* On empty hashmap, return immediately */
   if( 0 >= hashmap_count( m ) ) {
      goto cleanup;
   }

   /* Linear probing */
   for( i = 0; i < m->table_size ; i++ ) {
      if( 0 != m->data[i].in_use ) {
         count++;
      }
   }

cleanup:
   return count;
}
#endif

void hashmap_lock( struct HASHMAP* m, BOOL lock ) {
   #ifdef USE_THREADS
   #error Locking mechanism undefined!
   #elif defined( DEBUG )
   if( TRUE == lock ) {
      scaffold_assert( 0 == m->lock_count );
      m->lock_count++;
   } else {
      scaffold_assert( 1 == m->lock_count );
      m->lock_count--;
   }
   #endif /* USE_THREADS */
}
