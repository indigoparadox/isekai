
#define UI_C
#include "../ui.h"

#include "../graphics.h"
#include "../callback.h"
#include "../backlog.h"
#include "../htmtree.h"
#include "../htmlrend.h"

#include "../windefs.h"

static struct UI global_ui;

/* Count of all mboxes created so far. */
/* Appended to mbox names to make them unique/allow multiple mboxes. */
static int ui_mbox_global_iter = 0;

const struct tagbstring str_dialog_control_default_id =
   bsStatic( "dialog_text" );
const struct tagbstring str_dialog_label_default_id =
   bsStatic( "dialog_label" );
const struct tagbstring str_dialog_default_title =
   bsStatic( "Untitled Window" );

static void ui_draw_rect(
   struct UI_WINDOW* win, GRAPHICS_RECT* rect, GRAPHICS_COLOR color, BOOL filled
) {
   const GFX_COORD_PIXEL offset_y = UI_WINDOW_GRID_Y_START,
      total_y_test = rect->h + rect->y + offset_y;

   scaffold_assert( total_y_test <= win->area.h );

   rect->y += UI_WINDOW_GRID_Y_START;

   graphics_draw_rect(
      win->element, rect->x, rect->y, rect->w, rect->h, color, filled
   );

   rect->y -= UI_WINDOW_GRID_Y_START;
}

static void ui_draw_text(
   struct UI_WINDOW* win, GRAPHICS_RECT* rect,  GRAPHICS_TEXT_ALIGN alignment,
   GRAPHICS_COLOR color, GRAPHICS_FONT_SIZE size, const bstring text,
   BOOL cursor, BOOL margins
) {
   GFX_COORD_PIXEL offset_margins = 0;

   if( FALSE != margins ) {
      offset_margins = UI_TEXT_MARGIN;
   }

   rect->y += UI_WINDOW_GRID_Y_START;

   graphics_draw_text(
      win->element, rect->x + offset_margins, rect->y + offset_margins,
      alignment, color, size, text, cursor
   );

   rect->y -= UI_WINDOW_GRID_Y_START;
}

void ui_cleanup( struct UI* ui ) {
   #ifdef DEBUG
   ui_debug_stack( ui );
   #endif /* DEBUG */
   scaffold_assert( &global_ui == ui );
   /*
   vector_remove_cb( &(ui->windows), callback_free_windows, NULL );
   */

   vector_lock( ui->windows, TRUE );
   while( 0 < vector_count( ui->windows ) ) {
      ui_window_pop( ui );
   }
   vector_lock( ui->windows, FALSE );

   vector_free( &(ui->windows) );
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

   scaffold_assert( &global_ui == ui );

   win->ui = ui;
   win->area.x = x;
   win->area.y = y;
   win->area.w = width;
   win->area.h = height;
   win->element = mem_alloc( 1, GRAPHICS );
   win->dirty = TRUE; /* Draw at least once. */
#ifdef DEBUG
   win->sentinal = UI_SENTINAL_WINDOW;
#endif /* DEBUG */

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

   win->controls = hashmap_new();

   if( NULL != prompt ) {
      ui_control_new(
         ui, control, prompt, UI_CONTROL_TYPE_LABEL, FALSE, FALSE, NULL,
         UI_CONST_WIDTH_FULL, UI_CONST_HEIGHT_FULL,
         UI_CONST_WIDTH_FULL, UI_CONST_HEIGHT_FULL
      );
      ui_control_add(
         win, (const bstring)&str_dialog_label_default_id,
         control
      );
   }

   win->controls_active = vector_new();

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
   if( 0 < hashmap_count( win->controls ) ) {
      hashmap_remove_cb( win->controls, callback_free_controls, NULL );
      hashmap_free( &(win->controls) );
   }
   if( NULL != win->element && NULL != win->element->surface ) {
      graphics_surface_free( win->element );
      win->element = NULL;
   }
   vector_free( &(win->controls_active) );
}

void ui_window_free( struct UI_WINDOW* win ) {
   scaffold_assert( win->ui == &global_ui );
   scaffold_assert( UI_SENTINAL_WINDOW == win->sentinal );
   ui_window_cleanup( win );
   mem_free( win );
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
   const bstring text, UI_CONTROL_TYPE type, BOOL can_focus, BOOL new_row,
   bstring buffer, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
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
   control->min = 0;
   control->max = 0;
   control->self.area.x = x;
   control->self.area.y = y;
   control->self.area.w = width;
   control->self.area.h = height;
   control->can_focus = can_focus;
   control->self.title = NULL;
   control->self.attachment = NULL;
   control->self.grid_iter = 0;
   control->self.selection = 0;
   control->new_row = new_row;
#ifdef DEBUG
   control->self.sentinal = UI_SENTINAL_CONTROL;
#endif /* DEBUG */
   control->self.controls_active = vector_new();
}

