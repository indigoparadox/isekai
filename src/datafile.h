
#ifndef DATAFILE_H
#define DATAFILE_H

#include "tilemap.h"
#include "scaffold.h"

typedef void (*datafile_cb)( void* targ, const BYTE* tmdata, size_t datasize );

void datafile_parse_tilemap( void* targ, const BYTE* tmdata, size_t datasize );
void datafile_reserialize_tilemap( TILEMAP* t );
void datafile_load_file( void* targ_struct, bstring filename, datafile_cb cb );

#endif /* DATAFILE_H */
