#ifndef UI_H
#define UI_H

#include "vector.h"
#include "graphics.h"
#include "bstrlib/bstrlib.h"

typedef struct _UI UI;

typedef struct {
   UI* ui;
   uint32_t id;
   bstring title;
   GRAPHICS element;
} UI_WINDOW;

typedef enum {
   UI_CONTROL_TYPE_NONE,
   UI_CONTROL_TYPE_TEXT,
   UI_CONTROL_TYPE_BUTTON,
} UI_CONTROL_TYPE;

typedef struct {
   UI_WINDOW self; /* Parent Class */
   bstring text;
   UI_CONTROL_TYPE type;
} UI_CONTROL;

typedef struct _UI {
   GRAPHICS* screen;
   VECTOR windows;
} UI;

void ui_window_init( UI_WINDOW* win, UI* ui, gu x, gu y, gu width, gu height );
void ui_init( UI* ui, GRAPHICS* screen );
void ui_add_window( UI* ui, UI_WINDOW* win );
void ui_lock_windows( UI* ui, BOOL lock );

#endif /* CURSES_RPG_H */
