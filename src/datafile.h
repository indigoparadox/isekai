
#ifndef DATAFILE_H
#define DATAFILE_H

typedef enum DATAFILE_TYPE {
   DATAFILE_TYPE_INVALID = -1,
   DATAFILE_TYPE_MISC = 0,
   DATAFILE_TYPE_TILEMAP,
   DATAFILE_TYPE_TILESET,
   DATAFILE_TYPE_TILESET_TILES,
   DATAFILE_TYPE_MOBILE,
   DATAFILE_TYPE_MOBILE_SPRITES,
   DATAFILE_TYPE_ITEM_CATALOG,
   DATAFILE_TYPE_ITEM_CATALOG_SPRITES
} DATAFILE_TYPE;

#include "tilemap.h"
#include "scaffold.h"
#include "mobile.h"
#ifdef USE_ITEMS
#include "item.h"
#endif /* USE_ITEMS */

typedef void (*datafile_cb)(
   void* targ, bstring filename, const BYTE* tmdata, SCAFFOLD_SIZE datasize
);

struct MOBILE;
struct TILEMAP;
struct TILEMAP_TILESET;

#ifdef USE_EZXML
#ifdef USE_ITEMS
void datafile_parse_item_sprites_ezxml_t(
   struct ITEM_SPRITESHEET* spritesheet, ezxml_t xml_sprites,
   bstring def_path, BOOL local_images
);
void datafile_parse_item_ezxml_t(
   struct ITEM* e, ezxml_t xml_data, bstring def_path, BOOL local_images
);
#endif /* USE_ITEMS */
ezxml_t datafile_mobile_ezxml_peek_mob_id(
   BYTE* tmdata, SCAFFOLD_SIZE datasize, bstring mob_id_buffer
);
void datafile_parse_mobile_ezxml_t(
   struct MOBILE* o, ezxml_t xml_data, bstring def_path, BOOL local_images
);
BOOL datafile_tilemap_parse_tileset_ezxml(
   struct TILEMAP_TILESET* set, ezxml_t xml_tileset, bstring def_path,
   BOOL local_images
);
ezxml_t datafile_tilemap_ezxml_peek_lname(
   BYTE* tmdata, SCAFFOLD_SIZE datasize, bstring lname_buffer
);
SCAFFOLD_SIZE datafile_parse_tilemap_ezxml_t(
   struct TILEMAP* t, ezxml_t xml_data, bstring def_path, BOOL local_images
);
void datafile_parse_ezxml_string(
   void* object, BYTE* tmdata, SCAFFOLD_SIZE datasize, BOOL local_images,
   DATAFILE_TYPE type, bstring def_path
);
#endif /* USE_EZXML */


void datafile_handle_stream(
   DATAFILE_TYPE type, bstring filename, BYTE* data, size_t length,
   struct CLIENT* c
);

void datafile_reserialize_tilemap( struct TILEMAP* t );

#endif /* DATAFILE_H */
