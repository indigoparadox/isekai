
#ifndef B64_H
#define B64_H

#include <stdint.h>

#include "../bstrlib/bstrlib.h"
#include "../scaffold.h"

/* #define DEBUG_B64 */

void b64_encode( void* indata, size_t indata_len, bstring outstring, size_t linesz );
BYTE* b64_decode( size_t* outdata_len, bstring instring );

#endif /* B64_H */
