
#ifndef MOBILE_H
#define MOBILE_H

#include "ref.h"
#include "client.h"

struct VM_CADDY;
struct TWINDOW;
struct MOBILE;
struct CLIENT;

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

struct MOBILE_UPDATE_PACKET {
   struct MOBILE* o;
   struct CHANNEL* l;
   MOBILE_UPDATE update;
   TILEMAP_COORD_TILE x;
   TILEMAP_COORD_TILE y;
   struct MOBILE* target;
};

#define MOBILE_RANDOM_SERIAL_LEN 64
#define MOBILE_STEPS_MAX 32
#define MOBILE_STEPS_HALF (MOBILE_STEPS_MAX / 2)
#define MOBILE_STEPS_INCREMENT 8
#define MOBILE_SPRITE_SIZE 32

void mobile_gen_serial( struct MOBILE* o, struct VECTOR* mobiles );
struct MOBILE* mobile_new( bstring mob_id, TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y );
void mobile_free( struct MOBILE* o );
void mobile_init(
   struct MOBILE* o, const bstring mob_id, TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y
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
void mobile_draw_ortho( struct MOBILE* o, struct CLIENT* local_client, struct TWINDOW* twindow );
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
#ifdef USE_ITEMS
void mobile_add_item( struct MOBILE* o, struct ITEM* e );
#endif /* USE_ITEMS */
struct CHANNEL* mobile_get_channel( const struct MOBILE* o );
void mobile_call_reset_animation( struct MOBILE* o );
void mobile_do_reset_2d_animation( struct MOBILE* o );
#ifdef USE_VM
void mobile_vm_start( struct MOBILE* o );
#endif /* USE_VM */
void mobile_update_coords(
   struct MOBILE* o, TILEMAP_COORD_TILE x, TILEMAP_COORD_TILE y
);
int mobile_set_display_name( struct MOBILE* o, const bstring name );
void mobile_set_owner( struct MOBILE* o, struct CLIENT* c );
void mobile_set_serial( struct MOBILE* o, SERIAL serial );
struct TILEMAP* mobile_get_tilemap( const struct MOBILE* o );
size_t mobile_set_sprite(
   struct MOBILE* o, size_t id, struct MOBILE_SPRITE_DEF* sprite
);
BOOL mobile_add_animation(
   struct MOBILE* o, bstring name_dir, struct MOBILE_ANI_DEF* animation
);
int mobile_set_id( struct MOBILE* o, bstring mob_id );
void mobile_set_sprite_width( struct MOBILE* o, GFX_COORD_PIXEL w );
void mobile_set_sprite_height( struct MOBILE* o, GFX_COORD_PIXEL h );
void mobile_set_sprites_filename( struct MOBILE* o, const bstring filename );
void mobile_set_facing( struct MOBILE* o, MOBILE_FACING facing );
void mobile_set_animation( struct MOBILE* o, bstring ani_key );
TILEMAP_COORD_TILE mobile_get_x( const struct MOBILE* o );
TILEMAP_COORD_TILE mobile_get_y( const struct MOBILE* o );
TILEMAP_COORD_TILE mobile_get_prev_x( const struct MOBILE* o );
TILEMAP_COORD_TILE mobile_get_prev_y( const struct MOBILE* o );
bstring mobile_get_sprites_filename( const struct MOBILE* o );
bstring mobile_get_id( const struct MOBILE* o );
struct GRAPHICS* mobile_get_sprites( const struct MOBILE* o );
bstring mobile_get_def_filename( const struct MOBILE* o );
SERIAL mobile_get_serial( const struct MOBILE* o );
struct MOBILE_SPRITE_DEF* mobile_get_sprite(
   const struct MOBILE* o, size_t id
);
GFX_COORD_PIXEL mobile_get_steps_remaining( const struct MOBILE* o );
struct CLIENT* mobile_get_owner( const struct MOBILE* o );
BOOL mobile_get_animation_reset( const struct MOBILE* o );
void mobile_set_initialized( struct MOBILE* o, BOOL init );
void mobile_set_type( struct MOBILE* o, MOBILE_TYPE type );
void mobile_set_sprites( struct MOBILE* o, struct GRAPHICS* sheet );

#ifdef MOBILE_C
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
