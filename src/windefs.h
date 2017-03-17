
#ifndef WINDEFS_H
#define WINDEFS_H

#define windef_window( type, id, title, prompt, x, y, w, h ) \
   win = (struct UI_WINDOW*)calloc( 1, sizeof( struct UI_WINDOW ) ); \
   if( NULL == win ) { \
      scaffold_error = SCAFFOLD_ERROR_NULLPO; \
      goto cleanup; \
   } \
   ui_window_init( win, ui, type, id, title, prompt, x, y, w, h );

#define windef_control( \
      text, type, can_focus, buffer, x, y, w, h \
) \
   control = (struct UI_CONTROL*)calloc( 1, sizeof( struct UI_CONTROL ) ); \
   if( NULL == win ) { \
      scaffold_error = SCAFFOLD_ERROR_NULLPO; \
      goto cleanup; \
   } \
   ui_control_init( control, win, text, type, can_focus, buffer, x, y, w, h );

#ifdef DEBUG_VM

static struct tagbstring str_client_window_id_repl = bsStatic( "repl" );
static struct tagbstring str_client_window_title_repl =
   bsStatic( "Internal REPL" );
static struct tagbstring str_client_window_prompt_repl =
   bsStatic( "Enter a line to execute:" );

SCAFFOLD_INLINE
static void windef_show_repl( struct UI* ui ) {
   struct UI_WINDOW* win = NULL;
   if( NULL == ui_window_by_id( ui, &str_client_window_id_repl ) ) {
      windef_window(
         UI_WINDOW_TYPE_SIMPLE_TEXT, &str_client_window_id_repl,
         &str_client_window_title_repl, &str_client_window_prompt_repl,
         40, 40, 400, 80
      );
      ui_window_push( ui, win );
   }
cleanup: return;
}

#endif /* DEBUG_VM */

#ifdef USE_CONNECT_DIALOG

static struct tagbstring str_cdialog_title = bsStatic( "Connect to Server" );
static struct tagbstring str_cdialog_prompt =
   bsStatic( "Connect to [address:port]:" );

SCAFFOLD_INLINE
static void windef_show_connect( struct UI* ui ) {
   struct UI_WINDOW* win = NULL;
   windef_window(
      UI_WINDOW_TYPE_SIMPLE_TEXT, NULL, &str_cdialog_title, &str_cdialog_prompt,
      40, 40, 300, 80
   );
   ui_window_push( ui, win );
cleanup: return;
}

#endif /* USE_CONNECT_DIALOG */

#endif /* WINDEFS_H */
