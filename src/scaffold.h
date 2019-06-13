#ifndef SCAFFOLD_H
#define SCAFFOLD_H

#if defined( __STDC_VERSION__ ) && __STDC_VERSION__ >= 199901L
#define C99
#endif /* C99 */

#ifdef USE_SHM
#define DEBUG_NO_CLIENT_SERVER_MODEL
#endif /* USE_SHM */

/* = OS Detection Stuff = */

#ifdef __palmos__

typedef unsigned char BYTE;
typedef unsigned char bool;
typedef unsigned long SCAFFOLD_SIZE;
typedef long SCAFFOLD_SIZE_SIGNED;
#define SCAFFOLD_SIZE_MAX ULONG_MAX
#define SCAFFOLD_SIZE_SIGNED_MAX LONG_MAX
typedef int int16_t;
typedef unsigned int uint16_t;
typedef long int32_t;
typedef unsigned long uint32_t;
typedef char int8_t;
typedef unsigned char uint8_t;
#define __FUNCTION__ "Unavailable:"
#define SNPRINTF_UNAVAILABLE
#define true 1
#define false 0
#define USE_SYNCBUFF
#undef __GNUC__
#define EZXML_NOMMAP

#elif defined( WIN16 )

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

#define SCAFFOLD_SIZE_MAX ULONG_MAX
#define SCAFFOLD_SIZE_SIGNED_MAX LONG_MAX

#define USE_CLOCK 1
#define USE_FILE 1

#elif defined( _WIN32 ) && !defined( __MINGW32__ )

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <windows.h>
#include <stdint.h>

typedef unsigned char BYTE;
typedef unsigned long SCAFFOLD_SIZE;
typedef long SCAFFOLD_SIZE_SIGNED;

#define SCAFFOLD_SIZE_MAX ULONG_MAX
#define SCAFFOLD_SIZE_SIGNED_MAX LONG_MAX

#define USE_CLOCK 1
#define USE_FILE 1

#elif defined( __linux ) || defined( DJGPP ) || defined( __MINGW32__ )

#define _GNU_SOURCE
#undef _POSIX_SOURCE
#define _POSIX_SOURCE 1
#undef __USE_POSIX
#define __USE_POSIX 1

#include <stdint.h>

#ifndef BYTE
typedef uint8_t BYTE;
#endif /* BYTE */

#if 0
#if !defined( bool ) && !defined( WIN32 )
typedef uint8_t bool;
#endif /* bool */

#ifndef true
#define true 1
#endif /* true */

#ifndef false
#define false 0
#endif /* false */
#endif

#ifdef USE_SIZET
typedef size_t SCAFFOLD_SIZE;
typedef ssize_t SCAFFOLD_SIZE_SIGNED;
#else
typedef uint32_t SCAFFOLD_SIZE;
typedef int32_t SCAFFOLD_SIZE_SIGNED;
#define SCAFFOLD_SIZE_MAX UINT_MAX
#define SCAFFOLD_SIZE_SIGNED_MAX INT_MAX
#endif /* USE_SIZET */

#define USE_CLOCK 1
#define USE_FILE 1

#endif /* __palmos__ || WIN16 || _WIN32 || __linux */

/* = Debug = */

#ifdef DEBUG
#include <assert.h>
#endif /* DEBUG */

/* = Common Headers = */

#include <stdio.h>
#include <stddef.h>

#include <libvcol.h>
#include "bstrglue.h"
#include <colors.h>

COLOR_TABLE( SCAFFOLD )

/* = Serial Types = */

typedef int32_t INTERVAL;
typedef uint8_t SERIAL;
#define SERIAL_MIN 1
#define SERIAL_MAX UCHAR_MAX - SERIAL_MIN - 1

typedef uint16_t BIG_SERIAL;
#define BIG_SERIAL_MIN 1
#define BIG_SERIAL_MAX UINT_MAX - BIG_SERIAL_MIN - 1

/* = Configuration = */

#define SENTINAL 19691

#define SCAFFOLD_PRINT_BUFFER_ALLOC 110

#if defined( USE_DUKTAPE ) || defined( USE_TINYPY )
#ifndef USE_VM
#define USE_VM
#endif /* !USE_VM */
#endif /* USE_DUKTAPE || USE_TINYPY */

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

#ifndef DEBUG
/* Disable debug-level notifications. */
#define assert( expr )
#endif /* DEBUG */

