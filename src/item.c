
#include "item.h"

void item_init(
   struct ITEM* t, ITEM_TYPE type, SCAFFOLD_SIZE serial, const bstring display
) {
   t->display_name = bstrcpy( display );
   t->serial = serial;
   t->type = type;
}

void item_set_contents( struct ITEM* t, union ITEM_CONTENT content ) {
   t->content = content;
}
