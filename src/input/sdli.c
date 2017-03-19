
#define INPUT_C
#include "../input.h"

#include <SDL/SDL.h>

void input_init( struct INPUT* p ) {
   SDL_EnableUNICODE( 1 );
   p->event = scaffold_alloc( 1, SDL_Event );
}

void input_get_event( struct INPUT* input ) {
   uint16_t key_pressed;
   SDL_Event* event;

   event = (SDL_Event*)(input->event);
   scaffold_assert( NULL != event );

   SDL_PollEvent( event );

   if( SDL_KEYDOWN == event->type ) {
      input->type = INPUT_TYPE_KEY;
      input->character = event->key.keysym.unicode & 0x00ff;
      input->scancode = event->key.keysym.scancode;
#ifdef DEBUG_KEYS
      scaffold_print_debug( &module, "Scancode: %d\n", input->scancode );
      scaffold_print_debug( &module, "Char: %d\n", input->character );
#endif /* DEBUG_KEYS */
   } else if( SDL_QUIT == event->type ) {
      input->type = INPUT_TYPE_KEY;
      input->character = 'q';
   } else {
      input->type = INPUT_TYPE_NONE;
      input->character = 0;
      input->scancode = 0;
   }
}

void input_shutdown( struct INPUT* input ) {
   scaffold_free( input->event );
}