#if defined( DEBUG ) && !defined( DEBUG_NO_CLIENT_SERVER_MODEL )

#define scaffold_assert_client() \
   assert( 0 == strncmp( "CLIENT", lg_get_trace_cat_name(), 6 ) );
#define scaffold_assert_server() \
   assert( 0 == strncmp( "SERVER", lg_get_trace_cat_name(), 6 ) );

#define scaffold_set_client() \
   lg_set_trace_cat( "CLIENT" )
#define scaffold_set_server() \
   lg_set_trace_cat( "SERVER" )

#else

#define scaffold_assert_client()
#define scaffold_assert_server()
#define scaffold_set_client()
#define scaffold_set_server()

#endif /* DEBUG && !DEBUG_NO_CLIENT_SERVER_MODEL */

#ifdef USE_COLORED_CONSOLE
#if defined _WIN32 || defined WIN16
#error Colored console is not compatible with Windows!
#endif /* _WIN32 || WIN16 */

#ifdef SCAFFOLD_LOG_FILE
/* TODO: Colored log output to HTML. */
#error Colored console is not compatible with log files!
#endif /* SCAFFOLD_LOG_FILE */

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

#define scaffold_static_string( cstr ) \
    blk2bstr( bsStaticBlkParms( cstr ) )

#define scaffold_assign_or_cpy_c( target, source, retval ) \
   if( NULL == target ) { \
      target = bfromcstr( source ); \
   } else { \
      retval = bassigncstr( target, source ); \
      lgc_nonzero( retval ); \
   }

#define scaffold_check_silence() scaffold_error_silent = true;
#define scaffold_check_unsilence() scaffold_error_silent = false;

