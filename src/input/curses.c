
#include "../input.h"

#include <memory.h>
#include <ncurses.h>

void input_init( INPUT* p ) {
    memset( p, '\0', sizeof( INPUT ) );
}

int input_get_char( INPUT* input ) {
    int key;

    key = getch();

    return key;
}