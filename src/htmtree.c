
#include "htmtree.h"

#include <stdint.h>
#include <stdlib.h>

struct html_tree_entity_def {
   struct tagbstring name;
   struct tagbstring character;
};

static struct tagbstring tag_name = bsStatic( "name" );

#define HTML_TREE_SINGLETON_COUNT 18

static struct tagbstring html_tree_singleton_tags[HTML_TREE_SINGLETON_COUNT] = {
   bsStatic( "!--" ),
   bsStatic( "!DOCTYPE" ),
   bsStatic( "area" ),
   bsStatic( "base" ),
   bsStatic( "br" ),
   bsStatic( "col" ),
   bsStatic( "command" ),
   bsStatic( "embed" ),
   bsStatic( "hr" ),
   bsStatic( "img" ),
   bsStatic( "input" ),
   bsStatic( "keygen" ),
   bsStatic( "link" ),
   bsStatic( "meta" ),
   bsStatic( "param" ),
   bsStatic( "source" ),
   bsStatic( "track" ),
   bsStatic( "wbr" )
};

static bstring tag_comment = &(html_tree_singleton_tags[0]);

#define HTML_TREE_ENTITY_COUNT 12

static struct html_tree_entity_def entities[HTML_TREE_ENTITY_COUNT] = {
   { bsStatic( "dash" ), bsStatic( "-" ) },
   { bsStatic( "copy" ), bsStatic( "©" ) },
   { bsStatic( "amp" ), bsStatic( "&" ) },
   { bsStatic( "nbsp" ), bsStatic( " " ) },
   { bsStatic( "lt" ), bsStatic( "<" ) },
   { bsStatic( "gt" ), bsStatic( ">" ) },
   { bsStatic( "quot" ), bsStatic( "\"" ) },
   { bsStatic( "apos" ), bsStatic( "'" ) },
   { bsStatic( "cent" ), bsStatic( "¢" ) },
   { bsStatic( "pound" ), bsStatic( "£" ) },
   { bsStatic( "yen" ), bsStatic( "¥" ) },
   { bsStatic( "reg" ), bsStatic( "®" ) }
};

void html_tree_new_tag( struct html_tree* tree ) {
   struct html_tree_tag* new_tag = NULL;
   struct html_tree_tag* new_root = NULL;

   /* Create the new tag. */
   new_tag = calloc( 1, sizeof( struct html_tree_tag ) );
   new_tag->tag = bfromcstr( "" );
   new_tag->data = bfromcstr( "" );

   if( NULL == tree->root ) {
      new_root = calloc( 1, sizeof( struct html_tree_tag ) );
      new_root->tag = bfromcstr( "root" );
      new_root->data = bfromcstr( "" );

      tree->root = new_root;
      tree->root->first_child = new_tag;
      new_tag->parent = new_root;
      tree->current = tree->root->first_child;
   }

   if( NULL == tree->current ) {
      tree->current = tree->root;
   }

   if(
      new_tag != tree->current &&
      NULL == tree->current->first_child
   ) {
      /* If the new tag is not the root, it must be a child... */
      tree->current->first_child = new_tag;
      new_tag->parent = tree->current;
      tree->current = tree->current->first_child;
   } else if(
      new_tag != tree->current &&
      NULL != tree->current->first_child
   ) {
      /* If it's not the first child, it must be a sibling of that child. */
      new_tag->parent = tree->current;
      tree->current = tree->current->first_child;
      while( NULL != tree->current->next_sibling ) {
         tree->current = tree->current->next_sibling;
      }
      tree->current->next_sibling = new_tag;
      tree->current = tree->current->next_sibling;
   }
}

static struct html_tree_attr* html_tree_new_attr( struct html_tree_tag* tag ) {
   struct html_tree_attr* attr_iter = NULL;
   struct html_tree_attr* new_attr = NULL;

   new_attr = calloc( 1, sizeof( struct html_tree_attr ) );
   new_attr->label = bfromcstr( "" );
   new_attr->value = bfromcstr( "" );

   if( NULL == tag ) {
      /* TODO: Error. */
   }

   if( NULL == tag->attrs ) {
      tag->attrs = new_attr;
   } else {
      attr_iter = tag->attrs;
      while( NULL != attr_iter->next ) {
         attr_iter = attr_iter->next;
      }
      attr_iter->next = new_attr;
   }

   return new_attr;
}

