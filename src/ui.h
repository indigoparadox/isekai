#ifndef UI_H
#define UI_H

#include "vector.h"
#include "graphics.h"
#include "bstrlib/bstrlib.h"

struct UI;

struct UI_WINDOW {
   struct UI* ui;
   uint32_t id;
   bstring title;
   GRAPHICS element;
};

typedef enum {
   UI_CONTROL_TYPE_NONE,
   UI_CONTROL_TYPE_TEXT,
   UI_CONTROL_TYPE_BUTTON
} UI_CONTROL_TYPE;

struct UI_CONTROL {
   struct UI_WINDOW self; /* Parent Class */
   bstring text;
   UI_CONTROL_TYPE type;
};

struct UI {
   GRAPHICS* screen;
   struct VECTOR windows;
};

void ui_window_init( struct UI_WINDOW* win, struct UI* ui, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE width, SCAFFOLD_SIZE height );
void ui_window_transform( struct UI_WINDOW* win, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y, SCAFFOLD_SIZE width, SCAFFOLD_SIZE height );
void ui_init( struct UI* ui, GRAPHICS* screen );
void ui_add_window( struct UI* ui, struct UI_WINDOW* win );
void ui_lock_windows( struct UI* ui, BOOL lock );

#endif /* CURSES_RPG_H */
