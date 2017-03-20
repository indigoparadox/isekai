
#define UI_C
#include "../ui.h"

#include <stdlib.h>

#include "../callback.h"

#include "../windefs.h"

const struct tagbstring str_dialog_control_default_id =
   bsStatic( "dialog_text" );
const struct tagbstring str_dialog_label_default_id =
   bsStatic( "dialog_label" );
const struct tagbstring str_dialog_default_title =
   bsStatic( "Untitled Window" );

void ui_cleanup( struct UI* ui ) {
   vector_remove_cb( &(ui->windows), callback_free_windows, NULL );
   vector_cleanup( &(ui->windows) );
}

/** \brief
 * \param[in] title  Can be NULL for default title.
 * \param
 */
void ui_window_init(
   struct UI_WINDOW* win, struct UI* ui, UI_WINDOW_TYPE type,
   const bstring id, const bstring title, const bstring prompt,
   SCAFFOLD_SIZE_SIGNED x, SCAFFOLD_SIZE_SIGNED y,
   SCAFFOLD_SIZE_SIGNED width, SCAFFOLD_SIZE_SIGNED height
) {
   struct UI_CONTROL* control = NULL;

   win->ui = ui;
   win->x = x;
   win->y = y;
   win->width = width;
   win->height = height;
   win->element = scaffold_alloc( 1, GRAPHICS );

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
      windef_control(
         prompt, UI_CONTROL_TYPE_LABEL, FALSE, NULL, -1, -1, -1, -1
      );
      ui_control_add(
         win, (const bstring)&str_dialog_label_default_id,
         control
      );
   }

   if( UI_WINDOW_TYPE_SIMPLE_TEXT == type ) {
      windef_control(
         NULL, UI_CONTROL_TYPE_TEXT, FALSE, NULL, -1, -1, -1, -1
      );
      ui_control_add(
         win, (const bstring)&str_dialog_control_default_id,
         control
      );
   }

   graphics_surface_init( win->element, width, height );

cleanup:
   return;
}

void ui_window_cleanup( struct UI_WINDOW* win ) {
   bdestroy( win->title );
   bdestroy( win->id );
   if( NULL != win->controls.data ) {
      hashmap_remove_cb( &(win->controls), callback_free_controls, NULL );
      hashmap_cleanup( &(win->controls) );
   }
   if( NULL != win->element && NULL != win->element->surface ) {
      graphics_surface_free( win->element );
      free( win->element );
   }
}

void ui_window_free( struct UI_WINDOW* win ) {
   ui_window_cleanup( win );
   scaffold_free( win );
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
   SCAFFOLD_SIZE_SIGNED x, SCAFFOLD_SIZE_SIGNED y,
   SCAFFOLD_SIZE_SIGNED width, SCAFFOLD_SIZE_SIGNED height
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

   if(
      UI_CONTROL_TYPE_BUTTON == control->type ||
      UI_CONTROL_TYPE_TEXT == control->type
   ) {
      win->active_control = control;
   }
}

void ui_control_free( struct UI_CONTROL* control ) {
   ui_window_cleanup( &(control->self) );
   if( FALSE == control->borrowed_text_field ) {
      bdestroy( control->text );
   }
   scaffold_free( control );
}

void ui_init( struct UI* ui, GRAPHICS* screen ) {
   vector_init( &(ui->windows) );
}

