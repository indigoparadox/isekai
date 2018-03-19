
#ifndef MOBILE_H
#define MOBILE_H

#include "ref.h"
#include "client.h"

struct VM_CADDY;
struct VM;

typedef enum MOBILE_UPDATE {
   MOBILE_UPDATE_NONE,
   MOBILE_UPDATE_MOVEUP,
   MOBILE_UPDATE_MOVEDOWN,
   MOBILE_UPDATE_MOVELEFT,
   MOBILE_UPDATE_MOVERIGHT,
   MOBILE_UPDATE_ATTACK,
   MOBILE_UPDATE_DIG
} MOBILE_UPDATE;

typedef enum MOBILE_ANI_TYPE {
   MOBILE_ANI_TYPE_WALK,
   MOBILE_ANI_TYPE_ATTACK,
   MOBILE_ANI_TYPE_KNOCKBACK
} MOBILE_ANI_TYPE;

typedef enum MOBILE_FACING {
   MOBILE_FACING_DOWN = 0,
   MOBILE_FACING_UP = 1,
   MOBILE_FACING_RIGHT = 2,
   MOBILE_FACING_LEFT = 3
} MOBILE_FACING;

typedef enum MOBILE_TYPE {
   MOBILE_TYPE_GENERIC,
   MOBILE_TYPE_PLAYER
} MOBILE_TYPE;

struct MOBILE_ANI_DEF {
   MOBILE_FACING facing;
   bstring name;
   short speed;
   struct VECTOR frames; /*!< Pointers to the sprite defs in the sprite_defs vector. */
};

struct MOBILE_SPRITE_DEF {
   SCAFFOLD_SIZE id;
};

struct MOBILE {
   struct REF refcount;
   SERIAL serial;
   struct CLIENT* owner;
   GFX_COORD_TILE x;
   GFX_COORD_TILE y;
   GFX_COORD_TILE prev_x;
   GFX_COORD_TILE prev_y;
   GFX_COORD_PIXEL sprite_width;
   GFX_COORD_PIXEL sprite_height;
   GFX_COORD_PIXEL sprite_display_height;
   GFX_COORD_PIXEL steps_inc;
   GFX_COORD_PIXEL steps_inc_default;
   GFX_COORD_PIXEL steps_remaining;
   bstring sprites_filename;
   GRAPHICS* sprites;
   /* MOBILE_FRAME_ALT frame_alt;
   MOBILE_FRAME frame; */
   uint8_t current_frame;
   MOBILE_FACING facing;
   BOOL animation_reset;
   bstring display_name;
   bstring def_filename;
   bstring mob_id;
   struct CHANNEL* channel;
   MOBILE_TYPE type;
   struct VM* vm;
   struct VM_CADDY* vm_caddy;
   BOOL vm_started;
   struct HASHMAP vm_scripts;
   struct HASHMAP vm_globals;
   struct VECTOR sprite_defs;
   struct HASHMAP ani_defs;
   struct HASHMAP script_defs;
   struct MOBILE_ANI_DEF* current_animation;
   struct VECTOR items;
   BOOL initialized;
#ifdef USE_TURNS
   SCAFFOLD_SIZE vm_tick_prev;
#endif /* USE_TURNS */
#ifdef USE_RAYCASTING
   double ray_distance;
#endif /* USE_RAYCASTING */
};

struct MOBILE_UPDATE_PACKET {
   struct MOBILE* o;
   struct CHANNEL* l;
   MOBILE_UPDATE update;
   GFX_COORD_TILE x;
   GFX_COORD_TILE y;
   struct MOBILE* target;
};

#define MOBILE_RANDOM_SERIAL_LEN 64
#define MOBILE_STEPS_MAX 32
#define MOBILE_STEPS_HALF (MOBILE_STEPS_MAX / 2)
#define MOBILE_STEPS_INCREMENT 8
#define MOBILE_SPRITE_SIZE 32

#define mobile_new( o, mob_id, x, y ) \
    o = mem_alloc( 1, struct MOBILE ); \
    scaffold_check_null( o ); \
    mobile_init( o, mob_id, x, y );

void mobile_free( struct MOBILE* o );
void mobile_init(
   struct MOBILE* o, const bstring mob_id, GFX_COORD_TILE x, GFX_COORD_TILE y
);
void mobile_animation_free( struct MOBILE_ANI_DEF* animation );
void mobile_load_local( struct MOBILE* o );
void mobile_animate( struct MOBILE* o );
SCAFFOLD_INLINE void mobile_get_spritesheet_pos_ortho(
   struct MOBILE* o, SCAFFOLD_SIZE gid,
   GFX_COORD_PIXEL* x, GFX_COORD_PIXEL* y
);
void mobile_apply_steps_remaining(
   struct MOBILE* o, SCAFFOLD_SIZE* x, SCAFFOLD_SIZE* y, BOOL reverse
);
void mobile_draw_ortho(
   struct MOBILE* o, struct GRAPHICS_TILE_WINDOW* twindow
);
void mobile_set_channel( struct MOBILE* o, struct CHANNEL* l );
MOBILE_UPDATE
mobile_apply_update( struct MOBILE_UPDATE_PACKET* update, BOOL instant );
SCAFFOLD_INLINE
GFX_COORD_PIXEL
mobile_get_steps_remaining_x( const struct MOBILE* o, BOOL reverse );
SCAFFOLD_INLINE
GFX_COORD_PIXEL
mobile_get_steps_remaining_y( const struct MOBILE* o, BOOL reverse );
void mobile_speak( struct MOBILE* o, bstring speech );
SCAFFOLD_INLINE
BOOL mobile_is_local_player( struct MOBILE* o, struct CLIENT* c );
BOOL mobile_is_occupied( struct MOBILE* o );
void mobile_add_item( struct MOBILE* o, struct ITEM* e );
struct CHANNEL* mobile_get_channel( struct MOBILE* o );

#ifdef MOBILE_C
SCAFFOLD_MODULE( "mobile.c" );
const struct tagbstring str_mobile_default_ani = bsStatic( "normal" );
const struct tagbstring str_mobile_facing[4] = {
   bsStatic( "down" ),
   bsStatic( "up" ),
   bsStatic( "right" ),
   bsStatic( "left" )
};
const struct tagbstring str_mobile_def_id_default =
   bsStatic( "maidblac" );
#else
extern const struct tagbstring str_mobile_default_ani;
extern const struct tagbstring str_mobile_facing[4];
extern const struct tagbstring str_mobile_def_id_default;
#endif /* MOBILE_C */

#endif /* MOBILE_H */