static void html_tree_append_char( unsigned char c, struct html_tree* tree ) {
   struct html_tree_attr* attr_current = NULL;

   switch( tree->state ) {
      case HTML_TREE_IN_START_TAG:
         bconchar( tree->current->tag, c );
         break;

      case HTML_TREE_IN_DATA:
         bconchar( tree->current->data, c );
         break;

      case HTML_TREE_IN_ATTR_LABEL:
         attr_current = tree->current->attrs;
         /* The current attr is always last. */
         while( NULL != attr_current->next ) {
            attr_current = attr_current->next;
         }
         bconchar( attr_current->label, c );
         break;

      case HTML_TREE_IN_ATTR_VALUE:
         attr_current = tree->current->attrs;
         /* The current attr is always last. */
         while( NULL != attr_current->next ) {
            attr_current = attr_current->next;
         }
         bconchar( attr_current->value, c );
         break;

      case HTML_TREE_IN_ENTITY:
         attr_current = tree->current->attrs;
         /* The current attr is always last. */
         while(
            NULL != attr_current &&
            0 != bstricmp( &tag_name, attr_current->label )
         ) {
            attr_current = attr_current->next;
         }
         if( NULL == attr_current ) {
            attr_current = html_tree_new_attr( tree->current );
            bassign( attr_current->label, &tag_name );
         }
         bconchar( attr_current->value, c );
         break;
   }
}

static void html_tree_parse_char( unsigned char c, struct html_tree* tree ) {
   int i;
   short tag_is_singleton_or_entity = 0;

   switch( c ) {
      case '<':
         if(
            HTML_TREE_OPENING_TAG == tree->state ||
            HTML_TREE_IN_START_TAG == tree->state ||
            HTML_TREE_IN_END_TAG == tree->state ||
            HTML_TREE_IN_ATTR_LABEL == tree->state ||
            /* TODO: Should we allow these in attr values? */
            HTML_TREE_IN_ATTR_VALUE == tree->state
         ) {
            /* TODO: Error: Double-open tag. */
         } else {
            if(
               HTML_TREE_IN_DATA == tree->state &&
               NULL != tree->current &&
               0 == blength( tree->current->tag )
            ) {
               tree->current = tree->current->parent;
            }

            tree->state = HTML_TREE_OPENING_TAG;
         }
         break;

      case '>':
         for( i = 0 ; HTML_TREE_SINGLETON_COUNT > i ; i++ ) {
            if( 0 == bstricmp(
               tree->current->tag,
               &(html_tree_singleton_tags[i])
            ) ) {
               tag_is_singleton_or_entity = 1;
            }
         }

         if(
            HTML_TREE_IN_END_TAG == tree->state ||
            tag_is_singleton_or_entity
         ) {
            /* Strip off the end -- in comments. */
            if( 0 == bstricmp( tree->current->tag, tag_comment ) ) {
               btrunc(
                  tree->current->attrs->label,
                  blength( tree->current->attrs->label ) - 2
               );
            }

            if( 0 == blength( tree->current->tag ) ) {
               tree->current = tree->current->parent;
            }
            if( NULL != tree->current->parent ) {
               tree->current = tree->current->parent;
            }

            tree->state = HTML_TREE_IN_DATA;
         } else if( HTML_TREE_IN_START_TAG == tree->state ) {
            tree->state = HTML_TREE_IN_DATA;
         } else if( HTML_TREE_OPENING_TAG == tree->state ) {
            /* Weird empty tag. */
            tree->state = HTML_TREE_IN_DATA;
         }
         break;

      case '\0':
         break;

      case '&':
         if( NULL != tree->current && NULL != tree->current->parent ) {
            tree->current = tree->current->parent;
         }
         html_tree_new_tag( tree );
         tree->last_state = tree->state;
         tree->state = HTML_TREE_IN_ENTITY;
         break;

      case ';':
         if( HTML_TREE_IN_ENTITY == tree->state ) {
            for( i = 0 ; HTML_TREE_ENTITY_COUNT > i ; i++ ) {
               if( 0 == bstricmp(
                  &(entities[i].name),
                  tree->current->attrs->value
               ) ) {
                  bassign( tree->current->data, &(entities[i].character) );
               }
            }
            if( NULL != tree->current && NULL != tree->current->parent ) {
               tree->current = tree->current->parent;
            }
            html_tree_new_tag( tree );
            tree->state = tree->last_state;
         }
         /* TODO: Handle other states? */
         break;

      case '"':
         if( HTML_TREE_IN_ATTR_LABEL == tree->state ) {
            tree->state = HTML_TREE_IN_ATTR_VALUE;
         } else if( HTML_TREE_IN_ATTR_VALUE == tree->state ) {
            /* Only the start tag has attrs, anyway. */
            tree->state = HTML_TREE_IN_START_TAG;
         }
         break;

      case '=':
         /* Don't append the '=' to the attr label. */
         if( HTML_TREE_IN_ATTR_LABEL != tree->state ) {
            html_tree_append_char( c, tree );
         }
         break;

      case '/':
         if( HTML_TREE_OPENING_TAG == tree->state ) {
            tree->state = HTML_TREE_IN_END_TAG;
         } else if( HTML_TREE_IN_START_TAG == tree->state ) {
            tree->state = HTML_TREE_IN_END_TAG;
         }
         break;

      case '\n':
      case '\r':
         /* Always ignore newlines. */
         break;

      case '\t':
      case ' ':
         /* Limit spaces to one. */
         if(
            ' ' == tree->last_char ||
            '\t' == tree->last_char
         ) {
            break;
         }

         if( HTML_TREE_IN_START_TAG == tree->state ) {
            /* Don't add spaces to tag tags. */
            break;
         }

         /* Fall through to default. */

      case '?':
         if(
            '?' == c &&
            HTML_TREE_OPENING_TAG == tree->state
         ) {
            html_tree_new_tag( tree );
            tree->state = HTML_TREE_IN_START_TAG;
            break;
         } else if(
            '?' == c &&
            HTML_TREE_IN_START_TAG == tree->state
         ) {
            tree->state = HTML_TREE_IN_END_TAG;
            break;
         }

         /* Fall through to default. */

      default:
         if( HTML_TREE_OPENING_TAG == tree->state ) {
            /* The character isn't '/' or '>', so we're in a tag! */
            html_tree_new_tag( tree );
            tree->state = HTML_TREE_IN_START_TAG;
         } else if(
            ' ' == tree->last_char &&
            HTML_TREE_IN_START_TAG == tree->state
         ) {
            html_tree_new_attr( tree->current );
            tree->state = HTML_TREE_IN_ATTR_LABEL;
         } else if(
            HTML_TREE_IN_DATA == tree->state &&
            /* TODO: Crash if text before <html>? */
            ((NULL != tree->current &&
            0 < blength( tree->current->tag )) ||
            NULL == tree->current)
         ) {
            html_tree_new_tag( tree );
         }
         html_tree_append_char( c, tree );
         break;
   }

   tree->last_char = c;
}

