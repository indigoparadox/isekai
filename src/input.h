#ifndef INPUT_H
#define INPUT_H

#include "scaffold.h"
#include "client.h"

typedef enum INPUT_SCANCODE {
   INPUT_SCANCODE_NULL = 0,
#ifdef USE_ALLEGRO
   INPUT_SCANCODE_ESC = 59,
   INPUT_SCANCODE_BACKSPACE = 63,
   INPUT_SCANCODE_ENTER = 67,
   INPUT_SCANCODE_TAB = 64
#elif defined( USE_SDL )
   INPUT_SCANCODE_BACKSPACE = 22,
   INPUT_SCANCODE_ENTER = 36,
   INPUT_SCANCODE_ESC = 9,
   INPUT_SCANCODE_TAB = 23
#else
   INPUT_SCANCODE_BACKSPACE = 0,
   INPUT_SCANCODE_ENTER = 0,
   INPUT_SCANCODE_ESC = 0,
   INPUT_SCANCODE_TAB = 0
#endif
} INPUT_SCANCODE;

typedef enum _INPUT_TYPE {
   INPUT_TYPE_NONE,
   INPUT_TYPE_KEY,
   INPUT_TYPE_MOUSE,
   INPUT_TYPE_CLOSE
} INPUT_TYPE;

typedef enum INPUT_ASSIGNMENT {
   INPUT_ASSIGNMENT_ATTACK = ' ',
   INPUT_ASSIGNMENT_LEFT = 'a',
   INPUT_ASSIGNMENT_INV = 'e',
   INPUT_ASSIGNMENT_DOWN = 's',
   INPUT_ASSIGNMENT_RIGHT = 'd',
   INPUT_ASSIGNMENT_QUIT = 'q',
   INPUT_ASSIGNMENT_UP = 'w'
} INPUT_ASSIGNMENT;

struct INPUT {
   void* event;
   uint16_t character;
   uint16_t scancode;
   INPUT_TYPE type;
   uint16_t repeat;
};

void input_init( struct INPUT* p );
void input_get_event( struct INPUT* input );
void input_clear_buffer( struct INPUT* input );
void input_shutdown( struct INPUT* input );

#ifdef INPUT_C
SCAFFOLD_MODULE( "input.c" );
#endif /* INPUT_C */

#endif /* INPUT_H */
