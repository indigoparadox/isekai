
#include "scaffold.h"

const char* scaffold_blank_string = "";
struct tagbstring scaffold_space_string = bsStatic( " " );

BOOL scaffold_is_numeric( bstring line ) {
    int i;
    BOOL is_numeric = TRUE;

    for( i = 0 ; blength( line ) > i ; i++ ) {
        if( !isdigit( bdata( line )[i] ) ) {
            is_numeric = FALSE;
            break;
        }
    }

    return is_numeric;
}

bstring scaffold_pop_string( struct bstrList* list ) {
    bstring popped = list->entry[0];
    int i;

    for( i = 1 ; list->qty > i ; i++ ) {
        list->entry[i - 1] = list->entry[i];
    }

    list->qty--;

    return popped;
}
