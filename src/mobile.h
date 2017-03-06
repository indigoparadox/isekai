
#ifndef MOBILE_H
#define MOBILE_H

#include "ref.h"
#include "client.h"

#define MOBILE_SPRITE_SIZE 32

typedef enum _MOBILE_FACING {
   MOBILE_FACING_DOWN = 0,
   MOBILE_FACING_UP = 1,
   MOBILE_FACING_RIGHT = 2,
   MOBILE_FACING_LEFT = 3,
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
   size_t serial;
   struct CLIENT* owner;
   size_t x;
   size_t y;
   size_t prev_x;
   size_t prev_y;
   size_t steps_remaining;
   GRAPHICS* sprites;
   MOBILE_FRAME_ALT frame_alt;
   MOBILE_FRAME frame;
   MOBILE_FACING facing;
};

#define MOBILE_RANDOM_SERIAL_LEN 64

#define mobile_new( o ) \
    o = (struct MOBILE*)calloc( 1, sizeof( struct MOBILE ) ); \
    scaffold_check_null( o ); \
    mobile_init( o );

void mobile_free( struct MOBILE* o );
void mobile_init( struct MOBILE* o );
void mobile_animate( struct MOBILE* o );
inline void mobile_get_spritesheet_pos_ortho(
   GRAPHICS* g_tileset, MOBILE_FACING facing, MOBILE_FRAME frame,
   MOBILE_FRAME_ALT frame_alt, size_t* x, size_t* y
);
void mobile_draw_ortho( struct MOBILE* o, struct GRAPHICS_TILE_WINDOW* twindow );

#endif /* MOBILE_H */
