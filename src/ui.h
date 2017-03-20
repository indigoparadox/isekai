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
#define UI_BUTTON_BG       GRAPHICS_COLOR_DARK_CYAN
#define UI_BUTTON_FG       GRAPHICS_COLOR_WHITE
#define UI_LABEL_FG        GRAPHICS_COLOR_WHITE

#define UI_TEXT_MARGIN     5
#define UI_WINDOW_MARGIN   10

#define UI_TEXT_DEF_LENGTH 30

#define UI_TITLEBAR_SIZE   GRAPHICS_FONT_SIZE_8
#define UI_TEXT_SIZE       GRAPHICS_FONT_SIZE_10

struct UI;

struct UI_WINDOW {
   struct UI* ui;
   bstring title;
   struct HASHMAP controls;
   GRAPHICS* element;
   BOOL modal;
   struct UI_CONTROL* active_control;
   SCAFFOLD_SIZE_SIGNED x;
   SCAFFOLD_SIZE_SIGNED y;
   SCAFFOLD_SIZE_SIGNED width;
   SCAFFOLD_SIZE_SIGNED height;
   SCAFFOLD_SIZE_SIGNED grid_x;
   SCAFFOLD_SIZE_SIGNED grid_y;
   SCAFFOLD_SIZE_SIGNED grid_previous_button;
   bstring id;
};

typedef enum {
   UI_CONTROL_TYPE_NONE,
   UI_CONTROL_TYPE_TEXT,
   UI_CONTROL_TYPE_BUTTON,
   UI_CONTROL_TYPE_LABEL
} UI_CONTROL_TYPE;

typedef enum UI_WINDOW_TYPE {
   UI_WINDOW_TYPE_NONE,
   UI_WINDOW_TYPE_OK,
   UI_WINDOW_TYPE_YN,
   UI_WINDOW_TYPE_SIMPLE_TEXT
} UI_WINDOW_TYPE;

struct UI_CONTROL {
   struct UI_WINDOW self; /* Parent Class */
   bstring text;
   BOOL can_focus;
   UI_CONTROL_TYPE type;
   struct UI_WINDOW* owner;
   BOOL borrowed_text_field;
};

struct UI {
   struct VECTOR windows;
};

#define ui_window_new( ui, win, type, id, title, prompt, x, y, w, h ) \
   win = (struct UI_WINDOW*)calloc( 1, sizeof( struct UI_WINDOW ) ); \
   if( NULL == win ) { \
      scaffold_error = SCAFFOLD_ERROR_NULLPO; \
      goto cleanup; \
   } \
   ui_window_init( win, ui, type, id, title, prompt, x, y, w, h );

#define ui_control_new( \
      ui, control, text, type, can_focus, buffer, x, y, w, h \
) \
   control = (struct UI_CONTROL*)calloc( 1, sizeof( struct UI_CONTROL ) ); \
   if( NULL == control ) { \
      scaffold_error = SCAFFOLD_ERROR_NULLPO; \
      goto cleanup; \
   } \
   ui_control_init( control, text, type, can_focus, buffer, x, y, w, h );

void ui_cleanup( struct UI* ui );
void ui_window_init(
   struct UI_WINDOW* win, struct UI* ui, UI_WINDOW_TYPE type,
   const bstring id, const bstring title, const bstring prompt,
   SCAFFOLD_SIZE_SIGNED x, SCAFFOLD_SIZE_SIGNED y,
   SCAFFOLD_SIZE_SIGNED width, SCAFFOLD_SIZE_SIGNED height
);
void ui_window_cleanup( struct UI_WINDOW* win );
void ui_window_free( struct UI_WINDOW* win );
void ui_control_init(
   struct UI_CONTROL* control,
   const bstring text, UI_CONTROL_TYPE type, BOOL can_focus, bstring buffer,
   SCAFFOLD_SIZE_SIGNED x, SCAFFOLD_SIZE_SIGNED y,
   SCAFFOLD_SIZE_SIGNED width, SCAFFOLD_SIZE_SIGNED height
);
void ui_control_add(
   struct UI_WINDOW* win, bstring id, struct UI_CONTROL* control
);
void ui_control_free( struct UI_CONTROL* control );
void ui_window_transform(
   struct UI_WINDOW* win, SCAFFOLD_SIZE_SIGNED x, SCAFFOLD_SIZE_SIGNED y,
   SCAFFOLD_SIZE_SIGNED width, SCAFFOLD_SIZE_SIGNED height
);
void ui_init( struct UI* ui, GRAPHICS* screen );
void ui_window_push( struct UI* ui, struct UI_WINDOW* win );
void ui_window_pop( struct UI* ui );
SCAFFOLD_SIZE_SIGNED ui_poll_input(
   struct UI* ui, struct INPUT* input, bstring buffer, const bstring id
);
void ui_draw( struct UI* ui, GRAPHICS* g );
struct UI_WINDOW* ui_window_by_id( struct UI* ui, const bstring wid );

#ifdef UI_C
SCAFFOLD_MODULE( "ui.c" );
#endif /* UI_C */

#endif /* CURSES_RPG_H */
