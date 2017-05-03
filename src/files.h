
#ifndef FILES_H
#define FILES_H

#include "scaffold.h"

SCAFFOLD_SIZE files_read_contents( bstring path, BYTE** buffer, SCAFFOLD_SIZE* len )
#ifdef __GNUC__
__attribute__ ((warn_unused_result))
#endif /* __GNUC__ */
;
SCAFFOLD_SIZE_SIGNED scaffold_write_file( bstring path, BYTE* data, SCAFFOLD_SIZE_SIGNED len, BOOL mkdirs )
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

#ifdef FILES_C

#if defined( _WIN32 ) || defined( WIN16 )
struct tagbstring files_dirsep_string = bsStatic( "\\" );
#else
struct tagbstring files_dirsep_string = bsStatic( "/" );
#endif /* _WIN32 || WIN16 */
#define SCAFFOLD_DIRSEP_CHAR files_dirsep_string.data[0]

SCAFFOLD_MODULE( "files.c" );

#else

extern struct tagbstring files_dirsep_string;

#endif /* FILES_C */

#endif /* FILES_H */