void ui_control_add(
   struct UI_WINDOW* win, bstring id, struct UI_CONTROL* control
) {
   SCAFFOLD_SIZE_SIGNED verr;
   struct UI_CONTROL* control_last = NULL;
#ifdef DEBUG
   struct UI_CONTROL* control_test = NULL;

   control->owner = win;
   control->self.ui = win->ui;

   if( 0 < hashmap_count( win->controls ) ) {
      control_test = (struct UI_CONTROL*)hashmap_get( win->controls, id );
      scaffold_assert( NULL == control_test );
   }
#endif /* DEBUG */

   scaffold_assert( NULL != win );
   scaffold_assert( NULL != control );
   scaffold_assert( &global_ui == win->ui );

   if( hashmap_put( win->controls, id, control, FALSE ) ) {
      lg_error( __FILE__,
         "Attempted to double-add control: %b\n", id );
   }

   if( NULL == win->first_control ) {
      win->first_control = control;
   } else {
      control_last = win->first_control;
      while( NULL != control_last->next_control ) {
         control_last = control_last->next_control;
      }
      control_last->next_control = control;
   }

   if( FALSE != control->can_focus ) {
      verr = vector_add( win->controls_active, control );
      if( 0 <= verr && NULL == win->active_control ) {
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
   mem_free( control );
}

void ui_init( GRAPHICS* screen ) {
   global_ui.screen_g = screen;
   global_ui.windows = vector_new();
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
   if( NULL != new_area && new_area != &(win->area) ) {
      memcpy( &(win->area), new_area, sizeof( GRAPHICS_RECT ) );
   }
   old_element = win->element;
   win->element = NULL;
   graphics_surface_new( win->element, 0, 0, win->area.w, win->area.h );
   graphics_blit_stretch(
      win->element, 0, 0, win->area.w, win->area.h, old_element );
   #ifdef DEBUG
   ui_debug_stack( win->ui );
   #endif /* DEBUG */
cleanup:
   graphics_surface_free( old_element );
   return;
}

void ui_window_push( struct UI* ui, struct UI_WINDOW* win ) {
   SCAFFOLD_SIZE_SIGNED verr;
   struct UI_WINDOW* top_window = NULL;

   #ifdef DEBUG
   ui_debug_stack( ui );
   #endif /* DEBUG */

   /* TODO: Don't allow dupes. */
   scaffold_assert( NULL != win );
   scaffold_assert( NULL != ui );
   scaffold_assert( &global_ui == win->ui );
   scaffold_assert( &global_ui == ui );
   top_window = vector_get( ui->windows, 0 );
   if(
      NULL != top_window &&
      0 != bstrncmp( win->id, &str_wid_mbox, blength( &str_wid_mbox ) ) &&
      0 == bstrncmp( top_window->id, &str_wid_mbox, blength( &str_wid_mbox ) )
   ) {
      /* Message boxes always stay on top of normals like these. */
      lg_debug( __FILE__, "Pushing new dialog: %b\n", win->id );
      verr = vector_insert( ui->windows, 1, win );
   } else if(
      NULL != top_window &&
      0 == bstrncmp( win->id, &str_wid_mbox, blength( &str_wid_mbox ) ) &&
      0 == bstrncmp( top_window->id, &str_wid_mbox, blength( &str_wid_mbox ) )
   ) {
      /* Must be /another/ new messagebox. */
      lg_debug( __FILE__, "Pushing new mbox: %b\n", win->id );
      verr = vector_insert( ui->windows, 1, win );
   } else {
      /* Must be a new dialog. */
      lg_debug( __FILE__, "Pushing dialog or mbox: %b\n", win->id );
      verr = vector_insert( ui->windows, 0, win );
   }
   lgc_negative( verr );

cleanup:
   #ifdef DEBUG
   ui_debug_stack( ui );
   #endif /* DEBUG */

   return;
}

void ui_window_pop( struct UI* ui ) {
   struct UI_WINDOW* win = NULL;

   scaffold_assert( &global_ui == ui );

   #ifdef DEBUG
   ui_debug_stack( ui );
   #endif /* DEBUG */

   win = (struct UI_WINDOW*)vector_get( ui->windows, 0 );
   lgc_null( win );

   scaffold_assert( &global_ui == win->ui );

   ui_window_free( win );
   vector_lock( ui->windows, FALSE );
   vector_remove( ui->windows, 0);
   vector_lock( ui->windows, TRUE );

cleanup:
   #ifdef DEBUG
   ui_debug_stack( ui );
   #endif /* DEBUG */
   return;
}

static
SCAFFOLD_SIZE_SIGNED ui_poll_keys( struct UI_WINDOW* win, struct INPUT* p ) {
   struct UI_CONTROL* control = NULL;
   int bstr_result = 0;
   SCAFFOLD_SIZE_SIGNED input_length = 0;
   struct VECTOR* items_list = NULL;
   SCAFFOLD_SIZE_SIGNED* numbuff = NULL;

   control = win->active_control;
   if( NULL == control ) {
      goto cleanup;
   }

   /* Handle control-specific responses to keys. */

   switch( control->type ) {
   case UI_CONTROL_TYPE_INVENTORY:
      switch( p->character ) {
      case INPUT_ASSIGNMENT_LEFT:
         items_list = (struct VECTOR*)control->self.attachment;
         if( 0 < control->self.selection ) {
            control->self.selection--;
            win->dirty = TRUE;
         }
         goto cleanup;

      case INPUT_ASSIGNMENT_RIGHT:
         items_list = (struct VECTOR*)control->self.attachment;
         control->self.selection++;
         if( vector_count( items_list ) <= control->self.selection ) {
            control->self.selection = 0;
         }
         win->dirty = TRUE;
         goto cleanup;

      case INPUT_ASSIGNMENT_ATTACK:
         input_length = UI_INPUT_RETURN_KEY_ENTER;
         break;

      case INPUT_ASSIGNMENT_QUIT:
         input_length = UI_INPUT_RETURN_KEY_ESC;
         break;
      }
      break;

   case UI_CONTROL_TYPE_SPINNER:
      numbuff = (SCAFFOLD_SIZE_SIGNED*)(control->self.attachment);
      switch( p->scancode ) {
      case INPUT_SCANCODE_UP:
      case INPUT_SCANCODE_LEFT:
         (*numbuff)--;
         if( control->min > *numbuff ) {
            *numbuff = control->max;
         }
         win->dirty = TRUE;
         goto cleanup;

      case INPUT_SCANCODE_DOWN:
      case INPUT_SCANCODE_RIGHT:
         (*numbuff)++;
         if( control->max < *numbuff ) {
            *numbuff = 0;
         }
         win->dirty = TRUE;
         goto cleanup;

      case INPUT_SCANCODE_ENTER:
         /* Return spinner value. */
         /* (This could bite us if spinner value is 0? */
         input_length = *numbuff;
         break;

      case INPUT_SCANCODE_TAB:
         input_length = UI_INPUT_RETURN_KEY_NEXT;
         break;

      case INPUT_SCANCODE_ESC:
         input_length = UI_INPUT_RETURN_KEY_ESC;
         break;
      }
      break;

   case UI_CONTROL_TYPE_DROPDOWN:
      numbuff = (SCAFFOLD_SIZE_SIGNED*)(control->self.attachment);
      switch( p->scancode ) {
      case INPUT_SCANCODE_UP:
      case INPUT_SCANCODE_LEFT:
         (*numbuff)--;
         if( 0 > *numbuff ) {
            *numbuff = vector_count( control->list ) - 1;
         }
         win->dirty = TRUE;
         goto cleanup;

      case INPUT_SCANCODE_DOWN:
      case INPUT_SCANCODE_RIGHT:
         (*numbuff)++;
         if( vector_count( control->list ) <= *numbuff ) {
            *numbuff = 0;
         }
         win->dirty = TRUE;
         goto cleanup;

      case INPUT_SCANCODE_ENTER:
         input_length = UI_INPUT_RETURN_KEY_ENTER;
         break;

      case INPUT_SCANCODE_TAB:
         input_length = UI_INPUT_RETURN_KEY_NEXT;
         break;

      case INPUT_SCANCODE_ESC:
         input_length = UI_INPUT_RETURN_KEY_ESC;
         break;
      }
      break;

   case UI_CONTROL_TYPE_HTML:
      /* TODO */
      switch( p->scancode ) {
      case INPUT_SCANCODE_ENTER:
         input_length = UI_INPUT_RETURN_KEY_ENTER;
         break;

      case INPUT_SCANCODE_TAB:
         input_length = UI_INPUT_RETURN_KEY_NEXT;
         break;

      case INPUT_SCANCODE_ESC:
         input_length = UI_INPUT_RETURN_KEY_ESC;
         break;
      }
      break;

   case UI_CONTROL_TYPE_TEXT:
      /* This is different from above as the text is freely editable. */
      switch( p->scancode ) {
      case INPUT_SCANCODE_BACKSPACE:
         bstr_result = btrunc( control->text, blength( control->text ) - 1 );
         win->dirty = TRUE; /* Check can shunt to cleanup, so dirty first. */
         lgc_nonzero( bstr_result );
         break;

      case INPUT_SCANCODE_ENTER:
         /* Text fields have a length, so use that. */
         input_length = blength( control->text );
         break;

      case INPUT_SCANCODE_TAB:
         /* Don't concat it by default, and set special return. */
         input_length = UI_INPUT_RETURN_KEY_NEXT;
         break;

      case INPUT_SCANCODE_ESC:
         /* Don't concat it by default, and set special return. */
         input_length = UI_INPUT_RETURN_KEY_ESC;
         break;

      default:
         /* Append printable characters to text buffer. */
         if( !p->character ) {
#ifdef DEBUG_KEYS
            lg_debug( __FILE__, "Field input char unprintable.\n" );
#endif /* DEBUG_KEYS */
         } else {
#ifdef DEBUG_KEYS
            lg_debug( __FILE__, "Input field: %s\n", bdata( buffer ) );
#endif /* DEBUG_KEYS */
            bstr_result = bconchar( control->text, p->character );
            win->dirty = TRUE; /* Check can shunt to cleanup, so dirty first. */
            lgc_nonzero( bstr_result );
         }
         break;
      }
      break;
   }

   /* Handle common actions for special keys that apply to ALL controls. */

   switch( p->scancode ) {
   case INPUT_SCANCODE_ENTER:
      ui_window_destroy( win->ui, win->id );
      goto cleanup;

   case INPUT_SCANCODE_TAB:
      ui_window_next_active_control( win );
      win->dirty = TRUE;
      goto cleanup;

   case INPUT_SCANCODE_ESC:
      ui_window_destroy( win->ui, win->id );
      goto cleanup;
   }

cleanup:
   return input_length;
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

#ifdef DEBUG
   if( NULL != win ) {
      scaffold_assert( &global_ui == win->ui );
   }
#endif /* DEBUG */

   /* Special case: message boxes. */
   win = (struct UI_WINDOW*)vector_get( ui->windows, 0 );
   if(
      INPUT_TYPE_KEY == input->type &&
      NULL != win &&
      0 == bstrncmp( win->id, &str_wid_mbox, blength( &str_wid_mbox ) )
   ) {
      switch( input->scancode ) {
         case INPUT_SCANCODE_ENTER:
         case INPUT_SCANCODE_ESC:
            ui_window_destroy( win->ui, win->id );
            goto cleanup;

         default:
            goto cleanup;
      }
   }

   /* No message box, so grab the top or requested dialog. */
   if( NULL == id ) {
      win = (struct UI_WINDOW*)vector_get( ui->windows, 0 );
   } else {
      win = ui_window_by_id( ui, id );
   }
   if( NULL == win ) { goto cleanup; }

   assert( win->ui == &global_ui );

   if( 0 < input->repeat ) {
      goto cleanup;
   }

   if( INPUT_TYPE_KEY == input->type ) {
      input_length = ui_poll_keys( win, input );

      lg_debug(
         __FILE__, "Input length returned: %d\n", input_length
      );
   }

cleanup:
   return input_length;
}

static SCAFFOLD_SIZE ui_control_get_draw_width(
   const struct UI_CONTROL* control
) {
   SCAFFOLD_SIZE control_w = 0;
   GRAPHICS_RECT control_size = { 0 };
   bstring list_item = NULL;
   SCAFFOLD_SIZE num,
      i = 0;

   switch( control->type ) {
   case UI_CONTROL_TYPE_TEXT:
      /* Text boxes are wider than their input. */
      control_w = UI_TEXT_DEF_CONTROL_SIZE + (2 * UI_TEXT_MARGIN);
      break;

   case UI_CONTROL_TYPE_DROPDOWN:
      while( vector_count( control->list ) > i ) {
         list_item = vector_get( control->list, i );
         graphics_measure_text( NULL, &control_size, UI_TEXT_SIZE, list_item );
         num = control_size.w + (2 * UI_TEXT_MARGIN);
         if( num > control_w ) {
            control_w = num;
         }
         i++;
      }
      break;

   default:
      graphics_measure_text( NULL, &control_size, UI_TEXT_SIZE, control->text );
      control_w = control_size.w + (2 * UI_TEXT_MARGIN);
      break;
   }

   return control_w;
}

static SCAFFOLD_SIZE ui_control_get_draw_height(
   const struct UI_CONTROL* control
) {
   return UI_TEXT_SIZE + (2 * UI_TEXT_MARGIN);
}

void ui_control_set(
   struct UI_CONTROL* ctrl, UI_OPTION opt, UI_OPT_STATE state
) {
   switch( opt ) {
   case UI_OPTION_NEW_ROW:
      ctrl->new_row = state;
      break;
   }
}

static void ui_window_advance_grid(
   struct UI_WINDOW* win, const struct UI_CONTROL* control
) {
   GRAPHICS_RECT text;

   assert( win->ui == &global_ui );

   if( NULL != control ) {
      win->grid_pos.w = ui_control_get_draw_width( control );
      win->grid_pos.h = ui_control_get_draw_height( control );
   } else {
      graphics_measure_text( NULL, &text, UI_TEXT_SIZE, NULL );
      win->grid_pos.w = 0;
      win->grid_pos.h = text.h;
   }

   if( NULL == control || FALSE != control->new_row ) {
      win->grid_pos.x = UI_WINDOW_MARGIN;
      win->grid_pos.y += win->grid_pos.h + UI_WINDOW_MARGIN;
   } else {
      win->grid_pos.x += win->grid_pos.w + UI_WINDOW_MARGIN;
   }

/* cleanup: */
   assert( win->ui == &global_ui );
}

static void ui_window_reset_grid( struct UI_WINDOW* win ) {
   assert( win->ui == &global_ui );
   win->grid_pos.x = UI_WINDOW_GRID_X_START;
   win->grid_pos.y = 0;
   assert( win->ui == &global_ui );
}

static void* ui_control_window_size_cb(
   bstring idx, void* iter, void* arg
) {
   struct UI_WINDOW* win = (struct UI_WINDOW*)arg;
   const struct UI_CONTROL* control = (struct UI_CONTROL*)iter;
   GRAPHICS_RECT* largest_control = NULL;

   ui_window_advance_grid( (struct UI_WINDOW*)win, control );

   win->grid_pos.w = ui_control_get_draw_width( control );
   win->grid_pos.h = ui_control_get_draw_height( control );

   if(
      0 >= win->area.w ||
      win->area.w < win->grid_pos.x + win->grid_pos.w + UI_WINDOW_MARGIN
   ) {
      largest_control = mem_alloc( 1, GRAPHICS_RECT );
      largest_control->w = win->grid_pos.x + win->grid_pos.w + UI_WINDOW_MARGIN;
   }

   if(
      0 >= win->area.h ||
      win->area.h < win->grid_pos.y + win->grid_pos.h + UI_WINDOW_MARGIN
   ) {
      if( NULL == largest_control ) {
         largest_control = mem_alloc( 1, GRAPHICS_RECT );
      }
      largest_control->h = win->grid_pos.y + win->grid_pos.h + UI_WINDOW_MARGIN;
   }

   return largest_control;
}

static void ui_control_auto_size(
   struct UI_CONTROL* control, GRAPHICS_RECT* size_out, struct UI_WINDOW* win,
   SCAFFOLD_SIZE_SIGNED text_lines
) {
   GFX_COORD_PIXEL
      x = control->self.area.x,
      y = control->self.area.y,
      w = control->self.area.w,
      h = control->self.area.h;

   switch( x ) {
   case UI_CONST_WIDTH_FULL:
      size_out->x = UI_WINDOW_GRID_X_START;
      break;
   case UI_CONST_WIDTH_HALF:
      size_out->x = (win->area.w / 2) + UI_WINDOW_GRID_X_START; /* TODO: What if it's auto-sized? */
      break;
   default:
      size_out->x = x;
      break;
   }

   switch( y ) {
   case UI_CONST_HEIGHT_FULL:
      /* This translated for the title bar in the primitive stage. */
      size_out->y = UI_WINDOW_GRID_Y_START;
      break;
   case UI_CONST_HEIGHT_HALF:
      /* TODO: What if it's auto-sized? */
      size_out->y = (win->area.h / 2) + UI_WINDOW_GRID_Y_START;
      break;
   default:
      size_out->y = y;
      break;
   }

   switch( w ) {
   case UI_CONST_WIDTH_FULL:
      size_out->w = win->area.w - (2 * UI_WINDOW_MARGIN); /* TODO: What if it's auto-sized? */
      break;
   case UI_CONST_WIDTH_HALF:
      size_out->w = (win->area.w / 2) - (2 * UI_WINDOW_MARGIN); /* TODO: What if it's auto-sized? */
      break;
   default:
      size_out->w = w;
      break;
   }

   if( 0 < text_lines ) {
      size_out->h = ui_control_get_draw_height( control ) * text_lines;
      goto cleanup;
   }

   switch( h ) {
   case UI_CONST_HEIGHT_FULL:
      /* TODO: What if it's auto-sized? */
      size_out->h = win->area.h - (2 * UI_WINDOW_GRID_Y_START);
      break;
   case UI_CONST_HEIGHT_HALF:
      /* TODO: What if it's auto-sized? */
      size_out->h = (win->area.h / 2) - ( 2 * UI_WINDOW_GRID_Y_START);
      break;
   default:
      size_out->h = h;
      break;
   }

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
         nick_decorated, FALSE
      );

      pos->x += nick_size.w;
   } else {
      msg_fg = GRAPHICS_COLOR_MAGENTA;
   }

   graphics_draw_text(
      control->owner->element, pos->x, pos->y,
      GRAPHICS_TEXT_ALIGN_LEFT, msg_fg, UI_TEXT_SIZE, line->line, FALSE
   );

   bdestroy( nick_decorated );
   return NULL;
}

