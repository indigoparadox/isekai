
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

int input_get_char( INPUT* input ) {
    static VECTOR hysteresis = { 0 }; /* Kind of a hack to init this. */
    int i;
    int* i_test = NULL;
    int key_out = -1;

    poll_keyboard();

    for( i = 0 ; vector_count( hysteresis ) > i ; i++ ) {
        i_test = vector_get( hysteresis, i );
        if( !key[*i_test] ) {
            vector_delete( hysteresis, i );
            break;
        }
    }

    for( i = 0 ; KEY_MAX > i ; i++ ) {
        if( key[i] ) {
            for( j = 0 ; vector_count( hysteresis ) > j ; j++ ) {
                i_test = vector_get( hysteresis, j );
                if( *i_test == i ) {
                    // XXX
                }
            }

            key_out = i;
            i_test = calloc( 1, sizeof( int ) );
            *i_test = i;
            vector_add( hysteresis, i_test );
        }
    }

    return key_out;
}
