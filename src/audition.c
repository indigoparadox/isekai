
#include "audition.h"

#include "scaffold.h"

union AUDI_PARM_VAL {
   bstring str;
   int num;
   struct AUDI_PARM* node;
};

struct AUDI_PARM {
   union AUDI_PARM_VAL value;
   struct AUDI_PARM* next;
};

struct AUDI_NODE {
   enum AUDI_OP op;
   struct HASHMAP* functions;
   bstring call;
   struct AUDI_PARM* params;
   struct AUDI_NODE* first;
   struct AUDI_NODE* prev;
   struct AUDI_NODE* next;
};

struct AUDI_NODE* audition_node_new( enum AUDI_OP op ) {
   struct AUDI_NODE* out = NULL;

   out = mem_alloc( 1, struct AUDI_NODE );
   out->op = op;

   return out;
}

void auditin_node_add_function(
   struct AUDI_NODE* parent, bstring name, struct AUDI_NODE* func
) {
}

static BOOL validate_node( struct AUDI_NODE* node ) {
   BOOL res = TRUE;



   return res;
}

static void audition_parse_char(
   char chr, struct AUDI_NODE* current, enum AUDI_STATE* state, size_t* line
) {

   switch( chr ) {
   case '/':
      if( AUDI_STATE_SLASH == *state ) {
         /* A comment is starting. */
      } else if( AUDI_STATE_NUMBER == *state ) {
         /* Doing a math op. */
      } else {
         lg_error( __FILE__, "Invalid character '/' on line: %d\n", *line );
         *state = AUDI_STATE_INVALID;
      }
      break;

   case '(':
      break;

   case '{':
      break;

   case '\r':
   case '\n':
      /* End comments. Otherwise, ignore. */
      (*line)++;
      break;

   case '\t':
   case ' ':
      /* Ignore whitespace. */
      break;

   default:
      break;
   }
}

struct AUDI_NODE* audition_parse( bstring code ) {
   struct AUDI_NODE* first = NULL,
      * iter = NULL;
   size_t i = 0,
      len = 0,
      line = 0;
   enum AUDI_STATE state = AUDI_STATE_NONE;

   len = blength( code );
   first = audition_node_new( AUDI_OP_NONE );

   for( i = 0 ; len > i ; i++ ) {
      audition_parse_char( bchar( code, i ), iter, &state, &line );
   }

   return first;
}