#ifndef DISABLE_BACKLOG

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

#endif /* !DISABLE_BACKLOG */

#ifdef USE_ITEMS

static void ui_draw_item_sprite(
   struct UI_CONTROL* inv_pane, GRAPHICS_RECT* rect, struct ITEM* e
) {
   item_draw_ortho(
      e, rect->x, rect->y + UI_WINDOW_GRID_Y_START, inv_pane->owner->element
   );
}

static void* ui_control_draw_inventory_item(
   size_t idx, void* iter, void* arg
) {
   struct ITEM* e = (struct ITEM*)iter;
   struct UI_CONTROL* inv_pane = (struct UI_CONTROL*)arg;
   struct UI_WINDOW* win = inv_pane->owner;
   struct ITEM_SPRITESHEET* catalog = NULL;
   GRAPHICS_RECT label_size;
   GRAPHICS_RECT* inv_grid = &(inv_pane->self.grid_pos);
   GFX_COORD_PIXEL label_icon_offset;

   catalog = client_get_catalog( e->client_or_server, e->catalog_name );
   if( NULL == catalog ) { goto cleanup; }

   graphics_measure_text(
      win->element, &label_size, UI_TEXT_SIZE, e->display_name
   );
   scaffold_assert( 0 < label_size.w );
   label_icon_offset = (label_size.w / 2) - (catalog->spritewidth / 2);

   inv_grid->x += label_icon_offset + UI_TEXT_MARGIN;
   inv_grid->w = catalog->spritewidth;
   inv_grid->h = catalog->spriteheight;

   ui_draw_item_sprite( inv_pane, inv_grid, e );

   inv_grid->x -= label_icon_offset;
   inv_grid->y += (catalog->spriteheight + UI_TEXT_MARGIN);
   inv_grid->w = label_size.w;
   inv_grid->h = label_size.h;

   if(
      inv_pane == win->active_control &&
      inv_pane->self.selection == inv_pane->self.grid_iter
   ) {
      ui_draw_rect( win, inv_grid, UI_SELECTED_BG, TRUE );
   }

   ui_draw_text(
      win, inv_grid, GRAPHICS_TEXT_ALIGN_LEFT, GRAPHICS_COLOR_WHITE,
      UI_TEXT_SIZE, e->display_name, FALSE, FALSE
   );

   inv_grid->x += label_icon_offset + catalog->spritewidth;
   inv_grid->y -= (catalog->spriteheight + UI_TEXT_MARGIN);

   inv_pane->self.grid_iter++;

cleanup:
   return NULL;
}

