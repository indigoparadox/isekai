
#ifndef BACKLOG_H
#define BACKLOG_H

#include "scaffold.h"
#include "ui.h"

#ifdef USE_CLOCK
#include <time.h>
#endif /* USE_CLOCK */

struct BACKLOG_LINE {
#ifdef USE_CLOCK
   struct tm time;
#endif /* USE_CLOCK */
   bstring nick;
   bstring line;
};

void backlog_init();
void backlog_shutdown();
void backlog_line_free( struct BACKLOG_LINE* line );
void backlog_ensure_window( struct UI* ui, GFX_COORD_PIXEL height );
void backlog_close_window( struct UI* ui );
void* backlog_iter( vector_iter_cb cb, void* arg );
void backlog_speak( const bstring nick, const bstring msg );
void backlog_system( const bstring msg );

#ifdef BACKLOG_C
SCAFFOLD_MODULE( "backlog.c" );
#endif /* BACKLOG_C */

#endif /* BACKLOG_H */
