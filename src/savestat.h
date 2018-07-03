
#ifndef SAVESTAT_H
#define SAVESTAT_H

#include "scaffold.h"

#ifndef DISABLE_SAVESTATE

struct CHANNEL;
struct CLIENT;
struct MOBILE;
struct TILEMAP;

BOOL savestat_save_client( struct CLIENT* o, bstring path );
BOOL savestat_load_client( struct CLIENT* o, bstring path );

#endif /* DISABLE_SAVESTATE */

#endif /* SAVESTAT_H */
