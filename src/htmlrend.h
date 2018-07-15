
#ifndef HTML_REND_H
#define HTML_REND_H

#include "graphics.h"
#include "htmtree.h"

#define HTML_TREE_ATTR_BOLD 0x02
#define HTML_TREE_ATTR_ITALIC 0x04
#define HTML_TREE_ATTR_UNDERLINE 0x08

char html_tag_next_char( struct html_tree_tag* tag, int idx, int flags );
char html_tree_next_char( struct html_tree* tree, int idx, int flags );
void htmlrend_draw( GRAPHICS* g, struct html_tree* tree );

#endif /* HTML_REND_H */
