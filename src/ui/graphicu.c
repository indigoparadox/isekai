
#define UI_C
#include "../ui.h"

#include <stdlib.h>

#include "../graphics.h"
#include "../callback.h"
#include "../backlog.h"

#include "../windefs.h"

static struct UI global_ui;

const struct tagbstring str_dialog_control_default_id =
   bsStatic( "dialog_text" );
const struct tagbstring str_dialog_label_default_id =
   bsStatic( "dialog_label" );
const struct tagbstring str_dialog_default_title =
   bsStatic( "Untitled Window" );

void ui_cleanup( struct UI* ui ) {
   #ifdef DEBUG
   ui_debug_stack( ui );
   #endif /* DEBUG */
   scaffold_assert( &global_ui == ui );
   /*
   vector_remove_cb( &(ui->windows), callback_free_windows, NULL );
   */

   while( 0 < vector_count( &(ui->windows) ) ) {
      ui_window_pop( ui );
   }

   vector_cleanup( &(ui->windows) );
}

/** \brief
 * \param[in] title  Can be NULL for default title.
 * \param
 */
void ui_window_init(
   struct UI_WINDOW* win, struct UI* ui,
   const bstring id, const bstring title, const bstring prompt,
   GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
   GFX_COORD_PIXEL width, GFX_COORD_PIXEL height
) {
   struct UI_CONTROL* control = NULL;
   GRAPHICS_RECT sizing_rect = { 0 };

   scaffold_assert( &global_ui == ui );

   win->ui = ui;
   win->area.x = x;
   win->area.y = y;
   win->area.w = width;
   win->area.h = height;
   win->element = scaffold_alloc( 1, GRAPHICS );
   win->dirty = TRUE; /* Draw at least once. */

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

   vector_init( &(win->controls_active) );

   graphics_surface_init( win->element, width, height );

cleanup:
   return;
}

void ui_window_cleanup( struct UI_WINDOW* win ) {
   scaffold_assert( win->ui == &global_ui );
   bdestroy( win->title );
   win->title = NULL;
   bdestroy( win->id );
   win->id = NULL;
   if( NULL != win->controls.data ) {
      hashmap_remove_cb( &(win->controls), callback_free_controls, NULL );
      hashmap_cleanup( &(win->controls) );
   }
   if( NULL != win->element && NULL != win->element->surface ) {
      graphics_surface_free( win->element );
      win->element = NULL;
   }
   win->controls_active.count = 0; /* Force remove. */
   vector_cleanup( &(win->controls_active) );
}

void ui_window_free( struct UI_WINDOW* win ) {
   scaffold_assert( win->ui == &global_ui );
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
   struct UI_CONTROL* control,
   const bstring text, UI_CONTROL_TYPE type, BOOL can_focus, bstring buffer,
   GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
   GFX_COORD_PIXEL width, GFX_COORD_PIXEL height
) {
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
   control->self.area.x = x;
   control->self.area.y = y;
   control->self.area.w = width;
   control->self.area.h = height;
   control->can_focus = can_focus;
   control->self.title = NULL;
   control->self.attachment = NULL;
   vector_init( &(control->self.controls_active) );
}

void ui_control_add(
   struct UI_WINDOW* win, bstring id, struct UI_CONTROL* control
) {
#ifdef DEBUG
   struct UI_CONTROL* control_test = NULL;

   control->owner = win;
   control->self.ui = win->ui;

   if( 0 < hashmap_count( &(win->controls) ) ) {
      control_test = (struct UI_CONTROL*)hashmap_get( &(win->controls), id );
      scaffold_assert( NULL == control_test );
   }
#endif /* DEBUG */
   scaffold_assert( NULL != win );
   scaffold_assert( NULL != control );
   scaffold_assert( &global_ui == win->ui );

   hashmap_put( &(win->controls), id, control );

   if( TRUE == control->can_focus ) {
      vector_add( &(win->controls_active), control );
      if( NULL == win->active_control ) {
         win->active_control = control;
      }
   }
}

void ui_control_free( struct UI_CONTROL* control ) {

   scaffold_assert( &global_ui == control->self.ui );

   ui_window_cleanup( &(control->self) );
   if( FALSE == control->borrowed_text_field ) {
      bdestroy( control->text );
      control->text = NULL;
   }
   scaffold_free( control );
}

