
#define ITEM_C
#include "item.h"

void item_init( struct ITEM* e ) {
   e->display_name = NULL;
   e->serial = 0;
   e->type = ITEM_TYPE_GENERIC;
   e->sprites_filename = NULL;
   e->sprites = NULL;
}

void item_set_contents( struct ITEM* e, union ITEM_CONTENT content ) {
   e->content = content;
}
