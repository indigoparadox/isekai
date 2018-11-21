
#ifndef SAVESTAT_H
#define SAVESTAT_H

#include "scaffold.h"

#ifndef DISABLE_SAVESTATE

struct CHANNEL;
struct CLIENT;
struct MOBILE;
struct TILEMAP;

VBOOL savestat_save_client( struct CLIENT* o, bstring path );
VBOOL savestat_load_client( struct CLIENT* o, bstring path );

#endif /* DISABLE_SAVESTATE */

#endif /* SAVESTAT_H */
