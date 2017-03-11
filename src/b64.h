
#ifndef B64_H
#define B64_H

#include "bstrlib/bstrlib.h"
#include "scaffold.h"

#include <stdint.h>
#include <stddef.h>

void b64_encode( void* indata, SCAFFOLD_SIZE indata_len, bstring outstring, SCAFFOLD_SIZE_SIGNED linesz );
/* void* b64_decode( SCAFFOLD_SIZE* outdata_len, bstring instring ); */
int b64_decode(bstring indata, unsigned char *out, SCAFFOLD_SIZE *outLen)
#ifdef __GNUC__
__attribute__ ((warn_unused_result))
#endif /* __GNUC__ */
;

#endif /* B64_H */