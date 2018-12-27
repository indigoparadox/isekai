
#ifndef STORAGE_H
#define STORAGE_H

#include <libvcol.h>

#define STORAGE_VERSION_CURRENT 1

VBOOL storage_init();
void storage_free();
bstring storage_client_get_string( bstring key );
void storage_client_set_string( bstring key, bstring str );
int storage_client_get_int( bstring key );
void storage_client_set_int( bstring key, int val );

#endif /* STORAGE_H */
