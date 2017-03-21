
#define INPUT_C
#include "../input.h"

#include <memory.h>
#include <ncurses.h>

void input_init( struct INPUT* p ) {
   timeout( 5 );
}

void input_get_event( struct INPUT* input ) {
   int key;

   //key = getch();

   //return key;
}

void input_shutdown( struct INPUT* input ) {
}
