
#define SCAFFOLD_C
#include "scaffold.h"

static struct tagbstring str_scaffold_trace[3] = {
   bsStatic( "T_NONE" ),
   bsStatic( "T_CLIENT" ),
   bsStatic( "T_SERVER" )
};

#ifdef DEBUG
SCAFFOLD_TRACE scaffold_trace_path = SCAFFOLD_TRACE_NONE;
#endif /* DEBUG */

#ifdef SCAFFOLD_LOG_FILE
FILE* scaffold_log_handle = NULL;
FILE* scaffold_log_handle_err = NULL;
#endif /* SCAFFOLD_LOG_FILE */

int8_t scaffold_error = SCAFFOLD_ERROR_NONE;
BOOL scaffold_error_silent = FALSE;
bstring scaffold_print_buffer = NULL;

static char scaffold_random_chars[] =
   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

BOOL scaffold_is_numeric( bstring line ) {
   int i;
   BOOL is_numeric = TRUE;

   if( NULL == line ) {
      return FALSE;
   }

   for( i = 0 ; blength( line ) > i ; i++ ) {
      if( !isdigit( bdata( line )[i] ) ) {
         is_numeric = FALSE;
         break;
      }
   }

   return is_numeric;
}

/*
bstring scaffold_list_pop_string( struct bstrList* list ) {
   bstring popped = list->entry[0];
   int i;

   for( i = 1 ; list->qty > i ; i++ ) {
      list->entry[i - 1] = list->entry[i];
   }

   list->qty--;

   return popped;
}
*/

/*
void scaffold_list_remove_string( struct bstrList* list, bstring str ) {
   int i;
   int offset = 0;

   for( i = 0 ; list->qty - offset > i ; i++ ) {
      if( 0 == bstrcmp( list->entry[i], str ) ) {
         offset++;
      }
      if( 0 < offset ) {
         list->entry[i] = list->entry[i + offset];
      }
   }

   list->qty -= offset;

   return;
}
*/
#if 0
void scaffold_list_append_string_cpy( struct bstrList* list, bstring str ) {
   int bstr_result;

   scaffold_assert( NULL != list );
   scaffold_check_null( list->entry );

   if( list->qty + 1 >= list->mlen ) {
      bstr_result = bstrListAlloc( list, list->mlen * 2 );
      scaffold_check_nonzero( bstr_result );
      /*list->mlen *= 2;
      list->entry = (bstring*)realloc( list->entry, list->mlen );
      scaffold_check_null( list->entry );*/
   }

   list->entry[list->qty++] = bstrcpy( str );

cleanup:
   return;
}
#endif

BOOL scaffold_string_is_printable( bstring str ) {
   BOOL is_printable = TRUE;
   int i;
   const char* strdata = bdata( str );

   if( NULL == strdata ) {
      return FALSE;
   }

   for( i = 0 ; blength( str ) > i ; i++ ) {
      if( !scaffold_char_is_printable( strdata[i] ) ) {
         is_printable = FALSE;
         break;
      }
   }

   return is_printable;
}

#include "graphics.h"

static void scaffold_log(
   void* log, const bstring mod_in, enum GRAPHICS_COLOR color,
   const char* message, va_list varg
) {
   int bstr_ret;
   bstring prepend_buffer = NULL;

   if( NULL == scaffold_print_buffer ) {
      scaffold_print_buffer = bfromcstralloc( SCAFFOLD_PRINT_BUFFER_ALLOC, "" );
   }
   scaffold_assert( NULL != scaffold_print_buffer );

   bstr_ret = btrunc( scaffold_print_buffer, 0 );
   scaffold_check_nonzero( bstr_ret );

#ifdef DEBUG
   scaffold_snprintf(
      scaffold_print_buffer, "%b (%b): ",
      mod_in, &(str_scaffold_trace[scaffold_trace_path])
   );
   scaffold_vsnprintf( scaffold_print_buffer, message, varg );
#else
   scaffold_snprintf( scaffold_print_buffer, "%b: ", mod_in );
   scaffold_vsnprintf( scaffold_print_buffer, message, varg );
#endif /* DEBUG */

   scaffold_colorize( scaffold_print_buffer, color );

   fprintf( log, "%s", bdata( scaffold_print_buffer ) );

cleanup:
   return;
}

