#include "../input.h"

#include <allegro.h>

typedef struct {
   int keysym;
   void (*callback)( CLIENT* c, void* arg );
} INPUT_ENTRY;

static void input_key_quit( CLIENT* c, void* arg ) {

}

static const INPUT_ENTRY input_vtable[] = {
   {KEY_Q, input_key_quit},
   {-1, NULL}
};

void input_init( INPUT* p ) {
#ifdef INIT_ZEROES
   memset( p, '\0', sizeof( INPUT ) );
#endif /* INIT_ZEROES */

   install_keyboard();
}

int input_execute( INPUT* input ) {
   const INPUT_ENTRY* input_vtable_iter = input_vtable;

   poll_keyboard();

   while( NULL != input_vtable_iter ) {
      if( key[input_vtable_iter->keysym] ) {
         input_vtable_iter->callback( input->client, NULL );
      }

      input_vtable_iter++;
   }

   return FALSE;
}

int16_t input_get_char( INPUT* input ) {
   int16_t key_out = -1;

   poll_keyboard();

   if( keypressed() ) {
      key_out = readkey();
   }

   if( 0 <= key_out ) {
      return key_out & 0xff;
   } else {
      return 0;
   }
}
