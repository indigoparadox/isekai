
#ifndef STORAGE_H
#define STORAGE_H

#include <libvcol.h>

enum STORAGE_VALUE_TYPE {
   STORAGE_VALUE_NONE,
   STORAGE_VALUE_INT,
   STORAGE_VALUE_STRING
};

enum STORAGE_HALF {
   STORAGE_HALF_CLIENT,
   STORAGE_HALF_SERVER
};

#define STORAGE_VERSION_CURRENT 1

VBOOL storage_init();
void storage_free();
bstring storage_client_get_string( bstring store, bstring key );
void storage_client_set_string( bstring store, bstring key, bstring str );
int storage_get( bstring store, enum STORAGE_HALF half, bstring key, enum STORAGE_VALUE_TYPE type, struct VECTOR* out );

#endif /* STORAGE_H */
