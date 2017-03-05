
#define SCAFFOLD_C
#include "scaffold.h"

#include <stdlib.h>

struct tagbstring scaffold_empty_string = bsStatic( "" );
struct tagbstring scaffold_space_string = bsStatic( " " );

#ifdef DEBUG
SCAFFOLD_TRACE scaffold_trace_path = SCAFFOLD_TRACE_NONE;
#endif /* DEBUG */

uint8_t scaffold_error = SCAFFOLD_ERROR_NONE;

static char scaffold_random_chars[] =
   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

BOOL scaffold_is_numeric( bstring line ) {
   int i;
   BOOL is_numeric = TRUE;

   for( i = 0 ; blength( line ) > i ; i++ ) {
      if( !isdigit( bdata( line )[i] ) ) {
         is_numeric = FALSE;
         break;
      }
   }

   return is_numeric;
}

bstring scaffold_list_pop_string( struct bstrList* list ) {
   bstring popped = list->entry[0];
   int i;

   for( i = 1 ; list->qty > i ; i++ ) {
      list->entry[i - 1] = list->entry[i];
   }

   list->qty--;

   return popped;
}

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

void scaffold_list_append_string_cpy( struct bstrList* list, bstring str ) {
   int bstr_result;

   assert( NULL != list );
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

BOOL scaffold_string_is_printable( bstring str ) {
   BOOL is_printable = TRUE;
   int i;
   const char* strdata = bdata( str );

   for( i = 0 ; blength( str ) > i ; i++ ) {
      if( !scaffold_char_is_printable( strdata[i] ) ) {
         is_printable = FALSE;
         break;
      }
   }

   return is_printable;
}

#if 0
#ifdef DEBUG
void scaffold_printf_debug( const char* message, ... ) {
   bstring buffer = NULL;
   va_list varg;

   buffer = bfromcstralloc( strlen( message ), "" );
   scaffold_check_null( buffer );

   va_start( varg, message );
   scaffold_snprintf( buffer, message, varg );
   va_end( varg );

   //if( 0 == scaffold_error ) {
   //   scaffold_printf_debug( "%s", bdata( buffer ) );
   //}

cleanup:
   bdestroy( buffer );
   return;
}
#endif /* DEBUG */
#endif

void scaffold_snprintf( bstring buffer, const char* message, va_list varg ) {
   const char* chariter;
   bstring insert = NULL;
   int bstr_res;
   int i;

   scaffold_error = 0;

   for( chariter = message ; '\0' != *chariter ; chariter++ ) {
      if( '%' != *chariter ) {
         bconchar( buffer, *chariter );
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

void scaffold_random_string( bstring rand_str, size_t len ) {
   size_t i;
   btrunc( rand_str, 0 );
   for( i = 0; len > i ; i++ ) {
      bconchar(
         rand_str,
         scaffold_random_chars
            [rand() % (int)(sizeof( scaffold_random_chars )-1)]
      );
   }
}
