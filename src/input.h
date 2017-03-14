#ifndef INPUT_H
#define INPUT_H

#include "scaffold.h"
#include "client.h"

typedef enum INPUT_SCANCODE {
   INPUT_SCANCODE_BACKSPACE = 63,
   INPUT_SCANCODE_ENTER = 67
} INPUT_SCANCODE;

typedef enum _INPUT_TYPE {
   INPUT_TYPE_NONE,
   INPUT_TYPE_KEY,
   INPUT_TYPE_MOUSE
} INPUT_TYPE;

struct INPUT {
   void* event;
   uint16_t character;
   uint16_t scancode;
   INPUT_TYPE type;
};

void input_init( struct INPUT* p );
void input_get_event( struct INPUT* input );

#endif /* INPUT_H */