void ui_window_transform(
   struct UI_WINDOW* win, SCAFFOLD_SIZE_SIGNED x, SCAFFOLD_SIZE_SIGNED y,
   SCAFFOLD_SIZE_SIGNED width, SCAFFOLD_SIZE_SIGNED height
) {
   win->x = x;
   win->y = y;
   win->width = width;
   win->height = height;
   graphics_scale( win->element, width, height );
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
SCAFFOLD_SIZE_SIGNED ui_poll_input(
   struct UI* ui, struct INPUT* input, bstring buffer, const bstring id
) {
   struct UI_WINDOW* win = NULL;
   SCAFFOLD_SIZE_SIGNED input_length = 0;
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
      (scaffold_char_is_printable( input->character ) ||
      ' ' == input->character)
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

#if 0
static void* ui_control_draw_cb( const bstring res, void* iter, void* arg ) {
   struct UI_WINDOW* win = (struct UI_WINDOW*)arg;
   struct UI_CONTROL* control = (struct UI_CONTROL*)iter;
   GRAPHICS* g = NULL;
   GRAPHICS_RECT control_size;
   GRAPHICS_COLOR fg = UI_LABEL_FG;
   SCAFFOLD_SIZE_SIGNED win_x = win->x,
      win_y = win->y,
      win_w = win->width,
      win_h = win->height;

   graphics_measure_text( g, &control_size, UI_TEXT_SIZE, control->text );

   /* Figure out sizing. */
   if( 0 > control->self.width ) {
      switch( control->type ) {
      case UI_CONTROL_TYPE_LABEL:
      case UI_CONTROL_TYPE_TEXT:
         control->self.width = control_size.w;
         break;
      case UI_CONTROL_TYPE_BUTTON:
         control->self.width = control_size.w;
         break;
      case UI_CONTROL_TYPE_NONE:
         break;
      }
   }

   if( 0 > control->self.height ) {
      switch( control->type ) {
      case UI_CONTROL_TYPE_LABEL:
      case UI_CONTROL_TYPE_TEXT:
         control->self.height = control_size.h;
         break;
      case UI_CONTROL_TYPE_BUTTON:
         control->self.height = control_size.h;
         break;
      case UI_CONTROL_TYPE_NONE:
         break;
      }
   }

   if(
      0 > win->width ||
      win->width <= control_size.w + (2 * UI_WINDOW_MARGIN)
   ) {
      win_w = control_size.w + (2 * UI_WINDOW_MARGIN);
   }

   if(
      0 > win->height ||
      win->height <= control_size.h + (2 * UI_WINDOW_MARGIN)
   ) {
      win_h = control_size.h + (2 * UI_WINDOW_MARGIN);
   }

   /* TODO: Dynamic screen size detection. */

   ui_window_transform( win, win_x, win_y, win_w, win_h );
   g = win->element;

   if( 0 > win->x ) {
      win_x = (GRAPHICS_SCREEN_WIDTH / 2) + (win->width / 2);
   }

   if( 0 > win->y ) {
      win_y = (GRAPHICS_SCREEN_HEIGHT / 2) + (win->height / 2);
   }

   switch( control->type ) {
   case UI_CONTROL_TYPE_LABEL:
      break;
   case UI_CONTROL_TYPE_BUTTON:
      graphics_draw_rect(
         g,
         0 > control->self.x ? UI_WINDOW_MARGIN : control->self.x,
         0 > control->self.y ? UI_WINDOW_MARGIN : control->self.y,
         control->self.width, control->self.height,
         UI_BUTTON_BG
      );
      fg = UI_BUTTON_FG;
      break;
   case UI_CONTROL_TYPE_TEXT:
      graphics_draw_rect(
         g,
         0 > control->self.x ? UI_WINDOW_MARGIN : control->self.x,
         0 > control->self.y ? UI_WINDOW_MARGIN : control->self.y,
         control->self.width, control->self.height,
         UI_TEXT_BG
      );
      fg = UI_TEXT_FG;
      break;

   case UI_CONTROL_TYPE_NONE:
      break;
   }

   /* TODO: Draw the control onto the window. */
   if( NULL != control->text ) {
      graphics_draw_text(
         g, control->self.x + UI_TEXT_MARGIN, control->self.y + UI_TEXT_MARGIN,
         GRAPHICS_TEXT_ALIGN_LEFT, fg, UI_TEXT_SIZE, control->text
      );
   }

   return NULL;
}
#endif // 0

static void* ui_window_draw_cb( const bstring res, void* iter, void* arg ) {
   GRAPHICS* g = (GRAPHICS*)arg;
   struct UI_WINDOW* win = (struct UI_WINDOW*)iter;
   SCAFFOLD_SIZE_SIGNED win_x = win->x,
      win_y = win->y,
      win_w = win->width,
      win_h = win->height,
      grid_x = 0,
      grid_y = 0,
      grid_width = 0,
      grid_height = 0,
      win_width = 0,
      win_height = 0;
   BOOL previous_button = FALSE;
   struct UI_CONTROL* control = NULL;
   int i;
   GRAPHICS_COLOR fg, bg;

   vector_lock( &(win->controls), TRUE );
   for( i = 0 ; vector_count( &(win->controls) ) > i ; i++ ) {
      control = vector_get( &(win->controls), i );

      if( 0 <= control->self.x ) {
         grid_x = control->self.x;
      } else {
         grid_x += UI_WINDOW_MARGIN;
      }

      if( 0 <= control->self.y ) {
         grid_y = control->self.y;
      } else {
         grid_y += UI_WINDOW_MARGIN;
      }

      if( 0 <= control->self.width ) {
         grid_width = control->self.width;
      }

      if( 0 <= control->self.height ) {
         grid_height = control->self.height;
      }

      switch( control->type ) {
         case UI_CONTROL_TYPE_LABEL:
         case UI_CONTROL_TYPE_TEXT:
            previous_button = FALSE;
            break;

         case UI_CONTROL_TYPE_BUTTON:
            previous_button = TRUE;
            break;
      }

      /* Make sure this all fits on the window. */
      if( win->width < grid_x + grid_width + UI_WINDOW_MARGIN ) {
          win_width = grid_x + grid_width + UI_WINDOW_MARGIN;
      }

      if( win->height < grid_y + grid_height + UI_WINDOW_MARGIN ) {
          win_height = grid_y + grid_height + UI_WINDOW_MARGIN;
      }

      if( TRUE == previous_button ) {
         grid_x += UI_WINDOW_MARGIN;
      } else {
         grid_x = UI_WINDOW_MARGIN;
         grid_y += UI_WINDOW_MARGIN;
      }
   }

   ui_window_transform( win, 0, 0, win_width, win_height );

   grid_x = 0;
   grid_y = 0;
   grid_width = 0;
   grid_height = 0;
   for( i = 0 ; vector_count( &(win->controls) ) > i ; i++ ) {
      control = vector_get( &(win->controls), i );

      if( 0 <= control->self.x ) {
         grid_x = control->self.x;
      } else {
         grid_x += UI_WINDOW_MARGIN;
      }

      if( 0 <= control->self.y ) {
         grid_y = control->self.y;
      } else {
         grid_y += UI_WINDOW_MARGIN;
      }

      if( 0 <= control->self.width ) {
         grid_width = control->self.width;
      }

      if( 0 <= control->self.height ) {
         grid_height = control->self.height;
      }

      switch( control->type ) {
      case UI_CONTROL_TYPE_LABEL:
         previous_button = FALSE;
         break;
      case UI_CONTROL_TYPE_BUTTON:
         graphics_draw_rect(
            g, grid_x, grid_y, grid_width, grid_height, UI_BUTTON_BG
         );
         fg = UI_BUTTON_FG;
         previous_button = TRUE;
         break;
      case UI_CONTROL_TYPE_TEXT:
         graphics_draw_rect(
            g, grid_x, grid_y, grid_width, grid_height, UI_TEXT_BG
         );
         fg = UI_TEXT_FG;
         break;

      case UI_CONTROL_TYPE_NONE:
         break;
      }

      if( TRUE == previous_button ) {
         grid_x += UI_WINDOW_MARGIN;
      } else {
         grid_x = UI_WINDOW_MARGIN;
         grid_y += UI_WINDOW_MARGIN;
      }
   }
   vector_lock( &(win->controls), FALSE );

   /* Draw the window. */
   graphics_draw_rect(
      win->element, 0, 0, win->width, win->height, GRAPHICS_COLOR_BLUE
   );

   /* Draw the title bar. */
   graphics_draw_rect(
      win->element, 2, 2, win->width - 4, UI_TITLEBAR_SIZE + 4,
      UI_TITLEBAR_BG
   );
   graphics_draw_text(
      win->element, win->width / 2, 4, GRAPHICS_TEXT_ALIGN_CENTER,
      UI_TITLEBAR_FG, UI_TITLEBAR_SIZE, win->title
   );

   /* hashmap_iterate( &(win->controls), ui_control_draw_cb, win ); */

   /* Draw the window onto the screen. */
   graphics_blit(
      g, win->x, win->y, win->width, win->height, win->element
   );

   return NULL;
}

void ui_draw( struct UI* ui, GRAPHICS* g ) {
#ifdef DEBUG_PALETTE
   static bstring color_test = NULL;
   int i;
#endif /* DEBUG_PALETTE */

   vector_iterate_r( &(ui->windows), ui_window_draw_cb, g );

#ifdef DEBUG_PALETTE
   if( NULL == color_test ) {
      color_test = bfromcstr( "" );
   }
   for( i = 0 ; 16 > i ; i++ ) {
      bassignformat( color_test, "%d", i );
      graphics_draw_text( g, 10, 20 * i, GRAPHICS_TEXT_ALIGN_LEFT, i, GRAPHICS_FONT_SIZE_12, color_test );
   }
#endif /* DEBUG_PALETTE */
}

struct UI_WINDOW* ui_window_by_id( struct UI* ui, const bstring wid ) {
   return vector_iterate( &(ui->windows), callback_search_windows, wid );
}
