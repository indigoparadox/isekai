
#define BACKLOG_C
#include "backlog.h"

#include "vector.h"
#include "ui.h"
#include "callback.h"

static struct VECTOR global_backlog;

static struct tagbstring str_backlog_title = bsStatic( "Log" );
static struct tagbstring str_backlog_id = bsStatic( "backlog" );

void backlog_init() {
   vector_init( &global_backlog );
}

void backlog_shutdown() {
   vector_remove_cb(  &global_backlog, callback_free_backlog, NULL );
   vector_cleanup( &global_backlog );
}

void backlog_ensure_window( struct UI* ui ) {
   struct UI_WINDOW* win = NULL;
   struct UI_CONTROL* control = NULL;

   if( NULL == ui_window_by_id( ui, &str_backlog_id ) ) {
      ui_window_new(
         ui, win, UI_WINDOW_TYPE_BACKLOG, &str_backlog_id,
         &str_backlog_title, NULL,
         10, 400, 260, 70
      );
      ui_control_new(
         ui, control, NULL, UI_CONTROL_TYPE_BACKLOG, FALSE, NULL,
         0, 0, 260, 70 - UI_TITLEBAR_SIZE - UI_WINDOW_MARGIN
      );
      ui_control_add( win, &str_backlog_id, control );
      ui_window_push( ui, win );
   }
cleanup:
   return;
}

void* backlog_iter( vector_search_cb cb, void* arg ) {
   return vector_iterate( &global_backlog, cb, arg );
}

static void backlog_timestamp( struct BACKLOG_LINE* line ) {
   time_t time_now;
   struct tm* time_temp = NULL;

   /* Grab the time in a platform-agnostic way. */
   time( &time_now );
   time_temp = localtime( &time_now );
   memcpy( &(line->time), time_temp, sizeof( struct tm ) );
}

static void backlog_refresh_window() {
   struct UI_WINDOW* bl = NULL;

   /* TODO: A more elegant method of marking the backlog window dirty. */
   bl = ui_window_by_id( ui_get_local(), &str_backlog_id );
   if( NULL != bl ) {
      bl->dirty = TRUE;
   }
}

void backlog_speak( const bstring nick, const bstring msg ) {
   struct BACKLOG_LINE* line = NULL;

   line = (struct BACKLOG_LINE*)calloc(
      1, sizeof( struct BACKLOG_LINE)
   );
   scaffold_check_null( line );

   line->nick = bstrcpy( nick );
   line->line = bstrcpy( msg );

   backlog_timestamp( line );
   vector_insert( &global_backlog, 0, line );
   backlog_refresh_window();

cleanup:
   return;
}

void backlog_system( const bstring msg ) {
   struct BACKLOG_LINE* line = NULL;

   line = (struct BACKLOG_LINE*)calloc(
      1, sizeof( struct BACKLOG_LINE)
   );
   scaffold_check_null( line );

   line->nick = NULL;
   line->line = bstrcpy( msg );

   backlog_timestamp( line );
   vector_insert( &global_backlog, 0, line );
   backlog_refresh_window();

cleanup:
   return;
}
