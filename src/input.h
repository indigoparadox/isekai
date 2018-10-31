#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

typedef enum _INPUT_TYPE {
   INPUT_TYPE_NONE,
   INPUT_TYPE_KEY,
   INPUT_TYPE_MOUSE,
   INPUT_TYPE_CLOSE
} INPUT_TYPE;

struct INPUT {
   void* event;
   uint16_t character;
   uint16_t scancode;
   INPUT_TYPE type;
   uint16_t repeat;
};

#include "scaffold.h"
#include "client.h"

#ifdef USE_ALLEGRO
#include "allegro/keyboard.h"
#endif /* USE_ALLEGRO */

typedef enum INPUT_SCANCODE {
   INPUT_SCANCODE_NULL = 0,
#ifdef USE_ALLEGRO
   INPUT_SCANCODE_ESC = __allegro_KEY_ESC,
   INPUT_SCANCODE_BACKSPACE = __allegro_KEY_BACKSPACE,
   INPUT_SCANCODE_ENTER = __allegro_KEY_ENTER,
   INPUT_SCANCODE_TAB = __allegro_KEY_TAB,
   INPUT_SCANCODE_LEFT = __allegro_KEY_LEFT,
   INPUT_SCANCODE_RIGHT = __allegro_KEY_RIGHT,
   INPUT_SCANCODE_UP = __allegro_KEY_UP,
   INPUT_SCANCODE_DOWN = __allegro_KEY_DOWN
#elif defined( USE_SDL )
   INPUT_SCANCODE_BACKSPACE = 22,
   INPUT_SCANCODE_ENTER = 36,
   INPUT_SCANCODE_ESC = 9,
   INPUT_SCANCODE_TAB = 23,
   INPUT_SCANCODE_LEFT = 82,
   INPUT_SCANCODE_RIGHT = 83,
   INPUT_SCANCODE_UP = 84,
   INPUT_SCANCODE_DOWN = 85
#else
   INPUT_SCANCODE_BACKSPACE = 1,
   INPUT_SCANCODE_ENTER = 2,
   INPUT_SCANCODE_ESC = 3,
   INPUT_SCANCODE_TAB = 4,
   INPUT_SCANCODE_LEFT = 5,
   INPUT_SCANCODE_RIGHT = 6,
   INPUT_SCANCODE_UP = 7,
   INPUT_SCANCODE_DOWN = 8
#endif
} INPUT_SCANCODE;

typedef enum INPUT_ASSIGNMENT {
   INPUT_ASSIGNMENT_ATTACK = ' ',
   INPUT_ASSIGNMENT_LEFT = 'a',
   INPUT_ASSIGNMENT_INV = 'e',
   INPUT_ASSIGNMENT_DOWN = 's',
   INPUT_ASSIGNMENT_RIGHT = 'd',
   INPUT_ASSIGNMENT_QUIT = 'q',
   INPUT_ASSIGNMENT_UP = 'w'
} INPUT_ASSIGNMENT;

void input_init( struct INPUT* p );
void input_get_event( struct INPUT* input );
void input_clear_buffer( struct INPUT* input );
void input_shutdown( struct INPUT* input );

#endif /* INPUT_H */
