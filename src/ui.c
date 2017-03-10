#include "ui.h"

void ui_window_init( struct UI_WINDOW* win, struct UI* ui, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE width, SCAFFOLD_SIZE height ) {
   win->ui = ui;
   win->title = bfromcstr( "" );
   graphics_surface_init( &(win->element), x, y, width, height );
}

void ui_window_transform( struct UI_WINDOW* win, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE width, SCAFFOLD_SIZE height ) {
   win->element.x = x;
   win->element.y = y;
   graphics_scale( &(win->element), width, height );
}

void ui_init( struct UI* ui, GRAPHICS* screen ) {
   ui->screen = screen;
   vector_init( &(ui->windows) );
}

void ui_add_window( struct UI* ui, struct UI_WINDOW* win ) {
   ui_lock_windows( ui, TRUE );
   /* TODO: Don't allow dupes. */
   vector_add( &(ui->windows), win );
   ui_lock_windows( ui, FALSE );
}

void ui_lock_windows( struct UI* ui, BOOL lock ) {
#ifdef USE_THREADS
#error Locking mechanism undefined!
#endif /* USE_THREADS */
}
