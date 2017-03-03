
#ifndef CHECK_DATA_H
#define CHECK_DATA_H

#include <stdint.h>

#include "../src/bstrlib/bstrlib.h"
#include "../src/ref.h"

typedef struct _BLOB {
   REF refcount;
   uint32_t sentinal_start;
   size_t data_len;
   uint16_t* data;
   uint32_t sentinal_end;
} BLOB;

BLOB* create_blob( uint32_t sent_s, uint16_t ptrn, size_t c, uint32_t sent_e );

#endif /* CHECK_DATA_H */