static void html_tree_cleanup( struct html_tree_tag* tag ) {
   struct html_tree_tag* tag_iter = NULL;
   struct html_tree_attr* attr_iter = NULL;
   struct html_tree_tag* tag_last = NULL;
   struct html_tree_tag* tag_parent = NULL;
   bstring str_test = NULL;
   int attr_count;

   tag_iter = tag;
   while( NULL != tag_iter ) {
      attr_iter = tag_iter->attrs;
      attr_count = 0;
      while( NULL != attr_iter ) {
         attr_count++;
         attr_iter = attr_iter->next;
      }

      if( NULL != tag_iter->first_child ) {
         html_tree_cleanup( tag_iter->first_child );
      } else {
         /* This tag has no children. */
         str_test = bstrcpy( tag_iter->data );
         btrimws( str_test );
         if( 0 == blength( str_test ) && 0 == blength( tag_iter->tag ) && 0 == attr_count ) {
            /* This tag has no data or attrs. */
            if( NULL != tag_last ) {
               tag_last->next_sibling = tag_iter->next_sibling;
               html_tree_free_tag( tag_iter );
               tag_iter = tag_last->next_sibling;
               continue;
            } else if( NULL != tag_iter->parent ) {
               /* No prior sibling. */
               tag_parent = tag_iter->parent;
               tag_parent->first_child = tag_iter->next_sibling;
               html_tree_free_tag( tag_iter );
               tag_iter = tag_parent->first_child;
               continue;
            }
         }
      }

      tag_last = tag_iter;
      tag_iter = tag_iter->next_sibling;
   }
}

int html_tree_parse_string( bstring html_string, struct html_tree* out ) {
   int i;

   if( NULL == out ) {
      /* TODO: Error. */
   }

   if( NULL != out->root ) {
      /* TODO: Error. */
   }

   for( i = 0 ; blength( html_string ) > i ; i++ ) {
      html_tree_parse_char( bchar( html_string, i ), out );
   }

   html_tree_cleanup( out->root );
}

void html_tree_free_attr( struct html_tree_attr* attr ) {
   bdestroy( attr->label );
   bdestroy( attr->value );
   free( attr );
}

void html_tree_free_tag( struct html_tree_tag* tag ) {
   struct html_tree_attr* attr_iter = NULL;
   struct html_tree_attr* attr_next = NULL;

   attr_iter = tag->attrs;
   while( NULL != attr_iter ) {
      attr_next = attr_iter->next;
      html_tree_free_attr( attr_iter );
      attr_iter = attr_next;
   }

   bdestroy( tag->data );
   bdestroy( tag->tag );

   free( tag );
}
