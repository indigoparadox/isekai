
#ifndef FILES_H
#define FILES_H

#include "scaffold.h"

bstring files_read_contents_b( const bstring path );
size_t files_read_contents(
   const bstring path, BYTE** buffer, size_t* len
)
#ifdef USE_GNUC_EXTENSIONS
__attribute__ ((warn_unused_result))
#endif /* USE_GNUC_EXTENSIONS */
;
SCAFFOLD_SIZE_SIGNED files_write(
   bstring path, BYTE* data, SCAFFOLD_SIZE_SIGNED len, BOOL mkdirs
)
#ifdef USE_GNUC_EXTENSIONS
__attribute__ ((warn_unused_result))
#endif /* USE_GNUC_EXTENSIONS */
;
void files_list_dir(
   const bstring path, struct VECTOR* list, const bstring filter,
   BOOL dir_only, BOOL show_hidden
);
bstring files_basename( bstring path )
#ifdef USE_GNUC_EXTENSIONS
__attribute__ ((warn_unused_result))
#endif /* USE_GNUC_EXTENSIONS */
;
BOOL files_check_directory( const bstring path )
#ifdef USE_GNUC_EXTENSIONS
__attribute__ ((warn_unused_result))
#endif /* USE_GNUC_EXTENSIONS */
;
void files_join_path( bstring path1, const bstring path2 );
bstring files_root( bstring append );
bstring files_search( bstring search_filename );

#ifdef FILES_C

#if defined( _WIN32 ) || defined( WIN16 )
struct tagbstring files_dirsep_string = bsStatic( "\\" );
#else
struct tagbstring files_dirsep_string = bsStatic( "/" );
#endif /* _WIN32 || WIN16 */
#define SCAFFOLD_DIRSEP_CHAR files_dirsep_string.data[0]

#else

extern struct tagbstring files_dirsep_string;

#endif /* FILES_C */

#endif /* FILES_H */
