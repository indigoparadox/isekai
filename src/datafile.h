
#ifndef DATAFILE_H
#define DATAFILE_H

#include "tilemap.h"
#include "scaffold.h"

typedef void (*datafile_cb)(
   void* targ, bstring filename, const BYTE* tmdata, SCAFFOLD_SIZE datasize
);

#ifdef USE_EZXML
void datafile_parse_tilemap_ezxml(
   struct TILEMAP* t, const BYTE* tmdata, SCAFFOLD_SIZE datasize, BOOL local_images
);
#endif /* USE_EZXML */

void datafile_reserialize_tilemap( struct TILEMAP* t );

#endif /* DATAFILE_H */
