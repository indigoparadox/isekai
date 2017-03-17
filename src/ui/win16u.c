
#define UI_C
#include "../ui.h"


void ui_window_init(
   struct UI_WINDOW* win, struct UI* ui, UI_WINDOW_TYPE type,
   const bstring id, const bstring title, const bstring prompt,
   SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE width, SCAFFOLD_SIZE height
) {
}

void ui_window_cleanup( struct UI_WINDOW* win ) {
}

void ui_window_free( struct UI_WINDOW* win ) {
}

void ui_control_init(
   struct UI_CONTROL* control, struct UI_WINDOW* win,
   const bstring text, UI_CONTROL_TYPE type, BOOL can_focus, bstring buffer,
   SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE width, SCAFFOLD_SIZE height
) {
}

void ui_control_add(
   struct UI_WINDOW* win, bstring id, struct UI_CONTROL* control
) {
}

void ui_control_free( struct UI_CONTROL* control ) {
}

void ui_window_transform(
   struct UI_WINDOW* win, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y,
   SCAFFOLD_SIZE width, SCAFFOLD_SIZE height
) {
}

void ui_init( struct UI* ui, GRAPHICS* screen ) {
}

void ui_window_push( struct UI* ui, struct UI_WINDOW* win ) {
}

void ui_window_pop( struct UI* ui ) {
}

SCAFFOLD_SIZE ui_poll_input(
   struct UI* ui, struct INPUT* input, bstring buffer, const bstring id
) {
}

void ui_draw( struct UI* ui, GRAPHICS* g ) {
}

struct UI_WINDOW* ui_window_by_id( struct UI* ui, const bstring wid ) {
}
