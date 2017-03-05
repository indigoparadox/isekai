#include "../input.h"

#include "../client.h"

#include <allegro.h>

extern CLIENT* main_client;

typedef struct {
   int keysym;
   void (*callback)( CLIENT* c, void* arg );
} INPUT_ENTRY;

static void input_close_allegro_window() {
   scaffold_trace_path = SCAFFOLD_TRACE_CLIENT;
   client_stop( main_client );
}

void input_init( INPUT* p ) {
   install_keyboard();

   set_close_button_callback( input_close_allegro_window );
}

void input_get_event( INPUT* input ) {
   poll_keyboard();

   if( keypressed() ) {
      input->type = INPUT_TYPE_KEY;
      input->character = readkey() & 0xff;
   } else {
      input->type = INPUT_TYPE_NONE;
      input->character = 0;
   }
}