void scaffold_print_debug( const bstring mod_in, const char* message, ... ) {
#ifdef DEBUG
   va_list varg;
   int bstr_ret;
   SCAFFOLD_COLOR color;
#ifndef HEATSHRINK_DEBUGGING_LOGS
   const char* mod_in_c = NULL;

   mod_in_c = bdata( mod_in );

   /* How's this for an innovative solution to keeping it C89? */
   if( 0 == strncmp( "hs", mod_in_c, 2 ) ) {
      goto cleanup;
   } if( 0 == strncmp( "syncbuff", mod_in_c, 8 ) ) {
      goto cleanup;
   }
#endif /* HEATSHRINK_DEBUGGING_LOGS */

   if( SCAFFOLD_TRACE_SERVER == scaffold_trace_path ) {
      color = SCAFFOLD_COLOR_YELLOW;
   } else {
      color = SCAFFOLD_COLOR_CYAN;
   }

   va_start( varg, message );
   scaffold_log(
      scaffold_log_handle_err, mod_in, color, message, varg
   );
   va_end( varg );
#endif /* DEBUG */
cleanup:
   return;
}

void scaffold_print_debug_color(
   const bstring mod_in, SCAFFOLD_COLOR color, const char* message, ...
) {
#ifdef DEBUG
   va_list varg;
   va_start( varg, message );
   scaffold_log( scaffold_log_handle_err, mod_in, color, message, varg );
   va_end( varg );
#endif /* DEBUG */
cleanup:
   return;
}

void scaffold_print_info( const bstring mod_in, const char* message, ... ) {
   va_list varg;
   int bstr_ret;

   if( NULL == scaffold_print_buffer ) {
      scaffold_print_buffer = bfromcstralloc( SCAFFOLD_PRINT_BUFFER_ALLOC, "" );
   }
   scaffold_assert( NULL != scaffold_print_buffer );

   bstr_ret = btrunc( scaffold_print_buffer, 0 );
   scaffold_check_nonzero( bstr_ret );

   va_start( varg, message );
   scaffold_vsnprintf( scaffold_print_buffer, message, varg );
   va_end( varg );

#ifdef DEBUG
   fprintf( scaffold_log_handle, "%s: %s",
      bdata( mod_in ), bdata( scaffold_print_buffer ) );
#endif /* DEBUG */

#ifdef BACKLOG_PRESENT
   backlog_system( scaffold_print_buffer );
#endif /* BACKLOG_PRESENT */

cleanup:
   return;
}

void scaffold_print_error( const bstring mod_in, const char* message, ... ) {
   va_list varg;
#ifdef DEBUG
   va_start( varg, message );
   scaffold_log(
      scaffold_log_handle_err, mod_in, SCAFFOLD_COLOR_RED, message, varg
   );
   va_end( varg );
#endif /* DEBUG */
}

void scaffold_snprintf( bstring buffer, const char* message, ... ) {
   va_list varg;
   va_start( varg, message );
   scaffold_vsnprintf( buffer, message, varg );
   va_end( varg );
}

