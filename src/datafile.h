
#ifndef DATAFILE_H
#define DATAFILE_H

#include "tilemap.h"
#include "scaffold.h"
#include "mobile.h"
#include "item.h"

typedef enum DATAFILE_TYPE {
   DATAFILE_TYPE_NONE,
   DATAFILE_TYPE_TILEMAP,
   DATAFILE_TYPE_MOBILE,
   DATAFILE_TYPE_ITEM,
   DATAFILE_TYPE_ITEM_SPRITES,
   DATAFILE_TYPE_TILESET
} DATAFILE_TYPE;

typedef void (*datafile_cb)(
   void* targ, bstring filename, const BYTE* tmdata, SCAFFOLD_SIZE datasize
);

#ifdef USE_EZXML
void datafile_parse_item_sprites_ezxml_t(
   struct ITEM_SPRITESHEET* spritesheet, ezxml_t xml_sprites,
   bstring def_path, BOOL local_images
);
void datafile_parse_item_ezxml_t(
   struct ITEM* e, ezxml_t xml_data, bstring def_path, BOOL local_images
);
ezxml_t datafile_mobile_ezxml_peek_mob_id(
   BYTE* tmdata, SCAFFOLD_SIZE datasize, bstring mob_id_buffer
);
void datafile_parse_mobile_ezxml_t(
   struct MOBILE* o, ezxml_t xml_data, bstring def_path, BOOL local_images
);
void datafile_tilemap_parse_tileset_ezxml(
   struct TILEMAP_TILESET* set, ezxml_t xml_tileset, bstring def_path,
   BOOL local_images
);
ezxml_t datafile_tilemap_ezxml_peek_lname(
   BYTE* tmdata, SCAFFOLD_SIZE datasize, bstring lname_buffer
);
void datafile_parse_tilemap_ezxml_t(
   struct TILEMAP* t, ezxml_t xml_data, bstring def_path, BOOL local_images
);
void datafile_parse_ezxml_string(
   void* object, BYTE* tmdata, SCAFFOLD_SIZE datasize, BOOL local_images,
   DATAFILE_TYPE type, bstring def_path
);
#endif /* USE_EZXML */

void datafile_reserialize_tilemap( struct TILEMAP* t );

#ifdef DATAFILE_C
SCAFFOLD_MODULE( "datafile.c" );
#endif /* DATAFILE_C */

#endif /* DATAFILE_H */
