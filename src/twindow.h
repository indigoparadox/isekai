
#ifndef TWINDOW_H
#define TWINDOW_H

#include "graphics.h"
#include "tilemap.h"
#include "ui.h"

struct TWINDOW;
struct UI;

struct TWINDOW* twindow_new();
void twindow_update_details( struct TWINDOW* twindow );
struct GRAPHICS* twindow_get_screen( const struct TWINDOW* w );
struct UI* twindow_get_ui( const struct TWINDOW* w );
GFX_COORD_PIXEL twindow_get_grid_w( const struct TWINDOW* w );
GFX_COORD_PIXEL twindow_get_grid_h( const struct TWINDOW* w );
TILEMAP_COORD_TILE twindow_get_x( const struct TWINDOW* w );
TILEMAP_COORD_TILE twindow_get_y( const struct TWINDOW* w );
TILEMAP_COORD_TILE twindow_get_width( const struct TWINDOW* w );
TILEMAP_COORD_TILE twindow_get_height( const struct TWINDOW* w );
TILEMAP_COORD_TILE twindow_get_max_x( const struct TWINDOW* w );
TILEMAP_COORD_TILE twindow_get_max_y( const struct TWINDOW* w );
TILEMAP_COORD_TILE twindow_get_min_x( const struct TWINDOW* w );
TILEMAP_COORD_TILE twindow_get_min_y( const struct TWINDOW* w );
void shrink_twindow_height( struct TWINDOW* w, TILEMAP_COORD_TILE h_shrink );
struct TILEMAP* twindow_get_tilemap_active( const struct TWINDOW* w );
struct CLIENT* twindow_get_local_client( const struct TWINDOW* w );
TILEMAP_COORD_TILE twindow_get_right( const struct TWINDOW* w );
TILEMAP_COORD_TILE twindow_get_bottom( const struct TWINDOW* w );
void twindow_set_x( struct TWINDOW* w, TILEMAP_COORD_TILE x );
void twindow_set_y( struct TWINDOW* w, TILEMAP_COORD_TILE y );
void twindow_set_max_x( struct TWINDOW* w, TILEMAP_COORD_TILE x );
void twindow_set_max_y( struct TWINDOW* w, TILEMAP_COORD_TILE y );
void twindow_set_min_x( struct TWINDOW* w, TILEMAP_COORD_TILE x );
void twindow_set_min_y( struct TWINDOW* w, TILEMAP_COORD_TILE y );
void twindow_shrink_height( struct TWINDOW* w, TILEMAP_COORD_TILE h_shrink );
void twindow_grow_height( struct TWINDOW* w, TILEMAP_COORD_TILE h_grow );
void twindow_shift_left_x( struct TWINDOW* w, TILEMAP_COORD_TILE x_left );
void twindow_shift_right_x( struct TWINDOW* w, TILEMAP_COORD_TILE x_right );
void twindow_shift_up_y( struct TWINDOW* w, TILEMAP_COORD_TILE y_up );
void twindow_shift_down_y( struct TWINDOW* w, TILEMAP_COORD_TILE y_down );
void twindow_set_ui( struct TWINDOW* w, struct UI* ui );
void twindow_set_screen( struct TWINDOW* w, struct GRAPHICS* g );
void twindow_set_local_client( struct TWINDOW* w, struct CLIENT* c );

#endif /* !TWINDOW_H */
