
#ifndef ITEM_H
#define ITEM_H

#include "scaffold.h"

typedef enum ITEM_TYPE {
   ITEM_TYPE_GENERIC,
   ITEM_TYPE_ARMOR_HEAD,
   ITEM_TYPE_ARMOR_BODY,
   ITEM_TYPE_ARMOR_CLOAK,
   ITEM_TYPE_ARMOR_ARMS,
   ITEM_TYPE_ARMOR_FEET,
   ITEM_TYPE_ARMOR_LEGS,
   ITEM_TYPE_CONTAINER,
   ITEM_TYPE_POTION,
   ITEM_TYPE_SCROLL,
   ITEM_TYPE_BOOK
} ITEM_TYPE;


union ITEM_CONTENT {
   bstring book_text;
   struct VECTOR* container;
};

struct ITEM {
   SCAFFOLD_SIZE serial;
   bstring display_name;
   ITEM_TYPE type;
   union ITEM_CONTENT content;
};

void item_init(
   struct ITEM* t, ITEM_TYPE type, SCAFFOLD_SIZE serial, const bstring display
);
void item_set_contents( struct ITEM* t, union ITEM_CONTENT content );

#endif /* ITEM_H */
