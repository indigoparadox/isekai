
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

/*
typedef enum _MOBILE_FACING {
   MOBILE_FACING_DOWN = 0,
   MOBILE_FACING_UP = 1,
   MOBILE_FACING_RIGHT = 2,
   MOBILE_FACING_LEFT = 3
} MOBILE_FACING;

typedef enum _MOBILE_FRAME {
   MOBILE_FRAME_NONE = -1,
   MOBILE_FRAME_DEFAULT = 0,
   MOBILE_FRAME_LEFT_FORWARD = 1,
   MOBILE_FRAME_RIGHT_FORWARD = 2
} MOBILE_FRAME;

typedef enum _MOBILE_FRAME_ALT {
   MOBILE_FRAME_ALT_NONE = -1,
   MOBILE_FRAME_ALT_ATTACK1 = 0,
   MOBILE_FRAME_ALT_ATTACK2 = 1,
   MOBILE_FRAME_ALT_KNOCKBACK = 2
} MOBILE_FRAME_ALT;
*/

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
   SCAFFOLD_SIZE x;
   SCAFFOLD_SIZE y;
   SCAFFOLD_SIZE prev_x;
   SCAFFOLD_SIZE prev_y;
   SCAFFOLD_SIZE sprite_width;
   SCAFFOLD_SIZE sprite_height;
   SCAFFOLD_SIZE sprite_display_height;
   SCAFFOLD_SIZE_SIGNED steps_inc;
   SCAFFOLD_SIZE_SIGNED steps_inc_default;
   SCAFFOLD_SIZE_SIGNED steps_remaining;
   bstring sprites_filename;
   GRAPHICS* sprites;
   /* MOBILE_FRAME_ALT frame_alt;
   MOBILE_FRAME frame; */
   uint8_t current_frame;
   MOBILE_FACING facing;
   bstring display_name;
   bstring def_filename;
   bstring mob_id;
   struct CHANNEL* channel;
   struct VM* vm;
   struct VM_CADDY* vm_caddy;
   BOOL vm_started;
   struct HASHMAP vm_scripts;
   struct HASHMAP vm_globals;
   struct VECTOR sprite_defs;
   struct HASHMAP ani_defs;
   struct HASHMAP script_defs;
   struct MOBILE_ANI_DEF* current_animation;
   BOOL initialized;
#ifdef USE_TURNS
   SCAFFOLD_SIZE vm_tick_prev;
#endif /* USE_TURNS */
};

struct MOBILE_UPDATE_PACKET {
   struct MOBILE* o;
   struct CHANNEL* l;
   MOBILE_UPDATE update;
   SCAFFOLD_SIZE x;
   SCAFFOLD_SIZE y;
};

#define MOBILE_RANDOM_SERIAL_LEN 64
#define MOBILE_STEPS_MAX 32
#define MOBILE_STEPS_INCREMENT 8
#define MOBILE_SPRITE_SIZE 32
#define MOBILE_FRAME_DIVISOR 90
#define MOBILE_MOVE_DIVISOR 30

#define mobile_new( o, mob_id, x, y ) \
    o = (struct MOBILE*)calloc( 1, sizeof( struct MOBILE ) ); \
    scaffold_check_null( o ); \
    mobile_init( o, mob_id, x, y );

#define mobile_set_animation_facing( o, buffer, facing ) \
   if( NULL != o->current_animation ) { \
      buffer = bformat( \
         "%s-%s", \
         bdata( o->current_animation->name ), \
         str_mobile_facing[facing].data \
      ); \
      if( NULL != hashmap_get( &(o->ani_defs), buffer ) ) { \
         o->current_animation = \
            hashmap_get( &(o->ani_defs), buffer ); \
      } \
   }

void mobile_free( struct MOBILE* o );
void mobile_init(
   struct MOBILE* o, const bstring mob_id, SCAFFOLD_SIZE x, SCAFFOLD_SIZE y
);
void mobile_load_local( struct MOBILE* o );
void mobile_animate( struct MOBILE* o );
SCAFFOLD_INLINE void mobile_get_spritesheet_pos_ortho(
   struct MOBILE* o, SCAFFOLD_SIZE gid,
   SCAFFOLD_SIZE* x, SCAFFOLD_SIZE* y
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
SCAFFOLD_SIZE_SIGNED
mobile_get_steps_remaining_x( const struct MOBILE* o, BOOL reverse );
SCAFFOLD_INLINE
SCAFFOLD_SIZE_SIGNED
mobile_get_steps_remaining_y( const struct MOBILE* o, BOOL reverse );
void mobile_speak( struct MOBILE* o, bstring speech );
BOOL mobile_is_local_player( struct MOBILE* o );
BOOL mobile_is_occupied( struct MOBILE* o );

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
