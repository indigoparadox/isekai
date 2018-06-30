#ifndef UI_H
#define UI_H

#include "input.h"
#include "bstrlib/bstrlib.h"
#include "vector.h"
#include "hashmap.h"
#include "graphics.h"

#define UI_TITLEBAR_BG     GRAPHICS_COLOR_BROWN
#define UI_TITLEBAR_FG     GRAPHICS_COLOR_WHITE
#define UI_WINDOW_BG       GRAPHICS_COLOR_BLUE
#define UI_WINDOW_FG       GRAPHICS_COLOR_WHITE
#define UI_TEXT_BG         GRAPHICS_COLOR_DARK_BLUE
#define UI_TEXT_FG         GRAPHICS_COLOR_WHITE
#define UI_NICK_FG         GRAPHICS_COLOR_YELLOW
#define UI_BUTTON_BG       GRAPHICS_COLOR_DARK_CYAN
#define UI_BUTTON_FG       GRAPHICS_COLOR_WHITE
#define UI_LABEL_FG        GRAPHICS_COLOR_WHITE
#define UI_SELECTED_BG     GRAPHICS_COLOR_PURPLE
#define UI_SELECTED_FG     GRAPHICS_COLOR_YELLOW

#define UI_TEXT_MARGIN     5
#define UI_WINDOW_MARGIN   5

#define UI_BAR_WIDTH 2

#define UI_TEXT_DEF_LENGTH 30
#define UI_TEXT_DEF_CONTROL_SIZE 280

#define UI_TITLEBAR_SIZE   GRAPHICS_FONT_SIZE_8
#define UI_TEXT_SIZE       GRAPHICS_FONT_SIZE_8

#define UI_WINDOW_MIN_WIDTH 10
#define UI_WINDOW_MIN_HEIGHT 10

#define UI_WINDOW_GRID_X_START (UI_WINDOW_MARGIN)
#define UI_WINDOW_GRID_Y_START (UI_TITLEBAR_SIZE + (UI_BAR_WIDTH * 2) + UI_WINDOW_MARGIN)

#define UI_CONST_WIDTH_FULL -1
#define UI_CONST_WIDTH_HALF -2

#define UI_CONST_HEIGHT_FULL -1
#define UI_CONST_HEIGHT_HALF -2

#ifdef DEBUG
#define UI_SENTINAL_WINDOW 23232
#define UI_SENTINAL_CONTROL 45454
#endif /* DEBUG */

#define UI_INPUT_RETURN_KEY_ESC -1
#define UI_INPUT_RETURN_KEY_ENTER -2
#define UI_INPUT_RETURN_KEY_NEXT -3

struct UI;

typedef enum {
   UI_CONTROL_TYPE_NONE,
   UI_CONTROL_TYPE_TEXT,
   UI_CONTROL_TYPE_BUTTON,
   UI_CONTROL_TYPE_LABEL,
   UI_CONTROL_TYPE_CHECKBOX,
   UI_CONTROL_TYPE_TILEGRID,
   UI_CONTROL_TYPE_BACKLOG,
   UI_CONTROL_TYPE_INVENTORY,
   UI_CONTROL_TYPE_SPINNER,
   UI_CONTROL_TYPE_DROPDOWN
} UI_CONTROL_TYPE;

typedef enum {
   UI_OPTION_NEW_ROW
} UI_OPTION;

typedef enum {
   UI_OPT_STATE_DISABLE,
   UI_OPT_STATE_ENABLE
} UI_OPT_STATE;

struct UI_WINDOW {
   struct UI* ui;
   bstring title;
   struct HASHMAP controls;
   GRAPHICS* element;
   BOOL modal;
   struct UI_CONTROL* active_control;
   GRAPHICS_RECT area;
   GRAPHICS_RECT grid_pos;
   bstring id;
   struct VECTOR controls_active;
   void* attachment; /*!< Struct to draw data from. */
   SCAFFOLD_SIZE selection;
   SCAFFOLD_SIZE grid_iter;
   BOOL dirty;
   struct UI_CONTROL* first_control;
#ifdef DEBUG
   uint32_t sentinal;
#endif /* DEBUG */
};

