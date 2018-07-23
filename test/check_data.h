
#ifndef CHECK_DATA_H
#define CHECK_DATA_H

#include <stdint.h>

#include "../src/scaffold.h"
#include "../src/libvcol.h"
#include "../src/ref.h"

#define CHECK_BEGIN_END_COLOR SCAFFOLD_COLOR_MAGENTA

typedef struct _BLOB {
   struct REF refcount;
   uint32_t sentinal_start;
   SCAFFOLD_SIZE data_len;
   uint16_t* data;
   uint32_t sentinal_end;
} BLOB;

void free_blob( BLOB* blob );
BLOB* create_blob( uint32_t sent_s, uint16_t ptrn, SCAFFOLD_SIZE c, uint32_t sent_e );

#endif /* CHECK_DATA_H */
