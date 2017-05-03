
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
   ITEM_TYPE_WEAPON_GAUNTLET,
   ITEM_TYPE_WEAPON_MAX,
   ITEM_TYPE_RANGED_MIN,
   ITEM_TYPE_RANGED_BOW,
   ITEM_TYPE_RANGED_PISTOL,
   ITEM_TYPE_RANGED_RIFLE,
   ITEM_TYPE_RANGED_MAX,
   ITEM_TYPE_AMMO_MIN,
   ITEM_TYPE_AMMO_ARROW,
   ITEM_TYPE_AMMO_TOMAHAWK,
   ITEM_TYPE_AMMO_SHURIKEN,
   ITEM_TYPE_AMMO_KUNAI,
   ITEM_TYPE_AMMO_MAX,
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

struct ITEM_SPRITE {
   bstring display_name;
   ITEM_TYPE type;
};

struct ITEM_SPRITESHEET {
   struct REF refcount;
   /* bstring name; */
   GRAPHICS* sprites_image;
   bstring sprites_filename;
   BOOL sprites_requested;
   GFX_COORD_PIXEL spritewidth;
   GFX_COORD_PIXEL spriteheight;
   struct CLIENT* client_or_server;
   struct VECTOR sprites;
};

struct ITEM {
   struct REF refcount;
   BIG_SERIAL serial;
   /* struct ITEM_SPRITE* sprite;
   struct ITEM_SPRITESHEET* catalog; */
   bstring catalog_name;
   SCAFFOLD_SIZE sprite_id;
   bstring display_name;
   union ITEM_CONTENT content;
   SCAFFOLD_SIZE count;
   struct CLIENT* client_or_server;
};

#define item_new( e, serial, display_name, count, catalog, sprite, c ) \
    e = mem_alloc( 1, struct ITEM ); \
    scaffold_check_null( e ); \
    item_init( e, serial, display_name, count, catalog, sprite, c );

#define item_random_new( e, type, item_catalog, c ) \
    e = mem_alloc( 1, struct ITEM ); \
    scaffold_check_null( e ); \
    item_random_init( e, type, item_catalog, c );

#define item_spritesheet_new( catalog, name, client_or_server ) \
    catalog = mem_alloc( 1, struct ITEM_SPRITESHEET ); \
    scaffold_check_null( catalog ); \
    item_spritesheet_init( catalog, name, client_or_server );

void item_init(
   struct ITEM* e, BIG_SERIAL serial, const bstring display_name,
   SCAFFOLD_SIZE count, bstring catalog_name,
   SCAFFOLD_SIZE sprite_id, struct CLIENT* c
);
void item_random_init(
   struct ITEM* e, ITEM_TYPE type, const bstring catalog_name,
   struct CLIENT* c
);
void item_free( struct ITEM* e );
void item_sprite_free( struct ITEM_SPRITE* sprite );
void item_spritesheet_init(
   struct ITEM_SPRITESHEET* catalog,
   const bstring name,
   struct CLIENT* client_or_server
);
void item_spritesheet_free( struct ITEM_SPRITESHEET* catalog );
struct ITEM_SPRITE* item_spritesheet_get_sprite(
   struct ITEM_SPRITESHEET* catalog,
   SCAFFOLD_SIZE sprite_id
);
ITEM_TYPE item_type_from_c( const char* c_string );
SCAFFOLD_SIZE item_random_sprite_id_of_type(
   ITEM_TYPE type, struct ITEM_SPRITESHEET* catalog
);
void item_draw_ortho(
   struct ITEM* e, GFX_COORD_PIXEL x, GFX_COORD_PIXEL y, GRAPHICS* g
);
void item_set_contents( struct ITEM* e, union ITEM_CONTENT content );
BOOL item_is_container( struct ITEM* e );

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
   bsStatic( "weapon_gauntlet" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "ranged_bow" ),
   bsStatic( "ranged_pistol" ),
   bsStatic( "ranged_rifle" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "ammo_arrow" ),
   bsStatic( "ammo_tomahawk" ),
   bsStatic( "ammo_shuriken" ),
   bsStatic( "ammo_kunai" ),
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

SCAFFOLD_MODULE( "item.c" );

#else

extern const struct tagbstring item_type_strings[ITEM_TYPE_MAX];

#endif /* ITEM_C */

#endif /* ITEM_H */