struct UI_CONTROL {
   struct UI_WINDOW self; /* Parent Class */
   bstring text;
   BOOL can_focus;
   UI_CONTROL_TYPE type;
   struct UI_WINDOW* owner;
   BOOL borrowed_text_field;
   SCAFFOLD_SIZE_SIGNED min;
   SCAFFOLD_SIZE_SIGNED max;
   UI_OPT_STATE new_row;
   bstring* list;
   struct UI_CONTROL* next_control;
};

struct UI {
   GRAPHICS* screen_g;
   struct VECTOR windows;
};

#define ui_window_new( ui, win, id, title, prompt, x, y, w, h ) \
   win = mem_alloc( 1, struct UI_WINDOW ); \
   if( NULL == win ) { \
      scaffold_error = SCAFFOLD_ERROR_NULLPO; \
      goto cleanup; \
   } \
   ui_window_init( win, ui, id, title, prompt, x, y, w, h );

#define ui_control_new( \
      ui, control, text, type, can_focus, new_row, buffer, x, y, w, h \
) \
   control = mem_alloc( 1, struct UI_CONTROL ); \
   if( NULL == control ) { \
      scaffold_error = SCAFFOLD_ERROR_NULLPO; \
      goto cleanup; \
   } \
   ui_control_init( control, text, type, can_focus, new_row, buffer, x, y, w, h );

void ui_cleanup( struct UI* ui );
void ui_window_init(
   struct UI_WINDOW* win, struct UI* ui,
   const bstring id, const bstring title, const bstring prompt,
   GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
   GFX_COORD_PIXEL width, GFX_COORD_PIXEL height
);
void ui_window_cleanup( struct UI_WINDOW* win );
void ui_window_free( struct UI_WINDOW* win );
void ui_control_init(
   struct UI_CONTROL* control,
   const bstring text, UI_CONTROL_TYPE type, BOOL can_focus, BOOL new_row,
   bstring buffer, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
   GFX_COORD_PIXEL width, GFX_COORD_PIXEL height
);
void ui_control_add(
   struct UI_WINDOW* win, bstring id, struct UI_CONTROL* control
);
void ui_control_free( struct UI_CONTROL* control );
void ui_control_set( struct UI_CONTROL* ctrl, UI_OPTION opt, UI_OPT_STATE state );
void ui_window_transform(
   struct UI_WINDOW* win, const GRAPHICS_RECT* new_area
);
void ui_init( GRAPHICS* screen );
struct UI* ui_get_local();
void ui_set_inventory_pane_list(
   struct UI_CONTROL* inv_pane, struct VECTOR* list
);
void ui_window_push( struct UI* ui, struct UI_WINDOW* win );
void ui_window_pop( struct UI* ui );
SCAFFOLD_SIZE_SIGNED ui_poll_input(
   struct UI* ui, struct INPUT* input, const bstring id
);
void ui_draw( struct UI* ui, GRAPHICS* g );
void ui_window_draw_tilegrid( struct UI* ui, struct GRAPHICS_TILE_WINDOW* twindow );
struct UI_WINDOW* ui_window_by_id( struct UI* ui, const bstring wid );
void ui_message_box( struct UI* ui, const bstring message );
struct UI_CONTROL* ui_control_by_id( struct UI_WINDOW* win, const bstring id );
void ui_debug_window( struct UI* ui, const bstring id, bstring buffer );
BOOL ui_window_destroy( struct UI* ui, const bstring wid );
void ui_window_next_active_control( struct UI_WINDOW* win );
void ui_window_draw_grid( struct UI* ui, struct GRAPHICS_TILE_WINDOW* twindow );

#ifdef DEBUG
void ui_debug_stack( struct UI* ui );
#endif /* DEBUG */

#ifdef UI_C
SCAFFOLD_MODULE( "ui.c" );
static struct tagbstring str_wid_debug = bsStatic( "_debug" );
static struct tagbstring str_wid_mbox = bsStatic( "_messagebox" );
static struct tagbstring str_cid_mbox_l = bsStatic( "_message_lbl" );
static struct tagbstring str_cid_mbox_b = bsStatic( "_message_btn" );
static struct tagbstring str_ok = bsStatic( "OK" );
#endif /* UI_C */

#endif /* CURSES_RPG_H */
