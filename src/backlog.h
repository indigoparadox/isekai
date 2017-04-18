
#ifndef BACKLOG_H
#define BACKLOG_H

#include <time.h>

#include "scaffold.h"

struct BACKLOG_LINE {
   struct tm time;
   bstring nick;
   bstring line;
};

#ifdef BACKLOG_C
SCAFFOLD_MODULE( "backlog.c" );
#endif /* BACKLOG_C */

#endif /* BACKLOG_H */
