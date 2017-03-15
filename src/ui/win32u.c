
#define UI_C
#include "../ui.h"

#include <stdlib.h>

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
   struct UI_CONTROL* control = NULL;

   win->ui = ui;
   win->x = x;
   win->y = y;
   win->width = width;
   win->height = height;

   if( NULL != title ) {
      win->title = bstrcpy( title );
   } else {
      win->title = bstrcpy( &str_dialog_default_title );
   }

   hashmap_init( &(win->controls) );

   if( NULL != prompt ) {
      ui_control_new(
         control, win, prompt, UI_CONTROL_TYPE_LABEL,
         FALSE, NULL, 5, 20, width - 10, 10
      );
      ui_control_add(
         win, (const bstring)&str_dialog_label_default_id,
         control
      );
   }

   if( UI_WINDOW_TYPE_SIMPLE_TEXT == type ) {
      ui_control_new(
         control, win, NULL, UI_CONTROL_TYPE_TEXT,
         FALSE, NULL, 5, 40, width - 10, 10
      );
      ui_control_add(
         win, (const bstring)&str_dialog_control_default_id,
         control
      );
   }

   graphics_surface_init( &(win->element), width, height );

   scaffold_print_debug(
      &module, "Created window with %d controls: %s (%d, %d)\n",
      hashmap_count( &(win->controls) ), bdata( win->title ), win->x, win->y
   );

cleanup:
   return;
}

void ui_window_cleanup( struct UI_WINDOW* win ) {
   bdestroy( win->title );
   if( NULL != win->controls.data ) {
      hashmap_remove_cb( &(win->controls), callback_free_controls, NULL );
      hashmap_cleanup( &(win->controls) );
   }
   if( NULL != win->element.surface ) {
      graphics_surface_free( &(win->element) );
   }
}

void ui_window_free( struct UI_WINDOW* win ) {
   ui_window_cleanup( win );
   free( win );
}

void ui_control_init(
   struct UI_CONTROL* control, struct UI_WINDOW* win,
   const bstring text, UI_CONTROL_TYPE type, BOOL can_focus,
   bstring buffer,
   SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE width, SCAFFOLD_SIZE height
) {
   control->owner = win;
   control->type = type;
   if( NULL != text ) {
      control->text = bstrcpy( text );
   } else {
      control->text = NULL;
   }
   control->self.x = x;
   control->self.y = y;
   control->self.width = width;
   control->self.height = height;
   control->can_focus = can_focus;
   control->self.title = NULL;

   scaffold_print_debug(
      &module, "Created control: %s (%d, %d)\n",
      bdata( control->text ), control->self.x, control->self.y
   );
}

void ui_control_add(
   struct UI_WINDOW* win, bstring id, struct UI_CONTROL* control
) {
#ifdef DEBUG
   struct UI_CONTROL* control_test = NULL;

   if( 0 < hashmap_count( &(win->controls) ) ) {
      control_test = (struct UI_CONTROL*)hashmap_get( &(win->controls), id );
      scaffold_assert( NULL == control_test );
   }
#endif /* DEBUG */
   scaffold_assert( NULL != win );
   scaffold_assert( NULL != control );

   hashmap_put( &(win->controls), id, control );

   scaffold_print_debug(
      &module, "Added control: %s to window: %s\n",
      bdata( control->text ), bdata( win->title )
   );

   if(
      UI_CONTROL_TYPE_BUTTON == control->type ||
      UI_CONTROL_TYPE_TEXT == control->type
   ) {
      win->active_control = control;
      scaffold_print_debug(
         &module, "Set focusable control as focus: %s\n",
         bdata( control->text ), bdata( win->title )
      );
   }
}

void ui_control_free( struct UI_CONTROL* control ) {
   ui_window_cleanup( &(control->self) );
   if( FALSE == control->borrowed_text_field ) {
      bdestroy( control->text );
   }
   free( control );
}

void ui_init( struct UI* ui, GRAPHICS* screen ) {
   vector_init( &(ui->windows) );
}

void ui_window_transform(
   struct UI_WINDOW* win, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE width, SCAFFOLD_SIZE height
) {
   win->x = x;
   win->y = y;
   win->width = width;
   win->height = height;
   graphics_scale( &(win->element), width, height );
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