void ui_init( GRAPHICS* screen ) {
   vector_init( &(global_ui.windows) );
}

struct UI* ui_get_local() {
   return &global_ui;
}

void ui_window_transform(
   struct UI_WINDOW* win, const GRAPHICS_RECT* new_area
) {
   GRAPHICS* old_element = NULL;
   #ifdef DEBUG
   ui_debug_stack( win->ui );
   #endif /* DEBUG */
   scaffold_assert( &global_ui == win->ui );
   memcpy( &(win->area), new_area, sizeof( GRAPHICS_RECT ) );
   old_element = win->element;
   win->element = NULL;
   graphics_surface_new( win->element, 0, 0, win->area.w, win->area.h );
   graphics_blit_stretch( win->element, 0, 0, win->area.w, win->area.h, old_element );
   #ifdef DEBUG
   ui_debug_stack( win->ui );
   #endif /* DEBUG */
cleanup:
   graphics_surface_free( old_element );
   return;
}

void ui_window_push( struct UI* ui, struct UI_WINDOW* win ) {
   #ifdef DEBUG
   ui_debug_stack( ui );
   #endif /* DEBUG */
   /* TODO: Don't allow dupes. */
   scaffold_assert( NULL != win );
   scaffold_assert( NULL != ui );
   scaffold_assert( &global_ui == win->ui );
   scaffold_assert( &global_ui == ui );
   vector_insert( &(ui->windows), 0, win );
   #ifdef DEBUG
   ui_debug_stack( ui );
   #endif /* DEBUG */
}

void ui_window_pop( struct UI* ui ) {
   struct UI_WINDOW* win = NULL;

   scaffold_assert( &global_ui == ui );

   #ifdef DEBUG
   ui_debug_stack( ui );
   #endif /* DEBUG */

   win = (struct UI_WINDOW*)vector_get( &(ui->windows), 0 );
   scaffold_check_null( win );

   scaffold_assert( &global_ui == win->ui );

   ui_window_free( win );
   vector_remove( &(ui->windows), 0);

cleanup:
   #ifdef DEBUG
   ui_debug_stack( ui );
   #endif /* DEBUG */
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
   struct UI* ui, struct INPUT* input, const bstring id
) {
   struct UI_WINDOW* win = NULL;
   SCAFFOLD_SIZE_SIGNED input_length = 0;
   struct UI_CONTROL* control = NULL;
   int bstr_result;

#ifdef DEBUG
   if( NULL != win ) {
      scaffold_assert( &global_ui == win->ui );
   }
#endif /* DEBUG */

   if( NULL == id ) {
      win = (struct UI_WINDOW*)vector_get( &(ui->windows), 0 );
   } else {
      win = ui_window_by_id( ui, id );
   }
   if( NULL == win ) { goto cleanup; }

   assert( win->ui == &global_ui );
   control = win->active_control;

   if( 0 < input->repeat ) {
      goto cleanup;
   }

   if(
      NULL != control &&
      INPUT_TYPE_KEY == input->type &&
      (scaffold_char_is_printable( input->character ) ||
      ' ' == input->character)
   ) {
      bstr_result = bconchar( control->text, input->character );
      win->dirty = TRUE;
      scaffold_check_nonzero( bstr_result );
#ifdef DEBUG_KEYS
      scaffold_print_debug( &module, "Input field: %s\n", bdata( buffer ) );
#endif /* DEBUG_KEYS */
   } else if(
      NULL != control &&
      INPUT_TYPE_KEY == input->type &&
      INPUT_SCANCODE_BACKSPACE == input->scancode
   ) {
      bstr_result = btrunc( control->text, blength( control->text ) - 1 );
      win->dirty = TRUE;
      scaffold_check_nonzero( bstr_result );
   } else if(
      NULL != control &&
      INPUT_TYPE_KEY == input->type &&
      INPUT_SCANCODE_ENTER == input->scancode
   ) {
      input_length = blength( control->text );
      ui_window_destroy( win->ui, id );
   } else if(
      NULL != control &&
      INPUT_TYPE_KEY == input->type &&
      INPUT_SCANCODE_TAB == input->scancode
   ) {
      ui_window_next_active_control( win );
      win->dirty = TRUE;
   } else if(
      INPUT_TYPE_KEY == input->type &&
      INPUT_SCANCODE_ESC == input->scancode
   ) {
      input_length = -1;
      ui_window_destroy( win->ui, id );
      goto cleanup;
   }

cleanup:
   return input_length;
}

