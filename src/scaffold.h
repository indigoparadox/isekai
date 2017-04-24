#ifndef SCAFFOLD_H
#define SCAFFOLD_H

#if defined( __STDC_VERSION__ ) && __STDC_VERSION__ >= 199901L
#define C99
#endif /* C99 */

/* = OS Detection Stuff = */

#ifdef WIN16

#include <windows.h>

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

#elif defined( _WIN32 )

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <windows.h>
#include <stdint.h>

#elif defined( __linux )

#define _GNU_SOURCE
#undef _POSIX_SOURCE
#define _POSIX_SOURCE 1
#undef __USE_POSIX
#define __USE_POSIX 1
#include <stdint.h>

#endif /* WIN16 || _WIN32 || __linux */

/* = Debug */

#ifdef DEBUG
#include <assert.h>
#endif /* DEBUG */

/* = Common Headers = */

#include <stdio.h>
#include <stddef.h>
#include <memory.h>
#include <stdlib.h>

#include "bstrlib/bstrlib.h"
#include "colors.h"

COLOR_TABLE( SCAFFOLD )

/* = Missing Types = */

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

typedef int32_t INTERVAL;
typedef uint8_t SERIAL;
#define SERIAL_MIN 1
#define SERIAL_MAX UCHAR_MAX - SERIAL_MIN - 1

/* = Configuration = */

#define SENTINAL 19691

#define SCAFFOLD_PRINT_BUFFER_ALLOC 110

#if defined( USE_DUKTAPE ) || defined( USE_TINYPY )
#define USE_VM
#endif /* USE_DUKTAPE || USE_TINYPY */

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
   SCAFFOLD_ERROR_RANDOM,
   SCAFFOLD_ERROR_CONNECTION_CLOSED
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
void scaffold_print_debug_color(
   const bstring mod_in, SCAFFOLD_COLOR color, const char* message, ...
);

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

#ifdef USE_COLORED_CONSOLE
#if defined _WIN32 || defined WIN16
#error Colored console is not compatible with Windows!
#endif /* _WIN32 || WIN16 */

#ifdef SCAFFOLD_LOG_FILE
/* TODO: Colored log output to HTML. */
#error Colored console is not compatible with log files!
#endif /* SCAFFOLD_LOG_FILE */

#ifdef SCAFFOLD_C

static
struct tagbstring ansi_color_strs[7] = {
   /* GRAPHICS_COLOR_BLUE        =  9, */ bsStatic( "\x1b[34m" ),
   /* GRAPHICS_COLOR_GREEN       = 10, */ bsStatic( "\x1b[32m" ),
   /* GRAPHICS_COLOR_CYAN        = 11, */ bsStatic( "\x1b[36m" ),
   /* GRAPHICS_COLOR_RED         = 12, */ bsStatic( "\x1b[31m" ),
   /* GRAPHICS_COLOR_MAGENTA     = 13, */ bsStatic( "\x1b[35m" ),
   /* GRAPHICS_COLOR_YELLOW      = 14, */ bsStatic( "\x1b[33m" ),
   /* GRAPHICS_COLOR_WHITE       = 15  */ bsStatic( "\x1b[0m" )
};
#endif /* SCAFFOLD_C */

#endif /* USE_COLORED_CONSOLE */

typedef union CONTAINER_IDX_VALUE {
   SCAFFOLD_SIZE index;
   bstring key;
} CONTAINER_IDX_VALUE;

typedef enum CONTAINER_IDX_TYPE {
   CONTAINER_IDX_NUMBER,
   CONTAINER_IDX_STRING
} CONTAINER_IDX_TYPE;

struct CONTAINER_IDX {
   CONTAINER_IDX_TYPE type;
   CONTAINER_IDX_VALUE value;
};

/* = Utility Macros = */

#define SCAFFOLD_MODULE( mod_name ) static struct tagbstring module = bsStatic( mod_name )

#define scaffold_gen_serial( object, vector ) \
   do { \
      object->serial = SERIAL_MIN + (rand() % SERIAL_MAX); \
   } while( 0 == object->serial || NULL != vector_get( vector, object->serial ) );

#define scaffold_static_string( cstr ) \
    blk2bstr( bsStaticBlkParms( cstr ) )

