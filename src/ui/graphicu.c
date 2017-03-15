
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

   if( NULL != id ) {
      win->id = bstrcpy( id );
   } else {
      win->id = bfromcstr( "generic" );
   }

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

/** \brief
 *
 * \param
 * \param[in] buffer    A writable field. This gives external processes a way
 *                      to update this field.
 * \return
 *
 */

void ui_control_init(
   struct UI_CONTROL* control, struct UI_WINDOW* win,
   const bstring text, UI_CONTROL_TYPE type, BOOL can_focus, bstring buffer,
   SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE width, SCAFFOLD_SIZE height
) {
   control->owner = win;
   control->type = type;
   if( NULL != text ) {
      scaffold_assert( NULL == buffer );
      control->text = bstrcpy( text );
   } else if( NULL != buffer ) {
      scaffold_assert( NULL == text );
      control->text = buffer;
      control->borrowed_text_field = TRUE;
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
 * \param[in]  ui
 * \param[in]  input    The input object to pull input from.
 * \param[out] buffer   The string that a text input dialog will dump its input
 *                      to when the user presses enter.
 * \param[in]  id       The ID of the window to send input to, or NULL to just
 *                      send it to whatever's on top.
 * \return When the window is dismissed, returns the input length from the
 *         window. Could be 1/2 for a Y/N dialog, 1 for an OK dialog, or
 *         a length of text for a text input dialog.
 */
SCAFFOLD_SIZE ui_poll_input(
   struct UI* ui, struct INPUT* input, bstring buffer, const bstring id
) {
   struct UI_WINDOW* win = NULL;
   SCAFFOLD_SIZE input_length = 0;
   struct UI_CONTROL* control = NULL;
   int bstr_result;

   if( NULL == id ) {
      win = (struct UI_WINDOW*)vector_get( &(ui->windows), 0 );
   } else {
      win = ui_window_by_id( ui, id );
   }
   if( NULL == win ) { goto cleanup; }

   control = win->active_control;
   scaffold_assert( NULL != control );

   /* Borrow the given input buffer if we have one and let us know so we don't
    * try to free it, later.
    */
   if( NULL == control->text ) {
      control->borrowed_text_field = TRUE;
      control->text = buffer;
   }

   if(
      INPUT_TYPE_KEY == input->type &&
      scaffold_char_is_printable( input->character ) ||
      ' ' == input->character
   ) {
      bstr_result = bconchar( buffer, input->character );
      scaffold_check_nonzero( bstr_result );
#ifdef DEBUG_KEYS
      scaffold_print_debug( &module, "Input field: %s\n", bdata( buffer ) );
#endif /* DEBUG_KEYS */
   } else if(
      INPUT_TYPE_KEY == input->type &&
      INPUT_SCANCODE_BACKSPACE == input->scancode
   ) {
      bstr_result = btrunc( buffer, blength( buffer ) - 1 );
      scaffold_check_nonzero( bstr_result );
   } else if(
      INPUT_TYPE_KEY == input->type &&
      INPUT_SCANCODE_ENTER == input->scancode
   ) {
      input_length = blength( buffer );
   }

cleanup:
   return input_length;
}

static void* ui_control_draw_cb( const bstring res, void* iter, void* arg ) {
   struct UI_WINDOW* win = (struct UI_WINDOW*)arg;
   struct UI_CONTROL* control = (struct UI_CONTROL*)iter;
   GRAPHICS* g = &(win->element);

   switch( control->type ) {
   case UI_CONTROL_TYPE_LABEL:
      graphics_set_color( g, GRAPHICS_COLOR_WHITE );
      break;
   case UI_CONTROL_TYPE_BUTTON:
      graphics_set_color( g, GRAPHICS_COLOR_DARK_CYAN );
      graphics_draw_rect(
         g, control->self.x, control->self.y, control->self.width,
         control->self.height
      );
      graphics_set_color( g, GRAPHICS_COLOR_WHITE );
      break;
   case UI_CONTROL_TYPE_TEXT:
      graphics_set_color( g, GRAPHICS_COLOR_DARK_BLUE );
      graphics_draw_rect(
         g, control->self.x, control->self.y, control->self.width,
         control->self.height
      );
      graphics_set_color( g, GRAPHICS_COLOR_WHITE );
      break;

   case UI_CONTROL_TYPE_NONE:
      break;
   }

   /* TODO: Draw the control onto the window. */
   if( NULL != control->text ) {
      graphics_draw_text(
         g, control->self.x + 5, control->self.y + 2, GRAPHICS_TEXT_ALIGN_LEFT,
         control->text
      );
   }

   return NULL;
}

static void* ui_window_draw_cb( const bstring res, void* iter, void* arg ) {
   GRAPHICS* g = (GRAPHICS*)arg;
   struct UI_WINDOW* win = (struct UI_WINDOW*)iter;

   /* Draw the window. */
   graphics_set_color( &(win->element), GRAPHICS_COLOR_BLUE );
   graphics_draw_rect(
      &(win->element), 0, 0, win->width, win->height
   );

   /* Draw the title bar. */
   graphics_set_color( &(win->element), GRAPHICS_COLOR_BROWN );
   graphics_draw_rect(
      &(win->element), 2, 2, win->width - 4, 12 /* TODO: Get text height. */
   );
   graphics_set_color( &(win->element), GRAPHICS_COLOR_WHITE );
   graphics_draw_text(
      &(win->element), win->width / 2, 4, GRAPHICS_TEXT_ALIGN_CENTER, win->title
   );

   hashmap_iterate( &(win->controls), ui_control_draw_cb, win );

   /* Draw the window onto the screen. */
   graphics_blit(
      g, win->x, win->y, win->width, win->height, &(win->element)
   );
}

void ui_draw( struct UI* ui, GRAPHICS* g ) {
   vector_iterate_r( &(ui->windows), ui_window_draw_cb, g );

   /* static bstring color_test = NULL;
   int i;
   if( NULL == color_test ) {
      color_test = bfromcstr( "" );
   }
   for( i = 0 ; 16 > i ; i++ ) {
      graphics_set_color( g, i );
      bassignformat( color_test, "%d", i );
      graphics_draw_text( g, 10, 20 * i, GRAPHICS_TEXT_ALIGN_LEFT, color_test );
   } */
}

struct UI_WINDOW* ui_window_by_id( struct UI* ui, const bstring wid ) {
   vector_iterate( &(ui->windows), callback_search_windows, wid );
}