static SCAFFOLD_SIZE ui_control_get_draw_width( const struct UI_CONTROL* control ) {
   SCAFFOLD_SIZE control_w = 0;
   GRAPHICS_RECT control_size = { 0 };
   if( UI_CONTROL_TYPE_TEXT == control->type ) {
      /* Text boxes are wider than their input. */
      control_w = UI_TEXT_DEF_CONTROL_SIZE + (2 * UI_TEXT_MARGIN);
   } else {
      graphics_measure_text( NULL, &control_size, UI_TEXT_SIZE, control->text );
      control_w = control_size.w + (2 * UI_TEXT_MARGIN);
   }
   return control_w;
}

static SCAFFOLD_SIZE ui_control_get_draw_height( const struct UI_CONTROL* control ) {
   return UI_TEXT_SIZE + (2 * UI_TEXT_MARGIN);
}

static void ui_window_advance_grid( struct UI_WINDOW* win, const struct UI_CONTROL* control ) {
   SCAFFOLD_SIZE_SIGNED
      control_w = -1,
      control_h = -1;
   GRAPHICS_RECT text;

   assert( win->ui == &global_ui );

   if( NULL != control ) {
      control_w = ui_control_get_draw_width( control );
      control_h = ui_control_get_draw_height( control );
   } else {
      graphics_measure_text( NULL, &text, UI_TEXT_SIZE, NULL );
      control_w = 0;
      control_h = text.h;
   }

   if( NULL != control && UI_CONTROL_TYPE_BUTTON == control->type ) {
      win->grid_previous_button = TRUE;
   } else {
      win->grid_previous_button = FALSE;
   }

   if( TRUE == win->grid_previous_button ) {
      win->grid_x += control_w + UI_WINDOW_MARGIN;
   } else {
      win->grid_x = UI_WINDOW_MARGIN;
      win->grid_y += control_h + UI_WINDOW_MARGIN;
   }

   assert( win->ui == &global_ui );
}

static void ui_window_reset_grid( struct UI_WINDOW* win ) {
   assert( win->ui == &global_ui );
   win->grid_x = UI_WINDOW_MARGIN;
   win->grid_y = UI_TITLEBAR_SIZE + 4 + UI_WINDOW_MARGIN;
   win->grid_previous_button = FALSE;
   assert( win->ui == &global_ui );
}

static void* ui_control_window_size_cb( struct CONTAINER_IDX* idx, void* iter, void* arg ) {
   struct UI_WINDOW* win = (struct UI_WINDOW*)arg;
   const struct UI_CONTROL* control = (struct UI_CONTROL*)iter;
   GRAPHICS_RECT* largest_control = NULL;
   SCAFFOLD_SIZE_SIGNED
      control_w = control->self.area.w,
      control_h = control->self.area.h;
   GRAPHICS* g = NULL;

   control_w = ui_control_get_draw_width( control );
   control_h = ui_control_get_draw_height( control );

   if(
      0 >= win->area.w ||
      win->area.w < win->grid_x + control_w + UI_WINDOW_MARGIN
   ) {
      largest_control = scaffold_alloc( 1, GRAPHICS_RECT );
      largest_control->w = win->grid_x + control_w + UI_WINDOW_MARGIN;
   }

   if(
      0 >= win->area.h ||
      win->area.h < win->grid_y + control_h + UI_WINDOW_MARGIN
   ) {
      if( NULL == largest_control ) {
         largest_control = scaffold_alloc( 1, GRAPHICS_RECT );
      }
      largest_control->h = win->grid_y + control_h + UI_WINDOW_MARGIN;
   }

   ui_window_advance_grid( (struct UI_WINDOW*)win, control );

   return largest_control;
}

