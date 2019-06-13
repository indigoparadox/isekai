
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

bool storage_init();
void storage_free();
bstring storage_client_get_string( bstring store, bstring key );
void storage_client_set_string( bstring store, bstring key, bstring str );
int storage_get_single(
   bstring store,
   enum STORAGE_HALF half,
   bstring key,
   unsigned int index,
   enum STORAGE_VALUE_TYPE type,
   void** out
);
int storage_get(
   bstring store,
   enum STORAGE_HALF half,
   bstring key,
   int index,
   enum STORAGE_VALUE_TYPE type,
   struct VECTOR* out
);int storage_set_single(
   bstring store,
   enum STORAGE_HALF half,
   bstring key,
   int index,
   enum STORAGE_VALUE_TYPE type,
   void* value
);
int storage_set(
   bstring store,
   enum STORAGE_HALF half,
   bstring key,
   int index,
   enum STORAGE_VALUE_TYPE type,
   struct VECTOR* value
);

#endif /* STORAGE_H */
