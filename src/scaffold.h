#ifndef SCAFFOLD_H
#define SCAFFOLD_H

#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stddef.h>

#include "bstrlib/bstrlib.h"

#define SENTINAL 19691

#ifndef BYTE
typedef int8_t BYTE;
#endif /* BYTE */

#ifndef BOOL
typedef uint8_t BOOL;
#endif /* BOOL */

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

typedef enum {
   SCAFFOLD_ERROR_NONE,
   SCAFFOLD_ERROR_MISC,
   SCAFFOLD_ERROR_NULLPO,
   SCAFFOLD_ERROR_OUTOFBOUNDS,
   SCAFFOLD_ERROR_NEGATIVE,
   SCAFFOLD_ERROR_NONZERO,
   SCAFFOLD_ERROR_ZERO,
   SCAFFOLD_ERROR_NOT_NULLPO,
   SCAFFOLD_ERROR_DUPLICATE,
} SCAFFOLD_ERROR;

#ifdef DEBUG
typedef enum {
   SCAFFOLD_TRACE_NONE,
   SCAFFOLD_TRACE_CLIENT,
   SCAFFOLD_TRACE_SERVER,
} SCAFFOLD_TRACE;
#endif /* DEBUG */

//#ifndef GFX_CURSES
#if 1
#define scaffold_print_info( ... ) fprintf( stdout, __FILE__ ": " __VA_ARGS__ );
#define scaffold_print_error( ... ) fprintf( stderr, __FILE__ ": " __VA_ARGS__ );

#ifdef DEBUG

#define scaffold_print_debug( ... ) fprintf( stdout, __FILE__ ": " __VA_ARGS__ );

#define scaffold_assert_client() \
   assert( SCAFFOLD_TRACE_CLIENT == scaffold_trace_path )
#define scaffold_assert_server() \
   assert( SCAFFOLD_TRACE_SERVER == scaffold_trace_path )

#define scaffold_set_client() \
   assert( SCAFFOLD_TRACE_CLIENT == scaffold_trace_path )
#define scaffold_set_server() \
   assert( SCAFFOLD_TRACE_SERVER == scaffold_trace_path )

#else

#define scaffold_assert_client()
#define scaffold_assert_server()
#define scaffold_set_client()
#define scaffold_set_server()

#define scaffold_print_debug( ... )

#endif /* DEBUG */

#define scaffold_debug_file( handlename, filename, data_ptr, data_size ) \
   FILE* handlename = fopen( filename, "w" ); \
   fwrite( data_ptr, sizeof( uint8_t ), data_size, handlename ); \
   fclose( handlename );

#else
#define scaffold_print_info( ... )
#define scaffold_print_error( ... )
#define scaffold_print_debug( ... )
#endif /* GFX_CURSES */

#define scaffold_static_string( cstr ) \
    blk2bstr( bsStaticBlkParms( cstr ) )

#define scaffold_check_null( pointer ) \
    if( NULL == pointer ) { \
        scaffold_error = SCAFFOLD_ERROR_NULLPO; \
        scaffold_print_error( "Scaffold: Null pointer on line: %d\n", __LINE__ ); \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_not_null( pointer ) \
    if( NULL != pointer ) { \
        scaffold_error = SCAFFOLD_ERROR_NOT_NULLPO; \
        scaffold_print_error( "Scaffold: Non-null pointer on line: %d\n", __LINE__ ); \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_bounds( index, bound ) \
    if( index >= bound ) { \
        scaffold_error = SCAFFOLD_ERROR_OUTOFBOUNDS; \
        scaffold_print_error( "Scaffold: Out of bounds on line: %d\n", __LINE__ ); \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_negative( value ) \
    if( 0 > value ) { \
        scaffold_error = SCAFFOLD_ERROR_NEGATIVE; \
        scaffold_print_error( "Scaffold: Bad negative on line: %d\n", __LINE__ ); \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_nonzero( value ) \
    if( 0 != value ) { \
        scaffold_error = SCAFFOLD_ERROR_NONZERO; \
        scaffold_print_error( "Scaffold: Nonzero error on line: %d\n", __LINE__ ); \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_zero( value ) \
    if( 0 == value ) { \
        scaffold_error = SCAFFOLD_ERROR_ZERO; \
        scaffold_print_error( "Scaffold: Zero error on line: %d\n", __LINE__ ); \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_char_is_printable( c ) \
    (0x7f > (c) && 0x20 < (c))

#define scaffold_container_of( ptr, type, member ) \
    ((type *)((char *)(ptr) - offsetof( type, member )))

BOOL scaffold_is_numeric( bstring line );
bstring scaffold_pop_string( struct bstrList* list );
void scaffold_list_remove_string( struct bstrList* list, bstring str );
void scaffold_list_append_string_cpy( struct bstrList* list, bstring str );
BOOL scaffold_string_is_printable( bstring str );
#ifdef DEBUG
void scaffold_printf_debug( const char* message, ... );
#endif /* DEBUG */
void scaffold_snprintf( bstring buffer, const char* message, va_list varg );
void scaffold_random_string( bstring rand_str, size_t len );

#ifndef SCAFFOLD_C
#ifdef DEBUG
extern SCAFFOLD_TRACE scaffold_trace_path;
#endif /* DEBUG */
extern struct tagbstring scaffold_empty_string;
extern struct tagbstring scaffold_space_string;
extern uint8_t scaffold_error;
#endif /* SCAFFOLD_C */

#endif /* SCAFFOLD_H */