static void ui_control_draw_inventory(
   struct UI_WINDOW* win, struct UI_CONTROL* inv_pane
) {
   GRAPHICS_RECT bg_rect;
   struct VECTOR* items = NULL;

   /* Draw special out-of-grid BG because items coming. */

   memcpy( &bg_rect, &(inv_pane->self.area), sizeof( GRAPHICS_RECT ) );

   ui_control_auto_size( inv_pane, &bg_rect, win, -1 );

   graphics_shrink_rect( &bg_rect, UI_BAR_WIDTH );

   ui_draw_rect( win, &bg_rect, GRAPHICS_COLOR_DARK_BLUE, TRUE );

   inv_pane->self.grid_iter = 0;
   items = inv_pane->self.attachment;
   lgc_null_msg( items, "Item list invalid." );
   vector_iterate( items, ui_control_draw_inventory_item, inv_pane );

cleanup:
   return;
}

void ui_set_inventory_pane_list(
   struct UI_CONTROL* inv_pane, struct VECTOR* list
) {
   inv_pane->self.attachment = list;
}

#endif // USE_ITEMS

static void ui_control_draw_textfield(
   struct UI_WINDOW* win, struct UI_CONTROL* textfield
) {
   GRAPHICS_COLOR fg = UI_TEXT_FG;
   GRAPHICS_RECT bg_rect;

   if( textfield == win->active_control ) {
      fg = UI_SELECTED_FG;
   }

   memcpy( &bg_rect, &(textfield->self.area), sizeof( GRAPHICS_RECT ) );

   ui_control_auto_size( textfield, &bg_rect, win, 1 );

   win->grid_pos.w = bg_rect.w;
   win->grid_pos.h = bg_rect.h;

   ui_draw_rect( win, &(win->grid_pos), UI_TEXT_BG, TRUE );

   if( NULL != textfield->text ) {
      ui_draw_text(
         win, &(win->grid_pos),
         GRAPHICS_TEXT_ALIGN_LEFT, fg, UI_TEXT_SIZE, textfield->text,
         textfield == win->active_control ? TRUE : FALSE, TRUE
      );
   }
}

