
#define INPUT_C
#include "../input.h"

#include <allegro.h>

#include "../client.h"
#include "../server.h"
#include "../proto.h"

extern struct CLIENT* main_client;
extern struct SERVER* main_server;

volatile BOOL window_closed = FALSE;

typedef struct {
   int keysym;
   void (*callback)( struct CLIENT* c, void* arg );
} INPUT_ENTRY;

static void input_close_allegro_window() {
   /* scaffold_set_client();
   proto_client_stop( main_client ); */
   window_closed = TRUE;
}

void input_init( struct INPUT* p ) {
   install_keyboard();
   set_close_button_callback( input_close_allegro_window );
}

void input_get_event( struct INPUT* input ) {
   uint16_t key_pressed;

   poll_keyboard();

   if( FALSE != window_closed ) {
      input->type = INPUT_TYPE_CLOSE;
   } else if( keypressed() ) {
      key_pressed = readkey();
      input->type = INPUT_TYPE_KEY;
      input->character = key_pressed & 0xff;
      input->scancode = (key_pressed & 0xff00) >> 8;
#ifdef DEBUG_KEYS
      lg_debug( __FILE__, "Scancode: %d\n", input->scancode );
#endif /* DEBUG_KEYS */
   } else {
      input->type = INPUT_TYPE_NONE;
      input->character = 0;
      input->scancode = 0;
   }
}

void input_clear_buffer( struct INPUT* input ) {
   clear_keybuf();
}

void input_shutdown( struct INPUT* input ) {
}
