#ifndef SCAFFOLD_H
#define SCAFFOLD_H

#ifdef WIN16

#include <windows.h>

#elif defined( WIN32 )

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <windows.h>
#include <stdint.h>

#elif defined( __STDC_VERSION__ ) && __STDC_VERSION__ >= 199901L

#define C99
#define _GNU_SOURCE
#include <stdint.h>

#endif /* WIN16 || WIN32 || C99 */

#include <stdio.h>
#include <stddef.h>

#ifdef DEBUG
#include <assert.h>
#endif /* DEBUG */

#include "bstrlib/bstrlib.h"

#define SENTINAL 19691

#define SCAFFOLD_PRINT_BUFFER_ALLOC 110

#ifdef WIN16

typedef unsigned char BYTE;
typedef unsigned long SCAFFOLD_SIZE;
typedef long SCAFFOLD_SIZE_SIGNED;
typedef int int16_t;
typedef unsigned int uint16_t;
typedef long int32_t;
typedef unsigned long uint32_t;
typedef char int8_t;
typedef unsigned char uint8_t;
#define __FUNCTION__ "Unavailable:"
#define SNPRINTF_UNAVAILABLE

#else

#ifndef BYTE
typedef uint8_t BYTE;
#endif /* BYTE */

#if !defined( BOOL ) && !defined( WIN32 )
typedef uint8_t BOOL;
#endif /* BOOL */

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

#ifdef USE_SIZET
typedef size_t SCAFFOLD_SIZE;
typedef ssize_t SCAFFOLD_SIZE_SIGNED;
#else
typedef uint32_t SCAFFOLD_SIZE;
typedef int32_t SCAFFOLD_SIZE_SIGNED;
#endif /* USE_SIZET */

#endif /* WIN16 */

typedef enum {
   SCAFFOLD_ERROR_NONE,
   SCAFFOLD_ERROR_MISC,
   SCAFFOLD_ERROR_NULLPO,
   SCAFFOLD_ERROR_OUTOFBOUNDS,
   SCAFFOLD_ERROR_NEGATIVE,
   SCAFFOLD_ERROR_NONZERO,
   SCAFFOLD_ERROR_ZERO,
   SCAFFOLD_ERROR_NOT_NULLPO,
   SCAFFOLD_ERROR_DUPLICATE
} SCAFFOLD_ERROR;

#ifdef DEBUG
typedef enum {
   SCAFFOLD_TRACE_NONE,
   SCAFFOLD_TRACE_CLIENT,
   SCAFFOLD_TRACE_SERVER
} SCAFFOLD_TRACE;
#endif /* DEBUG */

#ifdef USE_INLINE
#define SCAFFOLD_INLINE inline
#else
#define SCAFFOLD_INLINE
#endif /* USE_INLINE */

#ifdef SCAFFOLD_LOG_FILE
#ifndef SCAFFOLD_C
extern FILE* scaffold_log_handle;
extern FILE* scaffold_log_handle_err;
#endif /* SCAFFOLD_C */
#else
#define scaffold_log_handle stdout
#define scaffold_log_handle_err stderr
#endif /* SCAFFOLD_LOG_FILE */

#ifdef DEBUG

void scaffold_print_debug( const bstring module, const char* message, ... );
void scaffold_print_info( const bstring module, const char* message, ... );
void scaffold_print_error( const bstring module, const char* message, ... );

#define scaffold_assert( arg ) assert( arg )

#define scaffold_assert_client() \
   scaffold_assert( SCAFFOLD_TRACE_CLIENT == scaffold_trace_path )
#define scaffold_assert_server() \
   scaffold_assert( SCAFFOLD_TRACE_SERVER == scaffold_trace_path )

#define scaffold_set_client() \
   scaffold_trace_path = SCAFFOLD_TRACE_CLIENT;
#define scaffold_set_server() \
   scaffold_trace_path = SCAFFOLD_TRACE_SERVER;

#else

/* Disable debug-level notifications. */

#define scaffold_assert( expr )

#define scaffold_assert_client()
#define scaffold_assert_server()
#define scaffold_set_client()
#define scaffold_set_server()

#endif /* DEBUG */

#define SCAFFOLD_MODULE( mod_name ) static struct tagbstring module = bsStatic( mod_name )

#define scaffold_static_string( cstr ) \
    blk2bstr( bsStaticBlkParms( cstr ) )

#define scaffold_check_silence() scaffold_error_silent = TRUE;
#define scaffold_check_unsilence() scaffold_error_silent = FALSE;