static void ui_control_draw_label(
   struct UI_WINDOW* win, struct UI_CONTROL* label
) {
   GRAPHICS_COLOR fg = UI_LABEL_FG;

   if( NULL != label->text ) {
      ui_draw_text(
         win, &(win->grid_pos),
         GRAPHICS_TEXT_ALIGN_LEFT, fg, UI_TEXT_SIZE, label->text,
         label == win->active_control ? TRUE : FALSE, TRUE
      );
   }
}

static void ui_control_draw_button(
   struct UI_WINDOW* win, struct UI_CONTROL* button
) {
   GRAPHICS_COLOR fg = UI_BUTTON_FG;
   GRAPHICS* g = win->element;

   if( button == win->active_control ) {
      fg = UI_SELECTED_FG;
   }

   win->grid_pos.w = ui_control_get_draw_width( button );
   win->grid_pos.h = ui_control_get_draw_height( button );

   ui_draw_rect( win, &(win->grid_pos), UI_BUTTON_BG, FALSE );

   if( NULL != button->text ) {
      graphics_draw_text(
         g, win->grid_pos.x + UI_TEXT_MARGIN, win->grid_pos.y + UI_TEXT_MARGIN,
         GRAPHICS_TEXT_ALIGN_LEFT, fg, UI_TEXT_SIZE, button->text,
         button == win->active_control ? TRUE : FALSE
      );
   }
}

