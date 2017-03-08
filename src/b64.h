
#ifndef B64_H
#define B64_H

#include "bstrlib/bstrlib.h"
#include "scaffold.h"

#include <stdint.h>
#include <stddef.h>

void b64_encode( void* indata, size_t indata_len, bstring outstring, ssize_t linesz );
/* void* b64_decode( size_t* outdata_len, bstring instring ); */
int b64_decode( bstring indata, unsigned char *out, size_t *outLen );

#endif /* B64_H */