void scaffold_vsnprintf( bstring buffer, const char* message, va_list varg ) {
   const char* chariter;
   bstring insert = NULL;
   int bstr_res;
   int i;
   void* p;

   scaffold_error = 0;

   for( chariter = message ; '\0' != *chariter ; chariter++ ) {
      if( '%' != *chariter ) {
         bstr_res = bconchar( buffer, *chariter );
         scaffold_check_nonzero( bstr_res );
         continue;
      }

      switch( *++chariter ) {
      case 'c':
         i = va_arg( varg, int );
         bstr_res = bconchar( buffer, i );
         scaffold_check_nonzero( bstr_res );
         break;

      case 'd':
         i = va_arg( varg, int );
         insert = bformat( "%d", i );
         scaffold_check_null( insert );
         bstr_res = bconcat( buffer, insert );
         bdestroy( insert );
         insert = NULL;
         scaffold_check_nonzero( bstr_res );
         break;

      case 's':
         insert = bfromcstr( va_arg( varg, char*) );
         scaffold_check_null( insert );
         bstr_res = bconcat( buffer, insert );
         bdestroy( insert );
         insert = NULL;
         scaffold_check_nonzero( bstr_res );
         break;

      case 'b':
         insert = va_arg( varg, bstring );
         bstr_res = bconcat( buffer, insert );
         insert = NULL;
         scaffold_check_nonzero( bstr_res );
         break;

      case 'p':
         p = va_arg( varg, void* );
         insert = bformat( "%p", p );
         scaffold_check_null( insert );
         bstr_res = bconcat( buffer, insert );
         bdestroy( insert );
         insert = NULL;
         scaffold_check_nonzero( bstr_res );
         break;

      case '%':
         bstr_res = bconchar( buffer, '%' );
         scaffold_check_nonzero( bstr_res );
         break;
      }
   }

cleanup:
   bdestroy( insert );
   return;
}

void scaffold_random_string( bstring rand_str, SCAFFOLD_SIZE len ) {
   SCAFFOLD_SIZE i;
   int bstr_result;
   bstr_result = btrunc( rand_str, 0 );
   scaffold_check_nonzero( bstr_result );
   for( i = 0; len > i ; i++ ) {
      bstr_result = bconchar(
         rand_str,
         scaffold_random_chars
            [rand() % (int)(sizeof( scaffold_random_chars )-1)]
      );
      scaffold_check_nonzero( bstr_result );
   }
cleanup:
   return;
}

BOOL scaffold_buffer_grow(
   BYTE** buffer, SCAFFOLD_SIZE* len, SCAFFOLD_SIZE new_len
) {
   BYTE* realloc_tmp = NULL;
   BOOL ok = FALSE;

   if( new_len > *len ) {
      realloc_tmp = mem_realloc( *buffer, new_len, BYTE );
      scaffold_check_null( realloc_tmp );
      *len = new_len;
      *buffer = realloc_tmp;
      realloc_tmp = NULL;
   }

   ok = TRUE;

cleanup:
   return ok;
}

#ifdef USE_COLORED_CONSOLE

void scaffold_colorize( bstring str, SCAFFOLD_COLOR color ) {
   bstring str_color = NULL;
   int color_i = (int)color;
   int bstr_ret;
   volatile int mlen;
   volatile int slen;
   volatile const unsigned char* data;

   if( color_i > 6 ) {
      color_i -= 9;
   }
   if( 0 > color_i || 7 <= color_i ) {
      goto cleanup;
   }
   str_color = &(ansi_color_strs[color_i]);

   mlen = str_color->mlen;
   slen = str_color->slen;
   data = str_color->data;

   bstr_ret = binsert( str, 0, str_color, '\0' );
   if( 0 != bstr_ret ) {
      goto cleanup;
   }
   bstr_ret = bconcat( str, &(ansi_color_strs[GRAPHICS_COLOR_WHITE - 9]) );
   if( 0 != bstr_ret ) {
      goto cleanup;
   }

cleanup:
   return;
}
#else

void scaffold_colorize( bstring str, SCAFFOLD_COLOR color ) {
   /* NOOP */
}

#endif /* USE_COLORED_CONSOLE */

int scaffold_strcmp_caseless( const char* s0, const char* s1 ) {
   int difference;
   for( ;; s0++, s1++ ) {
      difference = tolower( *s0 ) - tolower( *s1 );
      if( 0 != difference || !*s0 ) {
         return difference;
      }
   }
}