static void ui_control_draw_spinner(
   struct UI_WINDOW* win, struct UI_CONTROL* spinner
) {
   GRAPHICS_COLOR fg = UI_BUTTON_FG;
   GRAPHICS* g = win->element;
   bstring numbuf = NULL;
   SCAFFOLD_SIZE* num = spinner->self.attachment;

   if( spinner == win->active_control ) {
      fg = UI_SELECTED_FG;
   }

   win->grid_pos.w = ui_control_get_draw_width( spinner );
   win->grid_pos.h = ui_control_get_draw_height( spinner );

   ui_draw_rect( win, &(win->grid_pos), UI_TEXT_BG, FALSE );

   numbuf = bformat( "%d", *num );
   graphics_draw_text(
      g,
      win->grid_pos.x + UI_TEXT_MARGIN,
      /* TODO: Why does text float back up relative to background? */
      win->grid_pos.y + (/* Temp */ win->grid_pos.h) + UI_TEXT_MARGIN,
      GRAPHICS_TEXT_ALIGN_LEFT, fg, UI_TEXT_SIZE, numbuf,
      spinner == win->active_control ? TRUE : FALSE
   );
   bdestroy( numbuf );
}

static void ui_control_draw_dropdown(
   struct UI_WINDOW* win, struct UI_CONTROL* listbox
) {
   GRAPHICS_COLOR fg = UI_BUTTON_FG;
   GRAPHICS* g = win->element;
   bstring list_item = NULL;
   SCAFFOLD_SIZE_SIGNED* num = NULL;

   num = listbox->self.attachment;

   if( listbox == win->active_control ) {
      fg = UI_SELECTED_FG;
   }

#ifdef DEBUG
   list_item = vector_get( listbox->list, *num );
   list_item = bformat( "%s (%d)", bdata( list_item ), *num );
#else
   list_item = listbox->list[*num];
#endif /* DEBUG */

   win->grid_pos.w = ui_control_get_draw_width( listbox );
#ifdef DEBUG
   win->grid_pos.w += (UI_TEXT_SIZE * 5);
#endif /* DEBUG */
   win->grid_pos.h = ui_control_get_draw_height( listbox );

   ui_draw_rect( win, &(win->grid_pos), UI_TEXT_BG, FALSE );

   graphics_draw_text(
      g,
      win->grid_pos.x + UI_TEXT_MARGIN,
      /* TODO: Why does text float back up relative to background? */
      win->grid_pos.y + (/* Temp */ win->grid_pos.h) + UI_TEXT_MARGIN,
      GRAPHICS_TEXT_ALIGN_LEFT, fg, UI_TEXT_SIZE,
      list_item,
      listbox == win->active_control ? TRUE : FALSE
   );

#ifdef DEBUG
   bdestroy( list_item );
   list_item = NULL;
#endif /* DEBUG */

   ui_window_advance_grid( (struct UI_WINDOW*)win, listbox );
}

