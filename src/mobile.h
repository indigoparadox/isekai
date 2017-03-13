
#ifndef MOBILE_H
#define MOBILE_H

#include "ref.h"
#include "client.h"

typedef enum MOBILE_UPDATE {
   MOBILE_UPDATE_NONE,
   MOBILE_UPDATE_MOVEUP,
   MOBILE_UPDATE_MOVEDOWN,
   MOBILE_UPDATE_MOVELEFT,
   MOBILE_UPDATE_MOVERIGHT
} MOBILE_UPDATE;

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

struct MOBILE {
   struct REF refcount;
   uint8_t serial;
   struct CLIENT* owner;
   SCAFFOLD_SIZE x;
   SCAFFOLD_SIZE y;
   SCAFFOLD_SIZE prev_x;
   SCAFFOLD_SIZE prev_y;
   int16_t steps_inc;
   int16_t steps_remaining;
   bstring sprites_filename;
   GRAPHICS* sprites;
   MOBILE_FRAME_ALT frame_alt;
   MOBILE_FRAME frame;
   MOBILE_FACING facing;
   bstring display_name;
   struct CHANNEL* channel;
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
#define MOBILE_STEPS_INCREMENT 2
#define MOBILE_SPRITE_SIZE 32
#define MOBILE_FRAME_DIVISOR 90
#define MOBILE_MOVE_DIVISOR 30

#define mobile_new( o ) \
    o = (struct MOBILE*)calloc( 1, sizeof( struct MOBILE ) ); \
    scaffold_check_null( o ); \
    mobile_init( o );

void mobile_free( struct MOBILE* o );
void mobile_init( struct MOBILE* o );
void mobile_animate( struct MOBILE* o );
SCAFFOLD_INLINE void mobile_get_spritesheet_pos_ortho(
   GRAPHICS* g_tileset, MOBILE_FACING facing, MOBILE_FRAME frame,
   MOBILE_FRAME_ALT frame_alt, SCAFFOLD_SIZE* x, SCAFFOLD_SIZE* y
);
void mobile_apply_steps_remaining(
   struct MOBILE* o, SCAFFOLD_SIZE* x, SCAFFOLD_SIZE* y, BOOL reverse
);
void mobile_draw_ortho( struct MOBILE* o, struct GRAPHICS_TILE_WINDOW* twindow );
void mobile_set_channel( struct MOBILE* o, struct CHANNEL* l );
MOBILE_UPDATE mobile_apply_update( struct MOBILE_UPDATE_PACKET* update, BOOL instant );
SCAFFOLD_INLINE
SCAFFOLD_SIZE mobile_get_steps_remaining_x( const struct MOBILE* o, BOOL reverse );
SCAFFOLD_INLINE
SCAFFOLD_SIZE mobile_get_steps_remaining_y( const struct MOBILE* o, BOOL reverse );
SCAFFOLD_INLINE BOOL mobile_inside_inner_map_x(
   struct MOBILE* o, struct GRAPHICS_TILE_WINDOW* twindow
);
SCAFFOLD_INLINE BOOL mobile_inside_inner_map_y(
   struct MOBILE* o, struct GRAPHICS_TILE_WINDOW* twindow
);

#endif /* MOBILE_H */
