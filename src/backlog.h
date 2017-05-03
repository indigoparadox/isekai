
#ifndef BACKLOG_H
#define BACKLOG_H

#include "scaffold.h"
#include "ui.h"

#include <time.h>

struct BACKLOG_LINE {
   struct tm time;
   bstring nick;
   bstring line;
};

void backlog_init();
void backlog_shutdown();
void backlog_line_free( struct BACKLOG_LINE* line );
void backlog_ensure_window( struct UI* ui );
void* backlog_iter( vector_search_cb cb, void* arg );
static void backlog_timestamp( struct BACKLOG_LINE* line );
static void backlog_refresh_window();
void backlog_speak( const bstring nick, const bstring msg );
void backlog_system( const bstring msg );

#ifdef BACKLOG_C
SCAFFOLD_MODULE( "backlog.c" );
#endif /* BACKLOG_C */

#endif /* BACKLOG_H */
