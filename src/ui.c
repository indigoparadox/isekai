#include "ui.h"

void ui_window_init( UI_WINDOW* win, UI* ui, gu x, gu y, gu width, gu height ) {
   win->ui = ui;
   win->title = bfromcstr( "" );
   graphics_surface_init( &(win->element), x, y, width, height );
}

void ui_window_transform( UI_WINDOW* win, gu x, gu y, gu width, gu height ) {
   win->element.x = x;
   win->element.y = y;
   graphics_scale( &(win->element), width, height );
}

void ui_init( UI* ui, GRAPHICS* screen ) {
   ui->screen = screen;
   vector_init( &(ui->windows) );
}

void ui_add_window( UI* ui, UI_WINDOW* win ) {
   ui_lock_windows( ui, TRUE );
   /* TODO: Don't allow dupes. */
   vector_add( &(ui->windows), win );
   ui_lock_windows( ui, FALSE );
}

void ui_lock_windows( UI* ui, BOOL lock ) {

}
