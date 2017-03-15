
#ifndef DATAFILE_H
#define DATAFILE_H

#include "tilemap.h"
#include "scaffold.h"
#include "mobile.h"

typedef void (*datafile_cb)(
   void* targ, bstring filename, const BYTE* tmdata, SCAFFOLD_SIZE datasize
);

#ifdef USE_EZXML
ezxml_t datafile_mobile_ezxml_peek_mob_id(
   BYTE* tmdata, SCAFFOLD_SIZE datasize, bstring mob_id_buffer
);
void datafile_parse_mobile_ezxml_t(
   struct MOBILE* o, ezxml_t xml_data, BOOL local_images
);
void datafile_parse_mobile_ezxml_string(
   struct MOBILE* o, BYTE* tmdata, SCAFFOLD_SIZE datasize, BOOL local_images
);
ezxml_t datafile_tilemap_ezxml_peek_lname(
   BYTE* tmdata, SCAFFOLD_SIZE datasize, bstring lname_buffer
);
void datafile_parse_tilemap_ezxml_t(
   struct TILEMAP* t, ezxml_t xml_data, BOOL local_images
);
void datafile_parse_tilemap_ezxml_string(
   struct TILEMAP* t, BYTE* tmdata, SCAFFOLD_SIZE datasize, BOOL local_images
);
#endif /* USE_EZXML */

void datafile_reserialize_tilemap( struct TILEMAP* t );

#endif /* DATAFILE_H */