static void* ui_control_draw_backlog_line(
   struct CONTAINER_IDX* idx, void* iter, void* arg
) {
   struct UI_WINDOW* win = (struct UI_WINDOW*)arg;
   struct BACKLOG_LINE* line = (struct BACKLOG_LINE*)iter;
   bstring nick_decorated = NULL;
   GRAPHICS_RECT nick_size = { 0, 0, 0, 0 };
   GRAPHICS_COLOR msg_fg = UI_TEXT_FG;

   /* TODO: Divide multiline lines. */

   if( NULL != line->nick ) {
      /* Draw the nick, first. */
      nick_decorated = bformat( "%s: ", bdata( line->nick ) );

      graphics_measure_text( NULL, &nick_size, UI_TEXT_SIZE, nick_decorated );

      graphics_draw_text(
         win->element, win->grid_x + UI_TEXT_MARGIN, win->grid_y,
         GRAPHICS_TEXT_ALIGN_LEFT, UI_NICK_FG, UI_TEXT_SIZE, nick_decorated, FALSE
      );
   } else {
      msg_fg = GRAPHICS_COLOR_MAGENTA;
   }

   graphics_draw_text(
      win->element, win->grid_x + UI_TEXT_MARGIN + nick_size.w, win->grid_y,
      GRAPHICS_TEXT_ALIGN_LEFT, msg_fg, UI_TEXT_SIZE, line->line, FALSE
   );

   ui_window_advance_grid( (struct UI_WINDOW*)win, NULL );

   bdestroy( nick_decorated );
   return NULL;
}

static void ui_control_draw_backlog(
   struct UI_WINDOW* win, struct UI_CONTROL* backlog
) {
   GRAPHICS_COLOR fg = UI_TEXT_FG;
   GRAPHICS* g = win->element;
   SCAFFOLD_SIZE_SIGNED control_w;
   SCAFFOLD_SIZE_SIGNED control_h;
   struct CHANNEL* l = NULL;

   control_w = ui_control_get_draw_width( backlog );
   control_h = ui_control_get_draw_height( backlog );

   backlog_iter( ui_control_draw_backlog_line, win );

cleanup:
   return;
}

static void ui_control_draw_inventory(
   struct UI_WINDOW* win, struct UI_CONTROL* inv_pane
) {
   GRAPHICS_COLOR fg = UI_TEXT_FG;
   GRAPHICS* g = win->element;
   SCAFFOLD_SIZE_SIGNED control_w;
   SCAFFOLD_SIZE_SIGNED control_h;
   struct CHANNEL* l = NULL;

   control_w = ui_control_get_draw_width( inv_pane );
   control_h = ui_control_get_draw_height( inv_pane );

   //graphics_draw_rect( g, inv_pane->self.x , inv_pane->self.y, inv_pane->self.width, inv_pane->self.height, GRAPHICS_COLOR_DARK_BLUE )

cleanup:
   return;
}

static void ui_control_draw_textfield(
   struct UI_WINDOW* win, struct UI_CONTROL* textfield
) {
   GRAPHICS_COLOR fg = UI_TEXT_FG;
   GRAPHICS* g = win->element;
   SCAFFOLD_SIZE_SIGNED control_w;
   SCAFFOLD_SIZE_SIGNED control_h;

   control_w = ui_control_get_draw_width( textfield );
   control_h = ui_control_get_draw_height( textfield );

   graphics_draw_rect(
      g, win->grid_x, win->grid_y, control_w, control_h, UI_TEXT_BG, TRUE
   );

   if( NULL != textfield->text ) {
      graphics_draw_text(
         g, win->grid_x + UI_TEXT_MARGIN, win->grid_y + UI_TEXT_MARGIN,
         GRAPHICS_TEXT_ALIGN_LEFT, fg, UI_TEXT_SIZE, textfield->text,
         textfield == win->active_control ? TRUE : FALSE
      );
   }

   ui_window_advance_grid( (struct UI_WINDOW*)win, textfield );
}

static void ui_control_draw_label(
   struct UI_WINDOW* win, struct UI_CONTROL* label
) {
   GRAPHICS_COLOR fg = UI_LABEL_FG;
   GRAPHICS* g = win->element;

   if( NULL != label->text ) {
      graphics_draw_text(
         g, win->grid_x + UI_TEXT_MARGIN, win->grid_y + UI_TEXT_MARGIN,
         GRAPHICS_TEXT_ALIGN_LEFT, fg, UI_TEXT_SIZE, label->text,
         label == win->active_control ? TRUE : FALSE
      );
   }

   ui_window_advance_grid( (struct UI_WINDOW*)win, label );
}

