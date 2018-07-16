
#ifndef HTMLTREE_H
#define HTMLTREE_H

#include "libvcol.h"

enum html_tree_state {
   HTML_TREE_IN_DATA,
   HTML_TREE_OPENING_TAG,
   HTML_TREE_IN_START_TAG,
   HTML_TREE_IN_END_TAG,
   HTML_TREE_IN_ATTR_LABEL,
   HTML_TREE_IN_ATTR_VALUE,
   HTML_TREE_IN_ENTITY
};

struct html_tree_attr {
   bstring label;
   bstring value;
   struct html_tree_attr* next;
};

struct html_tree_tag {
   bstring tag;
   bstring data;
   struct html_tree_attr* attrs;
   struct html_tree_tag* parent;
   struct html_tree_tag* first_child;
   struct html_tree_tag* next_sibling;
};

struct html_tree {
   enum html_tree_state state;
   enum html_tree_state last_state;
   unsigned char last_char;
   struct html_tree_tag* root;
   struct html_tree_tag* current;
};

void html_tree_new_tag( struct html_tree* tree );
int html_tree_parse_string( bstring html_string, struct html_tree* out );
void html_tree_free_attr( struct html_tree_attr* attr );
void html_tree_free_tag( struct html_tree_tag* tag );

#endif /* HTMLTREE_H */

