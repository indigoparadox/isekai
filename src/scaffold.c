
#include "scaffold.h"

const char* scaffold_blank_string = "";

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