static void ui_control_draw_button(
   struct UI_WINDOW* win, struct UI_CONTROL* button
) {
   GRAPHICS_COLOR fg = UI_BUTTON_FG;
   GRAPHICS* g = win->element;
   SCAFFOLD_SIZE_SIGNED control_w;
   SCAFFOLD_SIZE_SIGNED control_h;

   control_w = ui_control_get_draw_width( button );
   control_h = ui_control_get_draw_height( button );

   graphics_draw_rect(
      g, win->grid_x, win->grid_y, control_w, control_h, UI_BUTTON_BG, FALSE
   );

   if( NULL != button->text ) {
      graphics_draw_text(
         g, win->grid_x + UI_TEXT_MARGIN, win->grid_y + UI_TEXT_MARGIN,
         GRAPHICS_TEXT_ALIGN_LEFT, fg, UI_TEXT_SIZE, button->text,
         button == win->active_control ? TRUE : FALSE
      );
   }

   ui_window_advance_grid( (struct UI_WINDOW*)win, button );
}

static void* ui_control_draw_cb( struct CONTAINER_IDX* idx, void* iter, void* arg ) {
   struct UI_WINDOW* win = (struct UI_WINDOW*)arg;
   struct UI_CONTROL* control = (struct UI_CONTROL*)iter;
#ifdef DEBUG
   const char* win_id_c = bdata( win->id ),
      * control_id_c = bdata( control->self.id );
#endif /* DEBUG */

   switch( control->type ) {
      case UI_CONTROL_TYPE_LABEL: ui_control_draw_label( win, control ); break;
      case UI_CONTROL_TYPE_BUTTON: ui_control_draw_button( win, control ); break;
      case UI_CONTROL_TYPE_TEXT: ui_control_draw_textfield( win, control ); break;
      case UI_CONTROL_TYPE_BACKLOG: ui_control_draw_backlog( win, control ); break;
      case UI_CONTROL_TYPE_INVENTORY: ui_control_draw_inventory( win, control ); break;
   }

cleanup:
   return NULL;
}

static void ui_window_enforce_minimum_size( struct UI_WINDOW* win ) {
   GRAPHICS_RECT* largest_control;
   //GRAPHICS_RECT win_new;

   //memcpy( &win_new, &(win->area), sizeof( GRAPHICS_RECT ) );

   /* Make sure the window can contain its largest control. */
   largest_control = scaffold_alloc( 1, GRAPHICS_RECT );
   scaffold_check_null( largest_control );
   memcpy( largest_control, &(win->area), sizeof( GRAPHICS_RECT ) );
   do {
      /* A pointer was returned, so update. */
      //win_new.w = largest_control->w;
      //win_new.h = largest_control->h;
      if( largest_control->w < win->area.w ) {
         largest_control->w = win->area.w;
      }
      if( largest_control->h < win->area.h ) {
         largest_control->h = win->area.h;
      }

      largest_control->x = win->area.x;
      largest_control->y = win->area.y;

      ui_window_transform( win, largest_control  );
      scaffold_free( largest_control );
      ui_window_reset_grid( win );
   } while( NULL != (largest_control = hashmap_iterate( &(win->controls), ui_control_window_size_cb, win )) );
   assert( NULL == largest_control );

   if( 0 >= win->area.w ) {
      win->area.w = UI_WINDOW_MIN_WIDTH;
   }
   if( 0 >= win->area.h ) {
      win->area.h = UI_WINDOW_MIN_HEIGHT;
   }

cleanup:
   assert( win->ui == &global_ui );
}

static void ui_window_draw_furniture( const struct UI_WINDOW* win ) {
   graphics_draw_rect(
      win->element, 2, 2, win->area.w - 4, UI_TITLEBAR_SIZE + 4,
      UI_TITLEBAR_BG, TRUE
   );
   graphics_draw_text(
      win->element, win->area.w / 2, 4, GRAPHICS_TEXT_ALIGN_CENTER,
      UI_TITLEBAR_FG, UI_TITLEBAR_SIZE, win->title, FALSE
   );
}

