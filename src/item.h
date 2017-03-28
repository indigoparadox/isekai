
#ifndef ITEM_H
#define ITEM_H

#include "scaffold.h"
#include "graphics.h"

typedef enum ITEM_TYPE {
   ITEM_TYPE_GENERIC = 0,
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
   ITEM_TYPE_RANGED_PISTOL,
   ITEM_TYPE_RANGED_RIFLE,
   ITEM_TYPE_RANGED_SHURIKEN,
   ITEM_TYPE_RANGED_MAX,
   ITEM_TYPE_SHIELD,
   ITEM_TYPE_CONTAINER,
   ITEM_TYPE_POTION_BOTTLE_SMALL,
   ITEM_TYPE_POTION_BOTTLE_LARGE,
   ITEM_TYPE_POTION_VIAL_SMALL,
   ITEM_TYPE_POTION_VIAL_LARGE,
   ITEM_TYPE_SCROLL,
   ITEM_TYPE_BOOK,
   ITEM_TYPE_INGOT,
   ITEM_TYPE_ORE,
   ITEM_TYPE_GEM,
   ITEM_TYPE_FOOD_FRUIT,
   ITEM_TYPE_FOOD_VEGETABLE,
   ITEM_TYPE_FOOD_MEAT,
   ITEM_TYPE_FOOD_MISC,
   ITEM_TYPE_MAX
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

#ifdef ITEM_C

const struct tagbstring item_type_strings[ITEM_TYPE_MAX] = {
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "armor_head" ),
   bsStatic( "armor_body" ),
   bsStatic( "armor_cloak" ),
   bsStatic( "armor_arms" ),
   bsStatic( "armor_feet" ),
   bsStatic( "armor_legs" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "weapon_flail" ),
   bsStatic( "weapon_sword" ),
   bsStatic( "weapon_spear" ),
   bsStatic( "weapon_wand" ),
   bsStatic( "weapon_knife" ),
   bsStatic( "weapon_hammer" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "ranged_bow" ),
   bsStatic( "ranged_pistol" ),
   bsStatic( "ranged_rifle" ),
   bsStatic( "ranged_shuriken" ),
   bsStatic( "" ),
   bsStatic( "shield" ),
   bsStatic( "container" ),
   bsStatic( "potion_bottle_small" ),
   bsStatic( "potion_bottle_large" ),
   bsStatic( "potion_vial_small" ),
   bsStatic( "potion_vial_large" ),
   bsStatic( "scroll" ),
   bsStatic( "book" ),
   bsStatic( "ingot" ),
   bsStatic( "ore" ),
   bsStatic( "gem" ),
   bsStatic( "food_fruit" ),
   bsStatic( "food_vegetable" ),
   bsStatic( "food_meat" ),
   bsStatic( "food_misc" )
};
#else

extern const struct tagbstring item_type_strings[ITEM_TYPE_MAX];

#endif /* ITEM_C */

#endif /* ITEM_H */
