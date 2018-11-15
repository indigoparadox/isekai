
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

bstring files_read_contents_b( const bstring path ) {
   BYTE* buffer = NULL;
   size_t len = 0;
   bstring out = NULL;

   files_read_contents( path, &buffer, &len );

   out = bfromcstr( buffer );

   mem_free( buffer );

   return out;
}

/** \brief Provide a block of memory that contains a given file's contents.
 *         May be pulled from weird special storage/ROM or mmap'ed.
 * \param[in] path   The path to identify the file to open.
 * \param[in] buffer A pointer to a pointer to NULL. Will return the buffer.
 * \param[in] len    A pointer to the size indicator for the buffer.
 * \return The number of bytes read, or -1 on failure.
 */
size_t files_read_contents(
   const bstring path, BYTE** buffer, size_t* len
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
   size_t sz_out = -1;
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

   lgc_null( path );
   path_c = bdata( path );
   lg_debug( __FILE__, "Reading from path: %s\n", path_c );

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
      goto cleanup;
   }
   *len = inputstat.st_size;
#endif /* _WIN32 || _WIN16 */
   *buffer = mem_alloc( *len, BYTE );
   lgc_null( *buffer );

   /* Read and close the file. */
#if defined( _WIN32 ) || defined( WIN16 )
   ReadFile( inputfd, *buffer, *len, &sz_out,  NULL );
#else
   sz_out = read( inputfd, *buffer, *len );
#endif /* _WIN32 || WIN16 */
   lgc_zero( sz_out, bdata( zero_error ) );
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

static void* cb_files_concat_dirs( size_t idx, void* iter, void* arg ) {
   bstring str_out = (bstring)arg;
   bstring str_cat = (bstring)iter;
   int bstr_res = 0;

   bstr_res = bconcat( str_out, str_cat );
   lgc_nonzero( bstr_res );
   bstr_res = bconchar( str_out, '/' );
   lgc_nonzero( bstr_res );

cleanup:
   return NULL;
}

