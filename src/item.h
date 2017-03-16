
#ifndef ITEM_H
#define ITEM_H

#include "scaffold.h"
#include "graphics.h"

typedef enum ITEM_TYPE {
   ITEM_TYPE_GENERIC,
   ITEM_TYPE_ARMOR_MIN,
   ITEM_TYPE_ARMOR_HEAD,
   ITEM_TYPE_ARMOR_BODY,
   ITEM_TYPE_ARMOR_CLOAK,
   ITEM_TYPE_ARMOR_ARMS,
   ITEM_TYPE_ARMOR_FEET,
   ITEM_TYPE_ARMOR_LEGS,
   ITEM_TYPE_ARMOR_MAX,
   ITEM_TYPE_WEAPON_MIN,
   ITEM_TYPE_WEAPON_FLAIL,
   ITEM_TYPE_WEAPON_SWORD,
   ITEM_TYPE_WEAPON_SPEAR,
   ITEM_TYPE_WEAPON_WAND,
   ITEM_TYPE_WEAPON_KNIFE,
   ITEM_TYPE_WEAPON_HAMMER,
   ITEM_TYPE_WEAPON_MAX,
   ITEM_TYPE_RANGED_MIN,
   ITEM_TYPE_RANGED_BOW,
   ITEM_TYPE_RANGED_GUN,
   ITEM_TYPE_RANGED_SHURIKEN,
   ITEM_TYPE_RANGED_MAX,
   ITEM_TYPE_SHIELD,
   ITEM_TYPE_CONTAINER,
   ITEM_TYPE_POTION,
   ITEM_TYPE_SCROLL,
   ITEM_TYPE_BOOK,
   ITEM_TYPE_INGOT,
   ITEM_TYPE_ORE,
   ITEM_TYPE_GEM,
   ITEM_TYPE_FOOD
} ITEM_TYPE;


union ITEM_CONTENT {
   bstring book_text;
   struct VECTOR* container;
};

struct ITEM {
   SCAFFOLD_SIZE serial;
   bstring display_name;
   bstring def_filename;
   bstring sprites_filename;
   GRAPHICS* sprites;
   ITEM_TYPE type;
   union ITEM_CONTENT content;
};

void item_init( struct ITEM* e );
void item_set_contents( struct ITEM* t, union ITEM_CONTENT content );

#endif /* ITEM_H */
