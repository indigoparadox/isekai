
#define SCAFFOLD_C
#include "scaffold.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef _WIN32
#include "wdirent.h"
#else
#include <dirent.h>
#endif /* _WIN32 */

struct tagbstring scaffold_empty_string = bsStatic( "" );
struct tagbstring scaffold_space_string = bsStatic( " " );
struct tagbstring scaffold_dirsep_string = bsStatic( "/" );

#define SCAFFOLD_DIRSEP_CHAR scaffold_dirsep_string.data[0]

#ifdef DEBUG
SCAFFOLD_TRACE scaffold_trace_path = SCAFFOLD_TRACE_NONE;
#endif /* DEBUG */

#ifdef SCAFFOLD_LOG_FILE
FILE* scaffold_log_handle = NULL;
FILE* scaffold_log_handle_err = NULL;
#endif /* SCAFFOLD_LOG_FILE */

int8_t scaffold_error = SCAFFOLD_ERROR_NONE;
BOOL scaffold_error_silent = FALSE;

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

/** \brief Provide a block of memory that contains a given file's contents.
 *         May be pulled from weird special storage/ROM or mmap'ed.
 *
 * \param path - The path to identify the file to open.
 * \param buffer - A pointer to a pointer to NULL. Will return the buffer.
 * \param len - A pointer to the size indicator for the buffer.
 * \return The number of bytes read, or -1 on failure.
 *
 */

SCAFFOLD_SIZE_SIGNED scaffold_read_file_contents( bstring path, BYTE** buffer, SCAFFOLD_SIZE* len ) {
   int inputfd = -1;
   SCAFFOLD_SIZE_SIGNED sz_out = -1;
   SCAFFOLD_SIZE_SIGNED sz_last = -1;
   struct stat inputstat;
#ifdef DEBUG
   char* path_c = NULL;
#endif /* DEBUG */

   if( NULL != *buffer ) {
      free( *buffer );
   }

   *buffer = NULL;
   *len = 0;

   /* TODO: Implement mmap() */

   scaffold_check_null( path );
#ifdef DEBUG
   path_c = bdata( path );
   scaffold_print_debug( "Reading from path: %s\n", path_c );
#endif /* DEBUG */

   inputfd = open( bdata( path ), O_RDONLY );
   if( 0 > inputfd ) {
      scaffold_error = SCAFFOLD_ERROR_OUTOFBOUNDS;
      goto cleanup;
   }

   /* Allocate enough space to hold the file. */
   if( 0 != fstat( inputfd, &inputstat ) || !S_ISREG( inputstat.st_mode ) ) {
      scaffold_error = SCAFFOLD_ERROR_OUTOFBOUNDS;
      goto cleanup;
   }
   *len = inputstat.st_size;
   *buffer = (BYTE*)calloc( *len, sizeof( BYTE ) );
   scaffold_check_null( *buffer );

   /* Read and close the file. */
   sz_out = read( inputfd, *buffer, *len );
   scaffold_check_zero( sz_out );
   scaffold_assert( sz_out == *len );

cleanup:
   if( 0 <= inputfd ) {
      close( inputfd );
   }
   return sz_out;
}

SCAFFOLD_SIZE_SIGNED scaffold_write_file( bstring path, BYTE* data, SCAFFOLD_SIZE len, BOOL mkdirs ) {
   FILE* outputfile = NULL;
   char* path_c = NULL;
   bstring test_path = NULL;
   struct bstrList* path_dirs = NULL;
   SCAFFOLD_SIZE true_qty;
   struct stat test_path_stat = { 0 };
   int stat_res;
   SCAFFOLD_SIZE_SIGNED sz_out = -1;

   scaffold_assert( NULL != data );
   scaffold_assert( 0 != len );

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
#ifdef WIN32
         CreateDirectory( path_c, NULL );
#else
         scaffold_check_nonzero( mkdir( path_c, 0 ) );
#endif // WIN32
      }

      bdestroy( test_path );
      test_path = NULL;
   }

write_file:

   path_c = bdata( path );
   scaffold_check_null( path_c );

   outputfile = fopen( path_c,"wb" );
   scaffold_check_null( outputfile );

   sz_out = fwrite( data, sizeof( BYTE ), len, outputfile );
   scaffold_check_zero( sz_out );
   scaffold_assert( sz_out == len );

cleanup:
   bdestroy( test_path );
   bstrListDestroy( path_dirs );
   if( NULL != outputfile ) {
      fclose( outputfile );
   }
   return sz_out;
}

void scaffold_list_dir(
   const bstring path, struct VECTOR* list, const bstring filter,
   BOOL dir_only, BOOL show_hidden
) {
   bstring child_path = NULL;
   char* path_c = NULL;
   DIR* dir = NULL;
   struct dirent* entry = NULL;
   int bstr_result;

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
         bstr_result = bconchar( child_path, '/' );
         scaffold_check_nonzero( bstr_result );
      }
      bstr_result = bcatcstr( child_path, entry->d_name );
      scaffold_check_nonzero( bstr_result );

      /* FIXME: Detect directories under Windows. */
#ifndef _WIN32
      if( DT_DIR != entry->d_type ) {
#endif /* _WIN32 */
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
#ifndef _WIN32
      } else {
         /* Always add directories. */
         vector_add( list, child_path );
      }
#endif /* _WIN32 */

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

BOOL scaffold_check_directory( const bstring path ) {
   struct stat dir_info = { 0 };
   char* path_c = NULL;

   scaffold_assert( NULL != path );

   scaffold_error = 0;
   scaffold_check_silence();

   path_c = bdata( path );
   scaffold_assert( NULL != path_c );
   scaffold_check_nonzero( stat( path_c, &dir_info ) );
   scaffold_check_zero( (dir_info.st_mode & S_IFDIR) );

cleanup:

   scaffold_check_unsilence();
   switch( scaffold_error ) {
   case SCAFFOLD_ERROR_NONZERO:
      scaffold_print_error( "Unable to open directory: %s\n", bdata( path ) );
      return FALSE;

   case SCAFFOLD_ERROR_ZERO:
      scaffold_print_error( "Not a directory: %s\n", bdata( path ) );
      return FALSE;

   default:
      return TRUE;
   }
}

bstring scaffold_basename( bstring path ) {
   bstring basename_out = NULL;
   struct bstrList* path_elements = NULL;

   scaffold_check_null( path );

   path_elements = bsplit( path, SCAFFOLD_DIRSEP_CHAR );
   scaffold_check_null( path_elements );

   if( 1 == path_elements->qty ) {
      basename_out = bstrcpy( path_elements->entry[0] );
   }

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