SCAFFOLD_SIZE_SIGNED files_write(
   bstring path, BYTE* data, SCAFFOLD_SIZE_SIGNED len, BOOL mkdirs
) {
   FILE* outputfile = NULL;
   char* path_c = NULL;
   bstring test_path = NULL;
   struct VECTOR* path_dirs = NULL;
   struct stat test_path_stat = { 0 };
   int stat_res;
   SCAFFOLD_SIZE_SIGNED sz_out = -1;
   bstring zero_error = NULL;
   size_t dir_count = 0;
   int res = 0;
   size_t p_count = 0;

   scaffold_assert( NULL != data );
   scaffold_assert( 0 != len );

   zero_error = bformat( "Zero bytes written to: %s", bdata( path ) );

   /* Make sure the parent directory exists. */
   path_dirs = bgsplit( path, '/' );
   if( 2 > vector_count( path_dirs ) ) {
      /* We don't need to create any directories. */
      goto write_file;
   }

   /* Process every subset of the path (dir1, dir1/dir2, dir1/dir2/file) until
    * we hit the final path to make sure that each parent directory exists,
    * creating them if they do not.
    */
   dir_count = vector_count( path_dirs );
   for( p_count = 1 ; p_count < dir_count ; p_count++ ) {
      test_path = bfromcstr( "" );
      lgc_null( test_path );
      vector_iterate_i( path_dirs, cb_files_concat_dirs, test_path, p_count );
      lgc_null( test_path );

      path_c = bdata( test_path );
      lgc_null( test_path );

#ifndef DJGPP
      stat_res = stat( path_c, &test_path_stat );
      if( 0 == (test_path_stat.st_mode & S_IFDIR) ) {
         lgc_error = LGC_ERROR_ZERO;
         lg_error(
            __FILE__, "Path %s is not a directory. Aborting.\n", path_c
         );
         goto cleanup;
      }
#endif // _DJGPP

      if( 0 != stat_res  ) {
         /* Directory does not exist, so create it. */
         lg_debug(
            __FILE__, "Creating missing directory: %s\n", path_c
         );
#ifdef _WIN32
         CreateDirectory( path_c, NULL );
#else
         res = mkdir( path_c, 0777 );
         lgc_nonzero( res );
#endif /* _WIN32 */
      }

      bdestroy( test_path );
      test_path = NULL;
   }

write_file:

   path_c = bdata( path );
   lgc_null( path_c );

   outputfile = fopen( path_c,"wb" );
   lgc_null( outputfile );

   sz_out = fwrite( data, sizeof( BYTE ), len, outputfile );
   lgc_zero( sz_out, bdata( zero_error ) );
   scaffold_assert( sz_out == len );

cleanup:
   bdestroy( zero_error );
   bdestroy( test_path );
   vector_remove_cb( path_dirs, callback_v_free_strings, NULL );
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

#ifndef DJGPP

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
   lgc_null( path_c );

   /* Try to open the directory as a directory. */
   dir = opendir( path_c );
   lgc_silence();
   lgc_null( dir );

   lg_debug( __FILE__, "Listing directory: %b\n", path );

   /* XXX: Not reading directories? */

   /* This is a valid directory that we can open. */
   while( NULL != (entry = readdir( dir )) ) {
      if( 0 == strcmp( ".", entry->d_name ) || 0 == strcmp( "..", entry->d_name ) ) {
         /* Don't enumerate special directories. */
         continue;
      }

      child_path = bstrcpy( path );
      lgc_unsilence(); /* Hint */
      lgc_null( child_path );

      if( '/' != bchar( child_path, blength( child_path ) - 1 ) ) {
         bstr_result = bconchar( child_path, '/' );
         lgc_nonzero( bstr_result );
      }
      bstr_result = bcatcstr( child_path, entry->d_name );
      lgc_nonzero( bstr_result );

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
         lg_debug( __FILE__, "Found file: %b\n", child_path );
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
         lg_debug( __FILE__, "Found directory: %b\n", child_path );

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
   lgc_unsilence();
   if( NULL != dir ) {
      closedir( dir );
   }
   return;
#endif /* WIN16 */
#endif // DJGPP
}

BOOL files_check_directory( const bstring path ) {
   struct stat dir_info = { 0 };
   char* path_c = NULL;
   bstring zero_error = NULL;

   scaffold_assert( NULL != path );

   zero_error = bformat( "Not a directory: %s", bdata( path ) );

   lgc_error = 0;
   lgc_silence();

   path_c = bdata( path );
   scaffold_assert( NULL != path_c );
#ifndef DJGPP
   lgc_nonzero( stat( path_c, &dir_info ) );
   lgc_zero( (dir_info.st_mode & S_IFDIR), bdata( zero_error ) );
#endif // DJGPP

cleanup:
   bdestroy( zero_error );
   lgc_unsilence();
   switch( lgc_error ) {
   case LGC_ERROR_NONZERO:
      lg_error(
         __FILE__, "Unable to open directory: %s\n", bdata( path ) );
      return FALSE;

   case LGC_ERROR_ZERO:
      lg_error( __FILE__, "Not a directory: %s\n", bdata( path ) );
      return FALSE;

   default:
      return TRUE;
   }
}

bstring files_basename( bstring path ) {
   bstring basename_out = NULL;
   struct VECTOR* path_elements = NULL;

   lgc_null( path );

   path_elements = bgsplit( path, SCAFFOLD_DIRSEP_CHAR );
   lgc_null( path_elements );

   if( 1 == vector_count( path_elements ) ) {
      basename_out = bstrcpy( vector_get( path_elements, 0 ) );
   }

   basename_out = vector_get( path_elements, vector_count( path_elements ) - 1 );

cleanup:
   vector_remove_cb( path_elements, callback_v_free_strings, NULL );
   vector_free( &path_elements );
   return basename_out;
}

void files_join_path( bstring path1, const bstring path2 ) {
   int bstr_res = 0;
   if( SCAFFOLD_DIRSEP_CHAR != bchar( path1, blength( path1 ) - 1 ) ) {
      bstr_res = bconchar( path1, SCAFFOLD_DIRSEP_CHAR );
      assert( 0 == bstr_res );
      lgc_nonzero( bstr_res );
   }
   bstr_res = bconcat( path1, path2 );
   assert( 0 == bstr_res );
   lgc_nonzero( bstr_res );
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
   lgc_null_msg( path_out, "Could not allocate path buffer." );
   files_join_path( path_out, append );
   lgc_nonzero( lgc_error );

cleanup:
   return path_out;
}

static void* files_search_cb( size_t idx, void* iter, void* arg ) {
   bstring file_iter = (bstring)iter,
      file_iter_short = NULL,
      file_search = (bstring)arg;

   file_iter_short = bmidstr(
      file_iter,
      str_server_data_path.slen + 1,
      blength( file_iter ) - str_server_data_path.slen - 1
   );
   lgc_null( file_iter_short );

   if( 0 != bstrcmp( file_iter_short, file_search ) ) {
      /* FIXME: If the files request and one of the files present start    *
       * with similar names (tilelist.png.tmx requested, tilelist.png      *
       * exists, eg), then weird stuff happens.                            */
      /* FIXME: Don't try to send directories. */
      lg_debug( __FILE__, "Server: Filename Cmp: Iter: %s (%s) Looking for: %s \n", bdata( file_iter ), bdata( file_iter_short ), bdata( file_search ) );
      bdestroy( file_iter_short );
      file_iter_short = NULL;
   } else {
      lg_debug( __FILE__, "Server: File Found: %s\n", bdata( file_iter ) );
   }

cleanup:
   return file_iter_short;
}

bstring files_search( bstring search_filename ) {
   struct VECTOR* files = NULL;
   bstring path_out = NULL;

   files = vector_new();
   files_list_dir( files_root( NULL ), files, NULL, FALSE, FALSE );
   path_out = vector_iterate( files, files_search_cb, search_filename );

cleanup:
   if( NULL != files ) {
      vector_remove_cb( files, callback_v_free_strings, NULL );
   }
   vector_cleanup( files );
   mem_free( files );
   return path_out;
}
