
#define FILES_C
#include "files.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

static struct tagbstring str_server_data_path =
   bsStatic( "testdata/server" );

#ifdef _WIN32
#include "wdirent.h"
#elif defined( __linux )
#include <dirent.h>
#include <unistd.h>
#endif /* _WIN32 || __linux */

#include "callback.h"

/** \brief Provide a block of memory that contains a given file's contents.
 *         May be pulled from weird special storage/ROM or mmap'ed.
 * \param[in] path   The path to identify the file to open.
 * \param[in] buffer A pointer to a pointer to NULL. Will return the buffer.
 * \param[in] len    A pointer to the size indicator for the buffer.
 * \return The number of bytes read, or -1 on failure.
 */
SCAFFOLD_SIZE files_read_contents(
   bstring path, BYTE** buffer, SCAFFOLD_SIZE* len
) {
   struct stat inputstat;
   char* path_c = NULL;
#if defined( _WIN32 )
   DWORD sz_out = -1;
   LARGE_INTEGER sz_win;
   HANDLE inputfd = NULL;
#elif defined( WIN16 )
   SCAFFOLD_SIZE_SIGNED sz_out = -1;
   int sz_win;
   HFILE inputfd = NULL;
#else
   SCAFFOLD_SIZE sz_out = -1;
   int inputfd = -1;
#endif /* _WIN32 */
   bstring zero_error = NULL;

   if( NULL != *buffer ) {
      mem_free( *buffer );
   }

   *buffer = NULL;
   *len = 0;

   zero_error = bformat( "Zero bytes read from file: %s", bdata( path ) );

   /* TODO: Implement mmap() */

   scaffold_check_null( path );
   path_c = bdata( path );
   scaffold_print_debug( &module, "Reading from path: %s\n", path_c );

#if defined( _WIN32 )
   inputfd = CreateFile(
      bdata( path ), GENERIC_READ, 0, NULL,
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL
   );
   if( NULL == inputfd ) {
#elif defined( WIN16 )
   inputfd = OpenFile( bdata( path ), NULL, OF_READ );
#else
   inputfd = open( path_c, O_RDONLY );
   if( 0 > inputfd ) {
#endif /* _WIN32 || _WIN16 */
      scaffold_error = SCAFFOLD_ERROR_OUTOFBOUNDS;
      goto cleanup;
   }

   /* Allocate enough space to hold the file. */
#if defined( _WIN32 )
   GetFileSizeEx( inputfd, &sz_win );
   *len = sz_win.LowPart;
#elif defined( WIN16 )
   sz_win = GetFileSize( inputfd, NULL );
#else
   if( 0 != fstat( inputfd, &inputstat ) || !S_ISREG( inputstat.st_mode ) ) {
      scaffold_error = SCAFFOLD_ERROR_OUTOFBOUNDS;
      goto cleanup;
   }
   *len = inputstat.st_size;
#endif /* _WIN32 || _WIN16 */
   *buffer = mem_alloc( *len, BYTE );
   scaffold_check_null( *buffer );

   /* Read and close the file. */
#if defined( _WIN32 ) || defined( WIN16 )
   ReadFile( inputfd, *buffer, *len, &sz_out,  NULL );
#else
   sz_out = read( inputfd, *buffer, *len );
#endif /* _WIN32 || WIN16 */
   scaffold_check_zero( sz_out, bdata( zero_error ) );
   scaffold_assert( sz_out == *len );

cleanup:
   bdestroy( zero_error );
#if defined( _WIN32 ) || defined( WIN16 )
   CloseHandle( inputfd );
#else
   if( 0 <= inputfd ) {
      close( inputfd );
   }
#endif /* _WIN32 || WIN16 */
   return sz_out;
}

SCAFFOLD_SIZE_SIGNED files_write(
   bstring path, BYTE* data, SCAFFOLD_SIZE_SIGNED len, BOOL mkdirs
) {
   FILE* outputfile = NULL;
   char* path_c = NULL;
   bstring test_path = NULL;
   struct VECTOR* path_dirs = NULL;
   SCAFFOLD_SIZE true_qty;
   struct stat test_path_stat = { 0 };
   int stat_res;
   SCAFFOLD_SIZE_SIGNED sz_out = -1;
   bstring zero_error = NULL;

   scaffold_assert( NULL != data );
   scaffold_assert( 0 != len );

   zero_error = bformat( "Zero bytes written to: %s", bdata( path ) );

   /* Make sure the parent directory exists. */
   path_dirs = bgsplit( path, '/' );
   if( 2 > vector_count( path_dirs ) ) {
      /* We don't need to create any directories. */
      goto write_file;
   }

   true_qty = vector_count( path_dirs );

   for( path_dirs->count = 1 ; path_dirs->count < true_qty ; path_dirs->count++ ) {
      test_path = bfromcstr( "" );
      scaffold_check_null( test_path );
      vector_iterate( path_dirs, callback_concat_strings, test_path );
      scaffold_check_null( test_path );

      path_c = bdata( test_path );
      scaffold_check_null( test_path );

      stat_res = stat( path_c, &test_path_stat );
      if( 0 == (test_path_stat.st_mode & S_IFDIR) ) {
         scaffold_error = SCAFFOLD_ERROR_ZERO;
         scaffold_print_error(
            &module, "Path %s is not a directory. Aborting.\n", path_c
         );
         goto cleanup;
      }

      if( 0 != stat_res  ) {
         /* Directory does not exist, so create it. */
         scaffold_print_debug(
            &module, "Creating missing directory: %s\n", path_c
         );
#ifdef _WIN32
         CreateDirectory( path_c, NULL );
#else
         scaffold_check_nonzero( mkdir( path_c, 0 ) );
#endif /* _WIN32 */
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
   scaffold_check_zero( sz_out, bdata( zero_error ) );
   scaffold_assert( sz_out == len );

cleanup:
   bdestroy( zero_error );
   bdestroy( test_path );
   vector_remove_cb( path_dirs, callback_free_strings, NULL );
   vector_free( &path_dirs );
   if( NULL != outputfile ) {
      fclose( outputfile );
   }
   return sz_out;
}

void files_list_dir(
   const bstring path, struct VECTOR* list, const bstring filter,
   BOOL dir_only, BOOL show_hidden
) {

   /* FIXME: Detect directories under WIN16 or DOS. */

#ifndef WIN16

   bstring child_path = NULL;
   char* path_c = NULL;
   DIR* dir = NULL;
   struct dirent* entry = NULL;
   int bstr_result;
   SCAFFOLD_SIZE_SIGNED verr;
   struct stat stat_buff;
   BOOL is_dir = FALSE;
   int stat_result;

   path_c = bdata( path );
   scaffold_check_null( path_c );

   /* Try to open the directory as a directory. */
   dir = opendir( path_c );
   scaffold_check_silence();
   scaffold_check_null( dir );

   scaffold_print_debug( &module, "Listing directory: %b\n", path );

   /* XXX: Not reading directories? */

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

#ifdef _DIRENT_HAVE_D_TYPE
      if( DT_DIR == entry->d_type ) {
         /* Directory. */
         is_dir = TRUE;
      } else if( DT_UNKNOWN != entry->d_type ) {
         /* Not a directory. */
         is_dir = FALSE;
      } else {
#endif /* _DIRENT_HAVE_D_TYPE */
         /* No dirent, so do this the hard way. */
         stat_result = stat( bdata( child_path ), &stat_buff );
         /* TODO: Check stat_result. */
         is_dir = S_ISDIR( stat_buff.st_mode );
#ifdef _DIRENT_HAVE_D_TYPE
      }
#endif /* _DIRENT_HAVE_D_TYPE */

      if( FALSE == is_dir ) {
         scaffold_print_debug( &module, "Found file: %b\n", child_path );
#endif /* _WIN32 */
         if(
            FALSE == dir_only &&
            (FALSE != show_hidden || '.' != entry->d_name[0])
         ) {
            /* We're tracking files, so add this, too. */
            verr = vector_add( list, child_path );
            if( 0 > verr ) {
               bdestroy( child_path );
            }
         } else {
            bdestroy( child_path );
         }
         continue;
#ifndef _WIN32
      } else {
         scaffold_print_debug( &module, "Found directory: %b\n", child_path );

         /* If the child is a directory then go deeper. */
         files_list_dir( child_path, list, filter, dir_only, show_hidden );

         /* Always add directories. */
         verr = vector_add( list, child_path );
         if( 0 > verr ) {
            bdestroy( child_path );
         }
      }
#endif /* _WIN32 */
   }

cleanup:
   scaffold_check_unsilence();
   if( NULL != dir ) {
      closedir( dir );
   }
   return;
#endif /* WIN16 */
}

BOOL files_check_directory( const bstring path ) {
   struct stat dir_info = { 0 };
   char* path_c = NULL;
   bstring zero_error = NULL;

   scaffold_assert( NULL != path );

   zero_error = bformat( "Not a directory: %s", bdata( path ) );

   scaffold_error = 0;
   scaffold_check_silence();

   path_c = bdata( path );
   scaffold_assert( NULL != path_c );
   scaffold_check_nonzero( stat( path_c, &dir_info ) );
   scaffold_check_zero( (dir_info.st_mode & S_IFDIR), bdata( zero_error ) );

cleanup:
   bdestroy( zero_error );
   scaffold_check_unsilence();
   switch( scaffold_error ) {
   case SCAFFOLD_ERROR_NONZERO:
      scaffold_print_error(
         &module, "Unable to open directory: %s\n", bdata( path ) );
      return FALSE;

   case SCAFFOLD_ERROR_ZERO:
      scaffold_print_error( &module, "Not a directory: %s\n", bdata( path ) );
      return FALSE;

   default:
      return TRUE;
   }
}

bstring files_basename( bstring path ) {
   bstring basename_out = NULL;
   struct VECTOR* path_elements = NULL;

   scaffold_check_null( path );

   path_elements = bgsplit( path, SCAFFOLD_DIRSEP_CHAR );
   scaffold_check_null( path_elements );

   if( 1 == vector_count( path_elements ) ) {
      basename_out = bstrcpy( vector_get( path_elements, 0 ) );
   }

   basename_out = vector_get( path_elements, vector_count( path_elements ) - 1 );

cleanup:
   vector_remove_cb( path_elements, callback_free_strings, NULL );
   vector_free( &path_elements );
   return basename_out;
}

void files_join_path( bstring path1, const bstring path2 ) {
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

bstring files_root( const bstring append ) {
   bstring path_out = NULL;

   if( NULL == append ) {
      path_out = &str_server_data_path;
      goto cleanup;
   }

   path_out = bstrcpy( &str_server_data_path );
   scaffold_check_null_msg( path_out, "Could not allocate path buffer." );
   files_join_path( path_out, append );
   scaffold_check_nonzero( scaffold_error );

cleanup:
   return path_out;
}

static void* files_search_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   bstring file_iter = (bstring)iter,
      file_iter_short = NULL,
      file_search = (bstring)arg;

   file_iter_short = bmidstr(
      file_iter,
      str_server_data_path.slen + 1,
      blength( file_iter ) - str_server_data_path.slen - 1
   );
   scaffold_check_null( file_iter_short );

   if( 0 != bstrcmp( file_iter_short, file_search ) ) {
      /* FIXME: If the files request and one of the files present start    *
       * with similar names (tilelist.png.tmx requested, tilelist.png      *
       * exists, eg), then weird stuff happens.                            */
      /* FIXME: Don't try to send directories. */
      scaffold_print_debug( &module, "Server: Filename Cmp: Iter: %s (%s) Looking for: %s \n", bdata( file_iter ), bdata( file_iter_short ), bdata( file_search ) );
      bdestroy( file_iter_short );
      file_iter_short = NULL;
   } else {
      scaffold_print_debug( &module, "Server: File Found: %s\n", bdata( file_iter ) );
   }

cleanup:
   return file_iter_short;
}

bstring files_search( bstring search_filename ) {
   struct VECTOR* files = NULL;
   bstring path_out = NULL;

   vector_new( files );
   files_list_dir( files_root( NULL ), files, NULL, FALSE, FALSE );
   path_out = vector_iterate( files, files_search_cb, search_filename );

cleanup:
   if( NULL != files ) {
      vector_remove_cb( files, callback_free_strings, NULL );
   }
   vector_cleanup( files );
   mem_free( files );
   return path_out;
}
