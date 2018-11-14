
#ifndef ACTION_H
#define ACTION_H

#include "tilemap.h"

enum ACTION_OP {
   ACTION_OP_NONE,
   ACTION_OP_MOVEUP,
   ACTION_OP_MOVEDOWN,
   ACTION_OP_MOVELEFT,
   ACTION_OP_MOVERIGHT,
   ACTION_OP_ATTACK,
   ACTION_OP_DIG
} ACTION_OP;

struct ACTION_PACKET;

struct ACTION_PACKET* action_packet_new(
   struct CHANNEL* l,
   struct MOBILE* o,
   enum ACTION_OP op,
   TILEMAP_COORD_TILE tile_x,
   TILEMAP_COORD_TILE tile_y,
   struct MOBILE* target
);
void action_enqueue( struct ACTION_PACKET* update );
size_t action_queue_proc( bstring mode_id );
struct CHANNEL* action_packet_get_channel( struct ACTION_PACKET* update );
struct MOBILE* action_packet_get_mobile( struct ACTION_PACKET* update );
struct MOBILE* action_packet_get_target( struct ACTION_PACKET* update );
TILEMAP_COORD_TILE action_packet_get_tile_x( struct ACTION_PACKET* update );
TILEMAP_COORD_TILE action_packet_get_tile_y( struct ACTION_PACKET* update );
enum ACTION_OP action_packet_get_op( struct ACTION_PACKET* update );
void action_packet_free( struct ACTION_PACKET** update );
void action_packet_set_op(
   struct ACTION_PACKET* update, enum ACTION_OP op );
void action_packet_set_tile_x(
   struct ACTION_PACKET* update, TILEMAP_COORD_TILE x );
void action_packet_set_tile_y(
   struct ACTION_PACKET* update, TILEMAP_COORD_TILE y );

#endif /* ACTION_H */
