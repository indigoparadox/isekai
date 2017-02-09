
#ifndef SCAFFOLD_H
#define SCAFFOLD_H

#include <stdint.h>
#include <stdio.h>

#ifndef BOOL
typedef uint8_t BOOL;
#endif /* BOOL */

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

typedef enum {
    SCAFFOLD_ERROR_NONE = 0,
    SCAFFOLD_ERROR_MISC = 1,
    SCAFFOLD_ERROR_NULLPO = 2,
    SCAFFOLD_ERROR_OUTOFBOUNDS = 3,
    SCAFFOLD_ERROR_NEGATIVE = 4,
} SCAFFOLD_ERROR;

#define scaffold_print_info( ... ) fprintf( stdout, __FILE__ ": " __VA_ARGS__ );
#define scaffold_print_debug( ... ) fprintf( stdout, __FILE__ ": " __VA_ARGS__ );
#define scaffold_print_error( ... ) fprintf( stderr, __FILE__ ": " __VA_ARGS__ );

#define scaffold_check_null( pointer ) \
    if( NULL == pointer ) { \
        scaffold_error = SCAFFOLD_ERROR_NULLPO; \
        scaffold_print_error( "Scaffold: Null pointer on line: %d\n", __LINE__ ); \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_bounds( index, bound ) \
    if( index >= bound ) { \
        scaffold_error = SCAFFOLD_ERROR_OUTOFBOUNDS; \
        scaffold_print_error( "Scaffold: Out of bounds on line: %d\n", __LINE__ ); \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

#define scaffold_check_negative( value ) \
    if( 0 > value ) { \
        scaffold_error = SCAFFOLD_ERROR_NEGATIVE; \
        scaffold_print_error( "Scaffold: Bad negative on line: %d\n", __LINE__ ); \
        goto cleanup; \
    } else { \
        scaffold_error = SCAFFOLD_ERROR_NONE; \
    }

uint8_t scaffold_error;

#endif /* SCAFFOLD_H */