static void ui_control_draw_html(
   struct UI_WINDOW* win, struct UI_CONTROL* html
) {
   GRAPHICS_COLOR fg = UI_TEXT_FG;
   GRAPHICS* g = win->element;
   GRAPHICS_RECT bg_rect;

   if( NULL == html->self.attachment && NULL != html->text ) {
      lg_debug( __FILE__, "Parsing HTML: %b\n", html->text );
      html_tree_parse_string(
         html->text, (struct html_tree*)&(html->self.attachment) );
   }

   htmlrend_draw( g, (struct html_tree*)(html->self.attachment) );

#if 0 /* TODO ? */
   if( textfield == win->active_control ) {
      fg = UI_SELECTED_FG;
   }

   memcpy( &bg_rect, &(textfield->self.area), sizeof( GRAPHICS_RECT ) );

   ui_control_auto_size( textfield, &bg_rect, win, 1 );

   win->grid_pos.w = bg_rect.w;
   win->grid_pos.h = bg_rect.h;

   ui_draw_rect( win, &(win->grid_pos), UI_TEXT_BG, TRUE );

   if( NULL != textfield->text ) {
      ui_draw_text(
         win, &(win->grid_pos),
         GRAPHICS_TEXT_ALIGN_LEFT, fg, UI_TEXT_SIZE, textfield->text,
         textfield == win->active_control ? TRUE : FALSE, TRUE
      );
   }
#endif
}

static void ui_window_enforce_minimum_size( struct UI_WINDOW* win ) {
   GRAPHICS_RECT* largest_control = NULL;
   BOOL auto_h = FALSE;
   BOOL auto_w = FALSE;

   if( 0 >= win->area.h ) {
      win->area.h = UI_WINDOW_MIN_HEIGHT;
      auto_h = TRUE;
   }
   if( 0 >= win->area.w ) {
      win->area.w = UI_WINDOW_MIN_WIDTH;
      auto_w = TRUE;
   }

   /* Make sure the window can contain its largest control. */
   largest_control = mem_alloc( 1, GRAPHICS_RECT );
   lgc_null( largest_control );
   memcpy( largest_control, &(win->area), sizeof( GRAPHICS_RECT ) );

   #if 0
   // XXX Doesn't work!
   /* Make sure the window isn't bigger than the screen. */
   if( GRAPHICS_SCREEN_WIDTH < largest_control->w ) {
      largest_control->w = GRAPHICS_SCREEN_WIDTH - 10;
   }
   if( GRAPHICS_SCREEN_HEIGHT < largest_control->h ) {
      largest_control->h = GRAPHICS_SCREEN_HEIGHT - 10;
   }
   #endif // 0

   do {
      /* A pointer was returned, so update. */
      if(
         0 >= largest_control->w &&
         largest_control->w < win->area.w
      ) {
         largest_control->w = win->area.w;
      }
      if(
         0 >= largest_control->h &&
         largest_control->h < win->area.h
      ) {
         largest_control->h = win->area.h;
      }

      /* Make sure the control is drawn on top of the window. */
      largest_control->x = win->area.x;
      largest_control->y = win->area.y;

      /* Apply changes. */
      ui_window_transform( win, largest_control );
      mem_free( largest_control );
      ui_window_reset_grid( win );
   } while(
      NULL != (largest_control =
      hashmap_iterate( win->controls, ui_control_window_size_cb, win ))
   );
   assert( NULL == largest_control );

   if( FALSE != auto_h ) {
      win->area.h += UI_WINDOW_GRID_Y_START;
      ui_window_transform( win, &(win->area) );
   }

cleanup:
   assert( win->ui == &global_ui );
}

static void ui_window_draw_furniture( const struct UI_WINDOW* win ) {
   graphics_draw_rect(
      win->element, UI_BAR_WIDTH, UI_BAR_WIDTH,
      win->area.w - (2 * UI_BAR_WIDTH), UI_TITLEBAR_SIZE + (2 * UI_BAR_WIDTH),
      UI_TITLEBAR_BG, TRUE
   );
   graphics_draw_text(
      win->element, win->area.w / UI_BAR_WIDTH, (2 * UI_BAR_WIDTH),
      GRAPHICS_TEXT_ALIGN_CENTER,
      UI_TITLEBAR_FG, UI_TITLEBAR_SIZE, win->title, FALSE
   );
}

