
#define SCAFFOLD_C
#include "scaffold.h"

#ifndef DISABLE_BACKLOG
#include "backlog.h"
#endif /* !DISABLE_BACKLOG */

#ifdef SCAFFOLD_LOG_FILE
FILE* scaffold_log_handle = NULL;
FILE* scaffold_log_handle_err = NULL;
#endif /* SCAFFOLD_LOG_FILE */

#include <stdlib.h>

//int8_t scaffold_error = SCAFFOLD_ERROR_NONE;
VBOOL scaffold_error_silent = VFALSE;
VBOOL scaffold_warning_silent = VFALSE;
bstring scaffold_print_buffer = NULL;

static char scaffold_random_chars[] =
   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

VBOOL scaffold_is_numeric( bstring line ) {
   int i;
   VBOOL is_numeric = VTRUE;

   if( NULL == line ) {
      return VFALSE;
   }

   for( i = 0 ; blength( line ) > i ; i++ ) {
      if( !isdigit( bdata( line )[i] ) ) {
         is_numeric = VFALSE;
         break;
      }
   }

   return is_numeric;
}

VBOOL scaffold_string_is_printable( bstring str ) {
   VBOOL is_printable = VTRUE;
   int i;
   const char* strdata = bdata( str );

   if( NULL == strdata ) {
      return VFALSE;
   }

   for( i = 0 ; blength( str ) > i ; i++ ) {
      if( !scaffold_char_is_printable( strdata[i] ) ) {
         is_printable = VFALSE;
         break;
      }
   }

   return is_printable;
}

void scaffold_random_string( bstring rand_str, SCAFFOLD_SIZE len ) {
   SCAFFOLD_SIZE i;
   int bstr_result;
   bstr_result = btrunc( rand_str, 0 );
   lgc_nonzero( bstr_result );
   for( i = 0; len > i ; i++ ) {
      bstr_result = bconchar(
         rand_str,
         scaffold_random_chars
            [rand() % (int)(sizeof( scaffold_random_chars )-1)]
      );
      lgc_nonzero( bstr_result );
   }
cleanup:
   return;
}

int scaffold_strcmp_caseless( const char* s0, const char* s1 ) {
   int difference;
   for( ;; s0++, s1++ ) {
      difference = tolower( *s0 ) - tolower( *s1 );
      if( 0 != difference || !*s0 ) {
         return difference;
      }
   }
}
