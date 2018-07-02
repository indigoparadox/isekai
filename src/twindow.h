
#ifndef TWINDOW_H
#define TWINDOW_H

#include "graphics.h"
#include "tilemap.h"

struct TWINDOW {
   GRAPHICS* g;            /*!< Graphics element to draw on. */
   TILEMAP_COORD_TILE x;        /*!< Window left in tiles. */
   TILEMAP_COORD_TILE y;        /*!< Window top in tiles. */
   TILEMAP_COORD_TILE width;    /*!< Window width in tiles. */
   TILEMAP_COORD_TILE height;   /*!< Window height in tiles. */
   TILEMAP_COORD_TILE max_x;    /*!< Right-most window tile. */
   TILEMAP_COORD_TILE max_y;    /*!< Bottom-most window tile. */
   TILEMAP_COORD_TILE min_x;    /*!< Left-most window tile. */
   TILEMAP_COORD_TILE min_y;    /*!< Top-most window tile. */
   GFX_COORD_PIXEL grid_w;
   GFX_COORD_PIXEL grid_h;
#ifdef DISABLE_MODE_POV
   BOOL dirty;
#endif /* !DISABLE_MODE_POV */
};


void twindow_update_details( struct TWINDOW* twindow );

#endif /* !TWINDOW_H */