static void* ui_window_draw_cb( size_t idx, void* iter, void* arg ) {
   GRAPHICS* g = (GRAPHICS*)arg;
   struct UI_WINDOW* win = (struct UI_WINDOW*)iter;
   SCAFFOLD_SIZE_SIGNED
      win_x = win->area.x,
      win_y = win->area.y;
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
      control = win->first_control;
      while( NULL != control ) {
         /* After the first control, start advancing the grid. */
         if( control != win->first_control ) {
            ui_window_advance_grid( (struct UI_WINDOW*)win, control );
         }

         switch( control->type ) {
            case UI_CONTROL_TYPE_NONE: break;
            case UI_CONTROL_TYPE_LABEL: ui_control_draw_label( win, control ); break;
            case UI_CONTROL_TYPE_BUTTON: ui_control_draw_button( win, control ); break;
            case UI_CONTROL_TYPE_TEXT: ui_control_draw_textfield( win, control ); break;
#ifndef DISABLE_BACKLOG
            case UI_CONTROL_TYPE_BACKLOG: ui_control_draw_backlog( win, control ); break;
#endif /* DISABLE_BACKLOG */
            case UI_CONTROL_TYPE_SPINNER: ui_control_draw_spinner( win, control ); break;
            case UI_CONTROL_TYPE_DROPDOWN: ui_control_draw_dropdown( win, control ); break;
#ifdef USE_ITEMS
            case UI_CONTROL_TYPE_INVENTORY: ui_control_draw_inventory( win, control ); break;
#endif // USE_ITEMS
            case UI_CONTROL_TYPE_HTML: ui_control_draw_html( win, control ); break;
         }

         control = control->next_control;
      }

      win->dirty = FALSE;

      assert( win->ui == &global_ui );
   }

   /* Draw the window onto the screen. */
   if( UI_CONST_WIDTH_FULL == win->area.x ) {
      win_x = (GRAPHICS_SCREEN_WIDTH / 2) - (win->area.w / 2);
   }
   if( UI_CONST_HEIGHT_FULL == win->area.y ) {
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

   vector_iterate_r( ui->windows, ui_window_draw_cb, g );

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

void ui_window_draw_tilegrid( struct UI* ui, struct TWINDOW* twindow ) {
   SCAFFOLD_SIZE_SIGNED grid_x = 0,
      grid_y = 0;

   scaffold_assert( &global_ui == ui );

   for( grid_x = 0 ; twindow_get_max_x( twindow ) > grid_x ; grid_x++ ) {

   }
}

struct UI_WINDOW* ui_window_by_id( struct UI* ui, const bstring wid ) {
   struct UI_WINDOW* found = NULL;
   scaffold_assert( &global_ui == ui );
   found = vector_iterate( ui->windows, callback_search_windows, wid );
   return found;
}

BOOL ui_window_destroy( struct UI* ui, const bstring wid ) {
   scaffold_assert( &global_ui == ui );
   return vector_remove_cb( ui->windows, callback_free_windows, wid );
}

struct UI_CONTROL* ui_control_by_id( struct UI_WINDOW* win, const bstring id ) {
   scaffold_assert( &global_ui == win->ui );
   return hashmap_get( win->controls, id );
}

void ui_window_next_active_control( struct UI_WINDOW* win ) {
   int i, active_controls_count;
   active_controls_count = vector_count( win->controls_active );
   for( i = 0 ; active_controls_count > i ; i++ ) {
      if(
         vector_get( win->controls_active, i ) == win->active_control &&
         i + 1 < active_controls_count
      ) {
         win->active_control = vector_get( win->controls_active, i + 1 );
         goto cleanup;
      }
   }

   /* Reached the end of the list. */
   if( 0 < active_controls_count ) {
      win->active_control = vector_get( win->controls_active, 0 );
   }

cleanup:
   return;
}

void ui_message_box( struct UI* ui, const bstring message ) {
   struct UI_WINDOW* win_mbox = NULL;
   struct UI_CONTROL* control = NULL;
   bstring iter_mbox_id = NULL;

   ui_mbox_global_iter++;
   iter_mbox_id = bformat( "%s-%d", str_wid_mbox.data, ui_mbox_global_iter );

   ui_window_new( ui, win_mbox, iter_mbox_id,
      &str_wid_mbox, NULL, -1, -1,
      UI_CONST_WIDTH_FULL, UI_CONST_HEIGHT_FULL );

   ui_control_new( ui, control, message,
      UI_CONTROL_TYPE_LABEL, FALSE, TRUE, NULL, -1, -1,
      UI_CONST_WIDTH_FULL, UI_CONST_HEIGHT_FULL );
   ui_control_add( win_mbox, &str_cid_mbox_l, control );

   ui_control_new( ui, control, &str_ok,
      UI_CONTROL_TYPE_BUTTON, FALSE, TRUE, NULL, -1, -1,
      UI_CONST_WIDTH_FULL, UI_CONST_HEIGHT_FULL );
   ui_control_add( win_mbox, &str_cid_mbox_b, control );

   ui_window_push( ui, win_mbox );

cleanup:
   bdestroy( iter_mbox_id );
   return;
}


void ui_debug_window( struct UI* ui, const bstring id, bstring buffer ) {
   struct UI_WINDOW* win_debug = NULL;
   struct UI_CONTROL* control_debug = NULL;

   if( NULL == ui_window_by_id( ui, &str_wid_debug ) ) {
      ui_window_new( ui, win_debug, &str_wid_debug,
         &str_wid_debug, NULL, 10, 10,
         UI_CONST_WIDTH_FULL, UI_CONST_HEIGHT_FULL );
      ui_window_push( ui, win_debug );
   }
   win_debug = ui_window_by_id( ui, &str_wid_debug );
   control_debug = ui_control_by_id( win_debug, id );
   if( NULL == control_debug ) {
      ui_control_new( ui, control_debug, NULL,
         UI_CONTROL_TYPE_LABEL, FALSE, TRUE, buffer, 310, 110,
         UI_CONST_WIDTH_FULL, UI_CONST_HEIGHT_FULL );
      ui_control_add( win_debug, id, control_debug );
   }
cleanup:
   return;
}

#ifdef DEBUG
void ui_debug_stack( struct UI* ui ) {
   // XXX: NOLOCK
   //vector_iterate( &(ui->windows), callback_assert_windows, NULL );
}
#endif /* DEBUG */