static void* ui_window_draw_cb( struct CONTAINER_IDX* idx, void* iter, void* arg ) {
   GRAPHICS* g = (GRAPHICS*)arg;
   struct UI_WINDOW* win = (struct UI_WINDOW*)iter;
   SCAFFOLD_SIZE_SIGNED
      win_x = win->area.x,
      win_y = win->area.y;
   BOOL previous_button = FALSE;
   struct UI_CONTROL* control = NULL;

   if( FALSE != win->dirty ) {
      ui_window_enforce_minimum_size( win );

      /* Draw the window. */
      graphics_draw_rect(
         win->element, 0, 0, win->area.w, win->area.h, UI_WINDOW_BG, TRUE
      );

      /* Draw the controls on to the window surface. */
      ui_window_draw_furniture( win );
      ui_window_reset_grid( win );
      hashmap_iterate( &(win->controls), ui_control_draw_cb, win );

      win->dirty = FALSE;

      assert( win->ui == &global_ui );
   }

   /* Draw the window onto the screen. */
   if( 0 > win->area.x ) {
      win_x = (GRAPHICS_SCREEN_WIDTH / 2) - (win->area.w / 2);
   }
   if( 0 > win->area.y ) {
      win_y = (GRAPHICS_SCREEN_HEIGHT / 2) - (win->area.h / 2);
   }
   assert( win->ui == &global_ui );
   graphics_blit( g, win_x, win_y, win->element );

cleanup:
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

void ui_window_draw_tilegrid( struct UI* ui, struct GRAPHICS_TILE_WINDOW* twindow ) {
   SCAFFOLD_SIZE_SIGNED grid_x = 0,
      grid_y = 0;

   scaffold_assert( &global_ui == ui );

   for( grid_x = 0 ; twindow->max_x > grid_x ; grid_x++ ) {

   }
}

struct UI_WINDOW* ui_window_by_id( struct UI* ui, const bstring wid ) {
   scaffold_assert( &global_ui == ui );
   return vector_iterate( &(ui->windows), callback_search_windows, wid );
}

BOOL ui_window_destroy( struct UI* ui, const bstring wid ) {
   scaffold_assert( &global_ui == ui );
   return vector_remove_cb( &(ui->windows), callback_free_windows, wid );
}

struct UI_CONTROL* ui_control_by_id( struct UI_WINDOW* win, const bstring id ) {
   scaffold_assert( &global_ui == win->ui );
   return hashmap_get( &(win->controls), id );
}

void ui_window_next_active_control( struct UI_WINDOW* win ) {
   int i, active_controls_count;
   active_controls_count = vector_count( &(win->controls_active) );
   for( i = 0 ; active_controls_count > i ; i++ ) {
      if(
         vector_get( &(win->controls_active), i ) == win->active_control &&
         i + 1 < active_controls_count
      ) {
         win->active_control = vector_get( &(win->controls_active), i + 1 );
         goto cleanup;
      }
   }

   /* Reached the end of the list. */
   if( 0 < active_controls_count ) {
      win->active_control = vector_get( &(win->controls_active), 0 );
   }

cleanup:
   return;
}

void ui_debug_window( struct UI* ui, const bstring id, bstring buffer ) {
   struct UI_WINDOW* win_debug = NULL;
   struct UI_CONTROL* control_debug = NULL;

   if( NULL == ui_window_by_id( ui, &str_wid_debug ) ) {
      ui_window_new( ui, win_debug, &str_wid_debug,
         &str_wid_debug, NULL, 10, 10, -1, -1 );
      ui_window_push( ui, win_debug );
   }
   win_debug = ui_window_by_id( ui, &str_wid_debug );
   control_debug = ui_control_by_id( win_debug, id );
   if( NULL == control_debug ) {
      ui_control_new( ui, control_debug, NULL,
         UI_CONTROL_TYPE_LABEL, FALSE, buffer, 310, 110, -1, -1 );
      ui_control_add( win_debug, id, control_debug );
   }
cleanup:
   return;
}

#ifdef DEBUG
void ui_debug_stack( struct UI* ui ) {
   vector_iterate_nolock( &(ui->windows), callback_assert_windows, NULL );
}
#endif /* DEBUG */
