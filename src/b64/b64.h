
#ifndef B64_H
#define B64_H

#include <stdint.h>

#include "../bstrlib/bstrlib.h"
#include "../scaffold.h"

/* #define DEBUG_B64 */

void b64_encode( uint8_t* indata, long indata_len, bstring outstring, int linesz );
uint8_t* b64_decode( long* outdata_len, bstring instring );

#endif /* B64_H */