#define scaffold_assign_or_cpy_c( target, source, retval ) \
   if( NULL == target ) { \
      target = bfromcstr( source ); \
   } else { \
      retval = bassigncstr( target, source ); \
      scaffold_check_nonzero( retval ); \
   }

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

#define scaffold_check_null_continue( pointer ) \
    if( NULL == pointer ) { \
        scaffold_error = SCAFFOLD_ERROR_NULLPO; \
        if( TRUE != scaffold_error_silent ) { \
            scaffold_print_error( &module, "Scaffold: Null pointer on line: %d\n", __LINE__ ); \
            scaffold_print_debug( &module, "Continuing loop..." ); \
        } \
        continue; \
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


#define scaffold_check_zero_against( last, value, msg ) \
    if( 0 == value && SCAFFOLD_ERROR_ZERO != last ) { \
        last = SCAFFOLD_ERROR_ZERO; \
        if( TRUE != scaffold_error_silent ) { \
            scaffold_print_error( &module, "Scaffold: Zero error on line: %d: %s\n", __LINE__, msg ); \
        } \
        goto cleanup; \
    } else if( SCAFFOLD_ERROR_ZERO != last ) { \
        last = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_zero( value, msg ) \
   scaffold_check_zero_against( scaffold_error, value, msg )

#define scaffold_char_is_printable( c ) \
    (0x7f > (c) && 0x20 < (c))

#define scaffold_container_of( ptr, type, member ) \
    ((type *)((char *)(ptr) - offsetof( type, member )))

struct VECTOR;

/* Vector needs some stuff above but is needed for stuff below. */
#include "vector.h"

#define scaffold_byte( number ) (0xff & number)

#define scaffold_alloc( count, type ) \
   (type*)calloc( count, sizeof( type ) )
#define scaffold_free( ptr ) free( ptr )
#define scaffold_realloc( ptr, count, type ) \
   (type*)realloc( \
      ptr, \
      (count * sizeof( type ) >= count ? (count * sizeof( type )) : 0) \
   )

BOOL scaffold_is_numeric( bstring line );
bstring scaffold_list_pop_string( struct bstrList* list );
void scaffold_list_remove_string( struct bstrList* list, bstring str );
void scaffold_list_append_string_cpy( struct bstrList* list, bstring str );
BOOL scaffold_string_is_printable( bstring str );
void scaffold_snprintf( bstring buffer, const char* message, ... );
void scaffold_vsnprintf( bstring buffer, const char* message, va_list varg );
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
BOOL scaffold_buffer_grow(
   BYTE** buffer, SCAFFOLD_SIZE* len, SCAFFOLD_SIZE new_len
)
#ifdef __GNUC__
__attribute__ ((warn_unused_result))
#endif /* __GNUC__ */
;
BOOL scaffold_random_bytes( BYTE* ptr, SCAFFOLD_SIZE length );
void scaffold_colorize( bstring str, SCAFFOLD_COLOR color );
int scaffold_strcmp_caseless( const char* s0, const char* s1 );

#ifdef SCAFFOLD_C

/* SCAFFOLD_MODULE( "scaffold.c" ); */
static struct tagbstring module = bsStatic( "scaffold.c" );

struct tagbstring scaffold_empty_string = bsStatic( "" );
struct tagbstring scaffold_space_string = bsStatic( " " );
struct tagbstring scaffold_colon_string = bsStatic( ":" );
struct tagbstring scaffold_exclamation_string = bsStatic( "!" );
struct tagbstring scaffold_null = bsStatic( "(null)" );

#if defined( _WIN32 ) || defined( WIN16 )
struct tagbstring scaffold_dirsep_string = bsStatic( "\\" );
#else
struct tagbstring scaffold_dirsep_string = bsStatic( "/" );
#endif /* _WIN32 || WIN16 */
#define SCAFFOLD_DIRSEP_CHAR scaffold_dirsep_string.data[0]

#else

#ifdef DEBUG
extern SCAFFOLD_TRACE scaffold_trace_path;
#endif /* DEBUG */
extern struct tagbstring scaffold_empty_string;
extern struct tagbstring scaffold_space_string;
extern struct tagbstring scaffold_colon_string;
extern struct tagbstring scaffold_exclamation_string;
extern struct tagbstring scaffold_null;
extern uint8_t scaffold_error;
extern BOOL scaffold_error_silent;

#endif /* SCAFFOLD_C */

#endif /* SCAFFOLD_H */
