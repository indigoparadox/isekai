#ifndef UI_H
#define UI_H

#include "input.h"
#include "bstrlib/bstrlib.h"
#include "vector.h"
#include "hashmap.h"
#include "graphics.h"

struct UI;

struct UI_WINDOW {
   struct UI* ui;
   bstring title;
   struct HASHMAP controls;
   GRAPHICS element;
   BOOL modal;
   struct UI_CONTROL* active_control;
   SCAFFOLD_SIZE x;
   SCAFFOLD_SIZE y;
   SCAFFOLD_SIZE width;
   SCAFFOLD_SIZE height;
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

#define ui_window_new( win, ui, type, id, title, prompt, x, y, w, h ) \
   win = (struct UI_WINDOW*)calloc( 1, sizeof( struct UI_WINDOW ) ); \
   scaffold_check_null( win ); \
   ui_window_init( win, ui, type, id, title, prompt, x, y, w, h );

#define ui_control_new( \
      control, win, text, type, can_focus, x, y, w, h \
) \
   control = (struct UI_CONTROL*)calloc( 1, sizeof( struct UI_CONTROL ) ); \
   scaffold_check_null( control ); \
   ui_control_init( control, win, text, type, can_focus, x, y, w, h );

void ui_window_init(
   struct UI_WINDOW* win, struct UI* ui, UI_WINDOW_TYPE type,
   const bstring id, const bstring title, const bstring prompt,
   SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE width, SCAFFOLD_SIZE height
);
void ui_window_cleanup( struct UI_WINDOW* win );
void ui_window_free( struct UI_WINDOW* win );
void ui_control_init(
   struct UI_CONTROL* control, struct UI_WINDOW* win,
   const bstring text, UI_CONTROL_TYPE type, BOOL can_focus,
   SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE width, SCAFFOLD_SIZE height
);
void ui_control_add(
   struct UI_WINDOW* win, bstring id, struct UI_CONTROL* control
);
void ui_control_free( struct UI_CONTROL* control );
void ui_window_transform(
   struct UI_WINDOW* win, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE width, SCAFFOLD_SIZE height
);
void ui_init( struct UI* ui, GRAPHICS* screen );
void ui_window_push( struct UI* ui, struct UI_WINDOW* win );
void ui_window_pop( struct UI* ui );
SCAFFOLD_SIZE ui_poll_input(
   struct UI* ui, struct INPUT* input, bstring buffer, const bstring id
);
void ui_draw( struct UI* ui, GRAPHICS* g );
struct UI_WINDOW* ui_window_by_id( struct UI* ui, const bstring wid );

#endif /* CURSES_RPG_H */
