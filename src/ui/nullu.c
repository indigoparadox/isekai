
#include "../ui.h"

void ui_cleanup( struct UI* ui ) {

}

void ui_window_init(
   struct UI_WINDOW* win, struct UI* ui,
   const bstring id, const bstring title, const bstring prompt,
   GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
   GFX_COORD_PIXEL width, GFX_COORD_PIXEL height
) {

}

void ui_window_cleanup( struct UI_WINDOW* win ) {

}

void ui_window_free( struct UI_WINDOW* win ) {

}

void ui_control_init(
   struct UI_CONTROL* control,
   const bstring text, UI_CONTROL_TYPE type, BOOL can_focus, BOOL new_row,
   bstring buffer, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y,
   GFX_COORD_PIXEL width, GFX_COORD_PIXEL height
) {

}

void ui_control_add(
   struct UI_WINDOW* win, bstring id, struct UI_CONTROL* control
) {

}

void ui_control_free( struct UI_CONTROL* control ) {

}

void ui_control_set( struct UI_CONTROL* ctrl, UI_OPTION opt, UI_OPT_STATE state ) {

}

void ui_window_transform(
   struct UI_WINDOW* win, const GRAPHICS_RECT* new_area
) {

}

void ui_init( GRAPHICS* screen ) {

}

struct UI* ui_get_local() {
    return NULL;
}

void ui_set_inventory_pane_list(
   struct UI_CONTROL* inv_pane, struct VECTOR* list
) {

}

void ui_window_push( struct UI* ui, struct UI_WINDOW* win ) {

}

void ui_window_pop( struct UI* ui ) {

}

SCAFFOLD_SIZE_SIGNED ui_poll_input(
   struct UI* ui, struct INPUT* input, const bstring id
) {
    return 0;
}

void ui_draw( struct UI* ui, GRAPHICS* g ) {

}

void ui_window_draw_tilegrid( struct UI* ui, struct TWINDOW* twindow ) {

}

struct UI_WINDOW* ui_window_by_id( struct UI* ui, const bstring wid ) {
    return NULL;
}

void ui_message_box( struct UI* ui, const bstring message ) {
    
}

struct UI_CONTROL* ui_control_by_id( struct UI_WINDOW* win, const bstring id ) {
    return NULL;
}

void ui_debug_window( struct UI* ui, const bstring id, bstring buffer ) {

}

BOOL ui_window_destroy( struct UI* ui, const bstring wid ) {
    return FALSE;
}

void ui_window_next_active_control( struct UI_WINDOW* win ) {

}

void ui_window_draw_grid( struct UI* ui, struct TWINDOW* twindow ) {

}
