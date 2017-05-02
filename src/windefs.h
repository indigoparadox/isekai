
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
   if( NULL == control ) { \
      scaffold_error = SCAFFOLD_ERROR_NULLPO; \
      goto cleanup; \
   } \
   ui_control_init( control, text, type, can_focus, buffer, x, y, w, h );

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
      ui_window_init(
         win, ui, &str_client_window_id_repl,
         &str_client_window_title_repl, &str_client_window_prompt_repl,
         40, 40, 400, 80
      );
      ui_window_push( ui, win );
   }
cleanup: return;
}

#endif /* DEBUG_VM */

#ifdef USE_CONNECT_DIALOG

SCAFFOLD_INLINE
static void windef_show_connect( struct UI* ui ) {
cleanup: return;
}

#endif /* USE_CONNECT_DIALOG */



#endif /* WINDEFS_H */