#define scaffold_check_null_msg( pointer, message ) \
    if( NULL == pointer ) { \
        scaffold_error = SCAFFOLD_ERROR_NULLPO; \
        if( TRUE != scaffold_error_silent ) { \
            scaffold_print_error( &module, "Scaffold: Null pointer on line: %d: %s\n", __LINE__, message ); \
        } \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_null( pointer ) \
    if( NULL == pointer ) { \
        scaffold_error = SCAFFOLD_ERROR_NULLPO; \
        if( TRUE != scaffold_error_silent ) { \
            scaffold_print_error( &module, "Scaffold: Null pointer on line: %d\n", __LINE__ ); \
        } \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_not_null( pointer ) \
    if( NULL != pointer ) { \
        scaffold_error = SCAFFOLD_ERROR_NOT_NULLPO; \
        if( TRUE != scaffold_error_silent ) { \
            scaffold_print_error( &module, "Scaffold: Non-null pointer on line: %d\n", __LINE__ ); \
        } \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_bounds( index, bound ) \
    if( index >= bound ) { \
        scaffold_error = SCAFFOLD_ERROR_OUTOFBOUNDS; \
        if( TRUE != scaffold_error_silent ) { \
            scaffold_print_error( &module, "Scaffold: Out of bounds on line: %d\n", __LINE__ ); \
        } \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_negative( value ) \
    if( 0 > value ) { \
        scaffold_error = SCAFFOLD_ERROR_NEGATIVE; \
        if( TRUE != scaffold_error_silent ) { \
            scaffold_print_error( &module, "Scaffold: Bad negative on line: %d\n", __LINE__ ); \
        } \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_nonzero( value ) \
    if( 0 != value ) { \
        scaffold_error = SCAFFOLD_ERROR_NONZERO; \
        if( TRUE != scaffold_error_silent ) { \
            scaffold_print_error( &module, "Scaffold: Nonzero error on line: %d\n", __LINE__ ); \
        } \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_zero_msg( value, message ) \
    if( 0 == value ) { \
        scaffold_error = SCAFFOLD_ERROR_ZERO; \
        if( TRUE != scaffold_error_silent ) { \
            scaffold_print_error( &module, "Scaffold: Zero error on line: %d: %s\n", __LINE__, message ); \
        } \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }


#define scaffold_check_zero_against( last, value ) \
    if( 0 == value && SCAFFOLD_ERROR_ZERO != last ) { \
        last = SCAFFOLD_ERROR_ZERO; \
        if( TRUE != scaffold_error_silent ) { \
            scaffold_print_error( &module, "Scaffold: Zero error on line: %d\n", __LINE__ ); \
        } \
        goto cleanup; \
    } else if( SCAFFOLD_ERROR_ZERO != last ) { \
        last = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_zero( value ) \
   scaffold_check_zero_against( scaffold_error, value )

#define scaffold_char_is_printable( c ) \
    (0x7f > (c) && 0x20 < (c))

#define scaffold_container_of( ptr, type, member ) \
    ((type *)((char *)(ptr) - offsetof( type, member )))

struct VECTOR;

/* Vector needs some stuff above but is needed for stuff below. */
#include "vector.h"

#define scaffold_byte( number ) (0xff & number)

BOOL scaffold_is_numeric( bstring line );
bstring scaffold_list_pop_string( struct bstrList* list );
void scaffold_list_remove_string( struct bstrList* list, bstring str );
void scaffold_list_append_string_cpy( struct bstrList* list, bstring str );
BOOL scaffold_string_is_printable( bstring str );
void scaffold_snprintf( bstring buffer, const char* message, va_list varg );
void scaffold_random_string( bstring rand_str, SCAFFOLD_SIZE len );
SCAFFOLD_SIZE_SIGNED scaffold_read_file_contents( bstring path, BYTE** buffer, SCAFFOLD_SIZE* len )
#ifdef __GNUC__
__attribute__ ((warn_unused_result))
#endif /* __GNUC__ */
;
SCAFFOLD_SIZE_SIGNED scaffold_write_file( bstring path, BYTE* data, SCAFFOLD_SIZE len, BOOL mkdirs )
#ifdef __GNUC__
__attribute__ ((warn_unused_result))
#endif /* __GNUC__ */
;
void scaffold_list_dir(
   const bstring path, struct VECTOR* list, const bstring filter,
   BOOL dir_only, BOOL show_hidden
);
bstring scaffold_basename( bstring path )
#ifdef __GNUC__
__attribute__ ((warn_unused_result))
#endif /* __GNUC__ */
;
BOOL scaffold_check_directory( const bstring path )
#ifdef __GNUC__
__attribute__ ((warn_unused_result))
#endif /* __GNUC__ */
;
void scaffold_join_path( bstring path1, const bstring path2 );

#ifdef SCAFFOLD_C
/* SCAFFOLD_MODULE( "scaffold.c" ); */
static struct tagbstring module = bsStatic( "scaffold.c" );
#else
#ifdef DEBUG
extern SCAFFOLD_TRACE scaffold_trace_path;
#endif /* DEBUG */
extern struct tagbstring scaffold_empty_string;
extern struct tagbstring scaffold_space_string;
extern uint8_t scaffold_error;
extern BOOL scaffold_error_silent;
#endif /* SCAFFOLD_C */

#endif /* SCAFFOLD_H */
