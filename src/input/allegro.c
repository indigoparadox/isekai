
#define INPUT_C
#include "../input.h"

#include <allegro.h>

#include "../client.h"
#include "../server.h"
#include "../proto.h"

extern struct CLIENT* main_client;
extern SERVER* main_server;

typedef struct {
   int keysym;
   void (*callback)( struct CLIENT* c, void* arg );
} INPUT_ENTRY;

static void input_close_allegro_window() {
   scaffold_set_client();
   proto_client_stop( main_client );
}

void input_init( struct INPUT* p ) {
   install_keyboard();
   set_close_button_callback( input_close_allegro_window );
}

void input_get_event( struct INPUT* input ) {
   uint16_t key_pressed;

   poll_keyboard();

   if( keypressed() ) {
      key_pressed = readkey();
      input->type = INPUT_TYPE_KEY;
      input->character = key_pressed & 0xff;
      input->scancode = (key_pressed & 0xff00) >> 8;
#ifdef DEBUG_KEYS
      scaffold_print_debug( &module, "Scancode: %d\n", input->scancode );
#endif /* DEBUG_KEYS */
   } else {
      input->type = INPUT_TYPE_NONE;
      input->character = 0;
   }
}
