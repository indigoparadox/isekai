
#define BACKLOG_C
#include "backlog.h"

#include "callback.h"

static struct VECTOR* global_backlog;

static struct tagbstring str_backlog_title = bsStatic( "Log" );
static struct tagbstring str_backlog_id = bsStatic( "backlog" );

void backlog_init() {
   global_backlog = vector_new();
   lg_set_info_cb( backlog_system );
}

void backlog_shutdown() {
   vector_remove_cb( global_backlog, callback_free_backlog, NULL );
   vector_free( &global_backlog );
}

void backlog_line_free( struct BACKLOG_LINE* line ) {
   bdestroy( line->line );
   bdestroy( line->nick );
   mem_free( line );
}

void backlog_ensure_window( struct UI* ui, GFX_COORD_PIXEL height ) {
   struct UI_WINDOW* win = NULL;
   struct UI_CONTROL* control = NULL;

   scaffold_assert( 0 < height );

   win = ui_window_by_id( ui, &str_backlog_id );
   if( NULL == win ) {
      lg_debug(
         __FILE__, "Creating backlog window, height: %d\n", height
      );
      ui_window_new(
         ui, win, &str_backlog_id,
         &str_backlog_title, NULL,
         0, GRAPHICS_SCREEN_HEIGHT - height, 460, height
      );
      ui_control_new(
         ui, control, NULL, UI_CONTROL_TYPE_BACKLOG, VFALSE, VFALSE, NULL,
         0, 0, 260, 70 - UI_TITLEBAR_SIZE - UI_WINDOW_MARGIN
      );
      ui_control_add( win, &str_backlog_id, control );
      ui_window_push( ui, win );
   }
cleanup:
   return;
}

void backlog_close_window( struct UI* ui ) {
   struct UI_WINDOW* win = NULL;
   win = ui_window_by_id( ui, &str_backlog_id );
   if( NULL != win ) {
      lg_debug( __FILE__, "Closing backlog window.\n" );
      ui_window_destroy( ui, &str_backlog_id );
   }
   return;
}

void* backlog_iter( vector_iter_cb cb, void* arg ) {
   return vector_iterate( global_backlog, cb, arg );
}

#ifdef USE_CLOCK

static void backlog_timestamp( struct BACKLOG_LINE* line ) {
   time_t time_now;
   struct tm* time_temp = NULL;

   /* Grab the time in a platform-agnostic way. */
   time( &time_now );
   time_temp = localtime( &time_now );
   memcpy( &(line->time), time_temp, sizeof( struct tm ) );
}

#endif /* USE_CLOCK */

static void backlog_refresh_window() {
   struct UI_WINDOW* bl = NULL;

   /* TODO: A more elegant method of marking the backlog window dirty. */
   bl = ui_window_by_id( ui_get_local(), &str_backlog_id );
   if( NULL != bl ) {
      bl->dirty = VTRUE;
   }
}

void backlog_speak( const bstring nick, const bstring msg ) {
   struct BACKLOG_LINE* line = NULL;
   SCAFFOLD_SIZE_SIGNED verr;

   line = mem_alloc( 1, struct BACKLOG_LINE );
   lgc_null( line );

   line->nick = bstrcpy( nick );
   line->line = bstrcpy( msg );

   backlog_timestamp( line );
   verr = vector_insert( global_backlog, 0, line );
   if( 0 > verr ) {
      backlog_line_free( line );
      goto cleanup;
   }
   backlog_refresh_window();

cleanup:
   return;
}

void backlog_system( const bstring msg ) {
   struct BACKLOG_LINE* line = NULL;
   SCAFFOLD_SIZE_SIGNED verr;

   line = mem_alloc( 1, struct BACKLOG_LINE );
   lgc_null( line );

   line->nick = NULL;
   line->line = bstrcpy( msg );

   backlog_timestamp( line );
   verr = vector_insert( global_backlog, 0, line );
   if( 0 > verr ) {
      /* Check below will destroy leftover object. */
      goto cleanup;
   }
   backlog_refresh_window();

cleanup:
   return;
}

static void* ui_control_draw_backlog_line(
   size_t idx, void* iter, void* arg
) {
   struct UI_CONTROL* control = (struct UI_CONTROL*)arg;
   struct BACKLOG_LINE* line = (struct BACKLOG_LINE*)iter;
   bstring nick_decorated = NULL;
   GRAPHICS_RECT nick_size = { 0, 0, 0, 0 };
   GRAPHICS_COLOR msg_fg = UI_TEXT_FG;
   GRAPHICS_RECT* pos = &(control->self.grid_pos);

   ui_window_advance_grid( &(control->self), NULL );

   /* TODO: Divide multiline lines. */

   if( NULL != line->nick ) {
      /* Draw the nick, first. */
      nick_decorated = bformat( "%s: ", bdata( line->nick ) );

      graphics_measure_text( NULL, &nick_size, UI_TEXT_SIZE, nick_decorated );

      graphics_draw_text(
         control->owner->element, pos->x, pos->y,
         GRAPHICS_TEXT_ALIGN_LEFT, UI_NICK_FG, UI_TEXT_SIZE,
         nick_decorated, VFALSE
      );

      pos->x += nick_size.w;
   } else {
      msg_fg = GRAPHICS_COLOR_MAGENTA;
   }

   graphics_draw_text(
      control->owner->element, pos->x, pos->y,
      GRAPHICS_TEXT_ALIGN_LEFT, msg_fg, UI_TEXT_SIZE, line->line, VFALSE
   );

   bdestroy( nick_decorated );
   return NULL;
}

/* TODO: Translate this to use abstract UI functions. */
static void ui_control_draw_backlog(
   struct UI_WINDOW* win, struct UI_CONTROL* backlog
) {
   win->grid_pos.w = ui_control_get_draw_width( backlog );
   win->grid_pos.h = ui_control_get_draw_height( backlog );

   memcpy(
      &(backlog->self.grid_pos), &(win->grid_pos), sizeof( GRAPHICS_RECT )
   );

   backlog->self.grid_pos.y += UI_WINDOW_MARGIN;

   backlog_iter( ui_control_draw_backlog_line, backlog );
}