#define scaffold_check_null_msg( pointer, message ) \
    if( NULL == pointer ) { \
        scaffold_error = SCAFFOLD_ERROR_NULLPO; \
        if( true != scaffold_error_silent ) { \
            scaffold_print_error( \
               __FILE__, \
               "Scaffold: Null pointer on line: %d: %s\n", \
               __LINE__, message ); \
        } \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_null_warning( pointer ) \
    if( NULL == pointer ) { \
        scaffold_error = SCAFFOLD_ERROR_NULLPO; \
        if( true != scaffold_warning_silent ) { \
            scaffold_print_warning( \
               __FILE__, "Scaffold: Null pointer on line: %d\n", __LINE__ ); \
        } \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_null( pointer ) \
    if( NULL == pointer ) { \
        scaffold_error = SCAFFOLD_ERROR_NULLPO; \
        if( true != scaffold_error_silent ) { \
            scaffold_print_error( \
               __FILE__, "Scaffold: Null pointer on line: %d\n", __LINE__ ); \
        } \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_null_continue( pointer ) \
    if( NULL == pointer ) { \
        scaffold_error = SCAFFOLD_ERROR_NULLPO; \
        if( true != scaffold_error_silent ) { \
            scaffold_print_error( \
               __FILE__, "Scaffold: Null pointer on line: %d\n", __LINE__ ); \
            scaffold_print_debug( __FILE__, "Continuing loop..." ); \
        } \
        continue; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_not_null( pointer ) \
    if( NULL != pointer ) { \
        scaffold_error = SCAFFOLD_ERROR_NOT_NULLPO; \
        if( true != scaffold_error_silent ) { \
            scaffold_print_error( \
               __FILE__, \
               "Scaffold: Non-null pointer on line: %d\n", __LINE__ ); \
        } \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_bounds( index, bound ) \
    if( index >= bound ) { \
        scaffold_error = SCAFFOLD_ERROR_OUTOFBOUNDS; \
        if( true != scaffold_error_silent ) { \
            scaffold_print_error( \
               __FILE__, "Scaffold: Out of bounds on line: %d\n", __LINE__ ); \
        } \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_negative( value ) \
    if( 0 > value ) { \
        scaffold_error = SCAFFOLD_ERROR_NEGATIVE; \
        if( true != scaffold_error_silent ) { \
            scaffold_print_error( \
               __FILE__, "Scaffold: Bad negative on line: %d\n", __LINE__ ); \
        } \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_nonzero( value ) \
    if( 0 != value ) { \
        scaffold_error = SCAFFOLD_ERROR_NONZERO; \
        if( true != scaffold_error_silent ) { \
            scaffold_print_error( \
               __FILE__, "Scaffold: Nonzero error on line: %d\n", __LINE__ ); \
        } \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_zero_msg( value, message ) \
    if( 0 == value ) { \
        scaffold_error = SCAFFOLD_ERROR_ZERO; \
        if( true != scaffold_error_silent ) { \
            scaffold_print_error( \
               __FILE__, \
               "Scaffold: Zero error on line: %d: %s\n", __LINE__, message ); \
        } \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }


#define scaffold_check_zero_against_warning( last, value, msg ) \
    if( 0 == value && SCAFFOLD_ERROR_ZERO != last ) { \
        last = SCAFFOLD_ERROR_ZERO; \
        if( true != scaffold_warning_silent ) { \
            scaffold_print_warning( \
               __FILE__, \
               "Scaffold: Zero warning on line: %d: %s\n", __LINE__, msg ); \
        } \
        goto cleanup; \
    } else if( SCAFFOLD_ERROR_ZERO != last ) { \
        last = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_zero_against( last, value, msg ) \
    if( 0 == value && SCAFFOLD_ERROR_ZERO != last ) { \
        last = SCAFFOLD_ERROR_ZERO; \
        if( true != scaffold_error_silent ) { \
            scaffold_print_error( \
               __FILE__, \
               "Scaffold: Zero error on line: %d: %s\n", __LINE__, msg ); \
        } \
        goto cleanup; \
    } else if( SCAFFOLD_ERROR_ZERO != last ) { \
        last = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_equal( value1, value2 ) \
    if( value1 != value2 ) { \
        scaffold_error = SCAFFOLD_ERROR_UNEQUAL; \
        if( true != scaffold_error_silent ) { \
            scaffold_print_error( \
               __FILE__,\
               "Values not equal: %d and %d: error on line: %d\n", \
               value1, value2, __LINE__ ); \
        } \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_zero( value, msg ) \
   scaffold_check_zero_against( scaffold_error, value, msg )

#define scaffold_char_is_printable( c ) \
    (0x7f > (c) && 0x20 < (c))

/** \brief Get the container struct of the given struct. Useful for structs
 *         that only ever exist inside of other structs.
 *(struct CHUNKER*)scaffold_container_of( ref, struct CHUNKER, refcount );
 * \param ptr        Instance pointer to struct to find container of.
 * \param type       Type of the container (e.g. struct SERVER).
 * \param member     Name of the class member in the container that ptr is.
 */
#define scaffold_container_of( ptr, type, member ) \
    ((type *)((char *)(ptr) - offsetof( type, member )))

struct VECTOR;

/* Vector needs some stuff above but is needed for stuff below. */
#include "libvcol.h"

#define scaffold_byte( number ) (0xff & number)

bool scaffold_is_numeric( bstring line );
bool scaffold_string_is_printable( bstring str );
void scaffold_snprintf( bstring buffer, const char* message, ... );
void scaffold_vsnprintf( bstring buffer, const char* message, va_list varg );
void scaffold_random_string( bstring rand_str, SCAFFOLD_SIZE len );
bool scaffold_random_bytes( BYTE* ptr, SCAFFOLD_SIZE length );
void scaffold_colorize( bstring str, SCAFFOLD_COLOR color );
int scaffold_strcmp_caseless( const char* s0, const char* s1 );
bool scaffold_buffer_grow(
   BYTE** buffer, SCAFFOLD_SIZE* len, SCAFFOLD_SIZE new_len
);

#ifdef SCAFFOLD_C

/* SCAFFOLD_MODULE( "scaffold.c" ); */
struct tagbstring scaffold_empty_string = bsStatic( "" );
struct tagbstring scaffold_space_string = bsStatic( " " );
struct tagbstring scaffold_colon_string = bsStatic( ":" );
struct tagbstring scaffold_exclamation_string = bsStatic( "!" );
struct tagbstring scaffold_null = bsStatic( "(null)" );

#else

extern struct tagbstring scaffold_empty_string;
extern struct tagbstring scaffold_space_string;
extern struct tagbstring scaffold_colon_string;
extern struct tagbstring scaffold_exclamation_string;
extern struct tagbstring scaffold_null;
extern uint8_t scaffold_error;
extern bool scaffold_error_silent;
extern bool scaffold_warning_silent;

#endif /* SCAFFOLD_C */

#endif /* SCAFFOLD_H */
