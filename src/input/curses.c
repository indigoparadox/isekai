
#define INPUT_C
#include "../input.h"

#include <memory.h>
#include <ncurses.h>

void input_init( struct INPUT* p ) {
#ifdef INIT_ZEROES
   memset( p, '\0', sizeof( INPUT ) );
#endif /* INIT_ZEROES */
   timeout( 5 );
}

void input_get_event( struct INPUT* input ) {
   int key;

   //key = getch();

   //return key;
}
