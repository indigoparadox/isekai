
#include "audition.h"

#include "scaffold.h"

struct AUDI_CALL {
   bstring name;
   struct AUDI_PARM* parms;
};

union AUDI_VARIABLE_VAL {
   bstring str;
   int num;
   struct AUDI_CALL* call;
};

struct AUDI_VARIABLE {
   enum AUDI_VARIABLE_TYPE type;
   union AUDI_VARIABLE_VAL value;
};

struct AUDI_PARM {
   struct AUDI_VARIABLE value;
   struct AUDI_PARM* next;
};

struct AUDI_NODE {
   enum AUDI_OP op;
   struct AUDI_PARM* parms;
   struct AUDI_NODE* first;
   struct AUDI_NODE* prev;
   struct AUDI_NODE* next;
};

struct AUDI_CONTEXT {
   struct AUDI_NODE node;
   struct HASHMAP* functions;
   struct HASHMAP* variables;
   struct AUDI_NODE* ip; /* Instruction Pointer */
};

static void audition_variable_cleanup( struct AUDI_VARIABLE* var );

static void audition_node_init( struct AUDI_NODE* node, enum AUDI_OP op ) {
   node->op = op;
   node->first = NULL;
   node->prev = NULL;
   node->next = NULL;
}

struct AUDI_NODE* audition_node_new( enum AUDI_OP op ) {
   struct AUDI_NODE* out = NULL;

   out = mem_alloc( 1, struct AUDI_NODE );
   audition_node_init( out, op );

   return out;
}

struct AUDI_CONTEXT* audition_context_new() {
   struct AUDI_CONTEXT* out = NULL;

   out = mem_alloc( 1, struct AUDI_CONTEXT );
   audition_node_init( &(out->node), AUDI_OP_CONTEXT );
   out->variables = hashmap_new();
   out->functions = hashmap_new();

   return out;
}

static void audition_parm_free( struct AUDI_PARM* parm ) {
   struct AUDI_PARM* next = NULL;

   /* Recursive cleanup. */
   if( NULL != parm ) {
      next = parm->next;
      audition_variable_cleanup( &(parm->value) );
      mem_free( parm );
      audition_parm_free( next );
   }
}

static void audition_variable_cleanup( struct AUDI_VARIABLE* var ) {
   struct AUDI_PARM* parm_iter = NULL;

   lgc_null( var );

   switch( var->type ) {
   case AUDI_VARIABLE_STRING:
      bdestroy( var->value.str );
      var->value.str = NULL;
      break;

   case AUDI_VARIABLE_CALL:
      bdestroy( var->value.call->name );
      audition_parm_free( var->value.call->parms );
      break;

   }

cleanup:
   return;
}

static void audition_node_cleanup( struct AUDI_NODE* node ) {
   lgc_null( node );

cleanup:
   return;
}

void audition_node_free( struct AUDI_NODE** node ) {
   if( AUDI_OP_CONTEXT == (*node)->op ) {
      audition_context_free( (struct AUDI_CONTEXT**)node );
   } else {
      mem_free( *node );
      *node = NULL;
   }
}

static bool cb_audition_free_vars( bstring idx, void* iter, void* arg ) {
   struct AUDI_VARIABLE* var = (struct AUDI_VARIABLE*)iter;
   audition_variable_cleanup( var );
   return true;
}

static void audition_context_cleanup( struct AUDI_CONTEXT* node ) {
   audition_node_cleanup( &(node->node) );
   hashmap_remove_cb( node->variables, cb_audition_free_vars, NULL );
   hashmap_free( &(node->variables) );
   hashmap_free( &(node->functions) );
}

void audition_context_free( struct AUDI_CONTEXT** node ) {
   if( NULL == *node ) {
      return;
   }
   audition_context_cleanup( *node );
   mem_free( *node );
   *node = NULL;
}

void audition_node_add_function(
   struct AUDI_NODE* parent, bstring name, struct AUDI_NODE* func
) {
}

void audition_context_step( struct AUDI_CONTEXT* ctx ) {

}

static bool validate_node( struct AUDI_NODE* node ) {
   bool res = true;



   return res;
}

static void audition_insert_node( struct AUDI_NODE** current, enum AUDI_OP op ) {
   struct AUDI_NODE* new_node = NULL;
   if( NULL == *current ) {
      *current = audition_node_new( op );
   } else {
      new_node = audition_node_new( op );
      new_node->next = (*current);
      new_node->prev = (*current)->prev;
      if( NULL != (*current)->prev ) {
         (*current)->prev->next = new_node;
      }
      (*current)->prev = new_node;
      (*current) = new_node;
   }
}

static void audition_add_node( struct AUDI_NODE** current, enum AUDI_OP op ) {
   if( NULL == *current ) {
      *current = audition_node_new( op );
   } else {
      (*current)->next = audition_node_new( op );
      (*current)->next->prev = (*current);
      (*current) = (*current)->next;
   }
}

static void audition_parse_char(
   char chr, struct AUDI_NODE** current, enum AUDI_STATE* state, size_t* line
) {
   switch( chr ) {
   case '/':
      switch( *state ) {
      case AUDI_STATE_SLASH:
         /* A comment is starting. */
         *state = AUDI_STATE_COMMENT;
         break;

      case AUDI_STATE_NUMBER:
         /* Doing a math op. */
         assert( AUDI_OP_NONE == (*current)->op || AUDI_OP_ASSIGN == (*current)->op );

         break;

      case AUDI_STATE_LITERAL:
         /* Literal slash. */
         break;

      default:
         lg_error( __FILE__, "Invalid character '/' on line: %d\n", *line );
         *state = AUDI_STATE_INVALID;
         break;
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

   case '0':
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '8':
   case '9':

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
      audition_parse_char( bchar( code, i ), &iter, &state, &line );
   }

   return first;
}

void audition_context_add_var(
   struct AUDI_CONTEXT* node, const bstring key, const bstring val
) {
   bstring val_cpy = NULL;

   lgc_null( node );
   val_cpy = bstrcpy( val );

   hashmap_put( node->variables, key, val_cpy, false );
cleanup:
   return;
}
