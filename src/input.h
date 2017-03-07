#ifndef INPUT_H
#define INPUT_H

#include "client.h"

#include <stdint.h>

typedef enum _INPUT_TYPE {
   INPUT_TYPE_NONE,
   INPUT_TYPE_KEY,
   INPUT_TYPE_MOUSE
} INPUT_TYPE;

typedef struct _INPUT {
   void* event;
   int16_t character;
   INPUT_TYPE type;
} INPUT;

void input_init( INPUT* p );
void input_get_event( INPUT* input );

#endif /* INPUT_H */
