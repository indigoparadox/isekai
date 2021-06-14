
#define UI_C
#include "../ui.h"
#include "../callback.h"

const struct tagbstring str_dialog_control_default_id =
   bsStatic( "dialog_text" );
const struct tagbstring str_dialog_label_default_id =
   bsStatic( "dialog_label" );
const struct tagbstring str_dialog_default_title =
   bsStatic( "Untitled Window" );

/** \brief
 * \param[in] title  Can be NULL for default title.
 * \param
 */
void ui_window_init(
   struct UI_WINDOW* win, struct UI* ui, UI_WINDOW_TYPE type,
   const bstring id, const bstring title, const bstring prompt,
   SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE width, SCAFFOLD_SIZE height
) {

}

void ui_window_cleanup( struct UI_WINDOW* win ) {
}

void ui_window_free( struct UI_WINDOW* win ) {
   ui_window_cleanup( win );
   mem_free( win );
}

void ui_control_init(
   struct UI_CONTROL* control, struct UI_WINDOW* win,
   const bstring text, UI_CONTROL_TYPE type, BOOL can_focus,
   bstring buffer,
   SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE width, SCAFFOLD_SIZE height
) {
}

void ui_control_add(
   struct UI_WINDOW* win, bstring id, struct UI_CONTROL* control
) {
}

void ui_control_free( struct UI_CONTROL* control ) {
   ui_window_cleanup( &(control->self) );
   if( FALSE == control->borrowed_text_field ) {
      bdestroy( control->text );
   }
   mem_free( control );
}

void ui_init( struct UI* ui, GRAPHICS* screen ) {
   vector_init( &(ui->windows) );
}

void ui_window_transform(
   struct UI_WINDOW* win, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE width, SCAFFOLD_SIZE height
) {
}

void ui_window_push( struct UI* ui, struct UI_WINDOW* win ) {
   /* TODO: Don't allow dupes. */
   scaffold_assert( NULL != win );
   scaffold_assert( NULL != ui );
   vector_add( &(ui->windows), win );
}

void ui_window_pop( struct UI* ui ) {
   struct UI_WINDOW* win = NULL;

   win = (struct UI_WINDOW*)vector_get( &(ui->windows), 0 );
   scaffold_check_null( win );

   ui_window_free( win );
   vector_remove( &(ui->windows), 0);

cleanup:
   return;
}

/** \brief
 * \param
 * \param
 * \return When the window is dismissed, returns the input length from the
 *         window. Could be 1/2 for a Y/N dialog, 1 for an OK dialog, or
 *         a length of text for a text input dialog.
 */
SCAFFOLD_SIZE ui_poll_input(
   struct UI* ui, struct INPUT* input, bstring buffer, const bstring id
) {
   return 0;
}

static void* ui_control_draw_cb( const bstring res, void* iter, void* arg ) {
   return NULL;
}

static void* ui_window_draw_cb( const bstring res, void* iter, void* arg ) {
   return NULL;
}

void ui_draw( struct UI* ui, GRAPHICS* g ) {

}

void ui_cleanup( struct UI* ui ) {
}

struct UI_WINDOW* ui_window_by_id( struct UI* ui, const bstring wid ) {
}

