
#define SCAFFOLD_C
#include "scaffold.h"

#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

struct tagbstring scaffold_empty_string = bsStatic( "" );
struct tagbstring scaffold_space_string = bsStatic( " " );
struct tagbstring scaffold_dirsep_string = bsStatic( "/" );

#define SCAFFOLD_DIRSEP_CHAR scaffold_dirsep_string.data[0]

#ifdef DEBUG
SCAFFOLD_TRACE scaffold_trace_path = SCAFFOLD_TRACE_NONE;
#endif /* DEBUG */

uint8_t scaffold_error = SCAFFOLD_ERROR_NONE;
BOOL scaffold_error_silent = FALSE;

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

void scaffold_read_file_contents( bstring path, BYTE** buffer, size_t* len ) {
   /* TODO: Implement mmap() */
   FILE* inputfile = NULL;

   *buffer = NULL;
   *len = 0;

   inputfile = fopen( bdata( path ), "rb" );
   scaffold_check_null( inputfile );

   /* Allocate enough space to hold the file. */
   fseek( inputfile, 0, SEEK_END );
   *len = ftell( inputfile );
   *buffer = (BYTE*)calloc( *len, sizeof( BYTE ) + 1 ); /* +1 for term. */
   scaffold_check_null( *buffer );
   fseek( inputfile, 0, SEEK_SET );

   /* Read and close the file. */
   fread( *buffer, sizeof( BYTE ), *len, inputfile );

cleanup:
   if( NULL != inputfile ) {
      fclose( inputfile );
   }
}

void scaffold_write_file( bstring path, BYTE* data, size_t len, BOOL mkdirs ) {
   FILE* outputfile = NULL;
   char* path_c = NULL;
   bstring test_path = NULL;
   struct bstrList* path_dirs = NULL;
   size_t true_qty;
   struct stat test_path_stat = { 0 };
   int stat_res;

   assert( NULL != data );
   assert( 0 != len );

   /* Make sure the parent directory exists. */
   path_dirs = bsplit( path, '/' );
   if( 2 > path_dirs->qty ) {
      /* We don't need to create any directories. */
      goto write_file;
   }

   true_qty = path_dirs->qty;

   for( path_dirs->qty = 1 ; path_dirs->qty < true_qty ; path_dirs->qty++ ) {
      test_path = bjoin( path_dirs, &scaffold_dirsep_string );
      scaffold_check_null( test_path );

      path_c = bdata( test_path );
      scaffold_check_null( test_path );

      stat_res = stat( path_c, &test_path_stat );
      if( 0 == (test_path_stat.st_mode & S_IFDIR) ) {
         scaffold_error = SCAFFOLD_ERROR_ZERO;
         scaffold_print_error(
            "Path %s is not a directory. Aborting.\n", path_c
         );
         goto cleanup;
      }

      if( 0 != stat_res  ) {
         /* Directory does not exist, so create it. */
         scaffold_print_info(
            "Creating missing directory: %s\n", path_c
         );
         scaffold_check_nonzero( mkdir( path_c, 0 ) );
      }

      bdestroy( test_path );
      test_path = NULL;
   }

write_file:

   path_c = bdata( path );
   scaffold_check_null( path_c );

   outputfile = fopen( path_c,"wb" );
   scaffold_check_null( outputfile );

   fwrite( data, sizeof( BYTE ), len, outputfile );

cleanup:
   bdestroy( test_path );
   bstrListDestroy( path_dirs );
   if( NULL != outputfile ) {
      fclose( outputfile );
   }
}

void scaffold_list_dir(
   bstring path, struct VECTOR* list, bstring filter, BOOL dir_only, BOOL show_hidden
) {
   bstring child_path = NULL;
   char* path_c = NULL;
   DIR* dir = NULL;
   struct dirent* entry = NULL;

   path_c = bdata( path );
   scaffold_check_null( path_c );

   /* Try to open the directory as a directory. */
   dir = opendir( path_c );
   scaffold_check_silence();
   scaffold_check_null( dir );

   /* This is a valid directory that we can open. */
   while( NULL != (entry = readdir( dir )) ) {
      if( 0 == strcmp( ".", entry->d_name ) || 0 == strcmp( "..", entry->d_name ) ) {
         /* Don't enumerate special directories. */
         continue;
      }

      child_path = bstrcpy( path );
      scaffold_check_unsilence(); /* Hint */
      scaffold_check_null( child_path );

      if( '/' != bchar( child_path, blength( child_path ) - 1 ) ) {
         bconchar( child_path, '/' );
      }
      bcatcstr( child_path, entry->d_name );

      if( DT_DIR != entry->d_type ) {
         if(
            FALSE == dir_only &&
            (FALSE != show_hidden || '.' != entry->d_name[0])
         ) {
            /* We're tracking files, so add this, too. */
            vector_add( list, child_path );
         } else {
            bdestroy( child_path );
         }
         continue;
      } else {
         /* Always add directories. */
         vector_add( list, child_path );
      }

      /* If the child is a directory then go deeper. */
      scaffold_list_dir( child_path, list, filter, dir_only, show_hidden );
   }

cleanup:
   scaffold_check_unsilence();
   if( NULL != dir ) {
      closedir( dir );
   }
   return;
}

bstring scaffold_basename( bstring path ) {
   bstring basename_out = NULL;
   struct bstrList* path_elements = NULL;

   scaffold_check_null( path );

   if( 1 == path_elements->qty ) {
      basename_out = bstrcpy( path_elements->entry[0] );
   }

   path_elements = bsplit( path, SCAFFOLD_DIRSEP_CHAR );
   scaffold_check_null( path_elements );

   basename_out = path_elements->entry[path_elements->qty - 1];

cleanup:
   bstrListDestroy( path_elements );
   return basename_out;
}

void scaffold_join_path( bstring path1, bstring path2 ) {
   int bstr_res = 0;
   if( SCAFFOLD_DIRSEP_CHAR != bchar( path1, blength( path1 ) - 1 ) ) {
      bstr_res = bconchar( path1, SCAFFOLD_DIRSEP_CHAR );
      scaffold_check_nonzero( bstr_res );
   }
   bstr_res = bconcat( path1, path2 );
   scaffold_check_nonzero( bstr_res );
cleanup:
   return;
}
