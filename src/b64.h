
#ifndef B64_H
#define B64_H

#include "scaffold.h"

#include <stddef.h>

void b64_encode( void* indata, SCAFFOLD_SIZE indata_len, bstring outstring, SCAFFOLD_SIZE_SIGNED linesz );
/* void* b64_decode( SCAFFOLD_SIZE* outdata_len, bstring instring ); */
int b64_decode(bstring indata, unsigned char *out, SCAFFOLD_SIZE *outLen)
#ifdef USE_GNUC_EXTENSIONS
__attribute__ ((warn_unused_result))
#endif /* USE_GNUC_EXTENSIONS */
;

#ifdef B64_C
SCAFFOLD_MODULE( "b64.c" );
#endif /* B64_C */

#endif /* B64_H */
