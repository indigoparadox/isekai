
#ifndef AUDITION_H
#define AUDITION_H

#include <bstrlib/bstrlib.h>

enum AUDI_OP {
   AUDI_OP_NONE = 0,
   AUDI_OP_IF,
   AUDI_OP_CALL,
   AUDI_OP_ASSIGN,
   AUDI_OP_CONTEXT,
   AUDI_OP_ADD,
   AUDI_OP_DIVIDE,
   AUDI_OP_SUBTRACT
};

enum AUDI_STATE {
   AUDI_STATE_NONE = 0,
   AUDI_STATE_FUNC_PARMS,
   AUDI_STATE_SLASH,
   AUDI_STATE_NUMBER,
   AUDI_STATE_LITERAL,
   AUDI_STATE_INVALID,
   AUDI_STATE_COMMENT
};

enum AUDI_VARIABLE_TYPE {
   AUDI_VARIABLE_NONE,
   AUDI_VARIABLE_STRING,
   AUDI_VARIABLE_CALL,
   AUDI_VARIABLE_NUM
};


struct AUDI_CALL;
struct AUDI_VARIABLE;
struct AUDI_PARM;
struct AUDI_NODE;
struct AUDI_CONTEXT;

struct AUDI_NODE* audition_node_new( enum AUDI_OP op );
struct AUDI_CONTEXT* audition_context_new();
void audition_node_free( struct AUDI_NODE** node );
void audition_context_free( struct AUDI_CONTEXT** node );
void audition_context_step( struct AUDI_CONTEXT* ctx );
void audition_node_add_function(
   struct AUDI_NODE* parent, bstring name, struct AUDI_NODE* func
);
struct AUDI_NODE* audition_parse( bstring code );
void audition_context_add_var(
   struct AUDI_CONTEXT* node, const bstring key, const bstring val );

#endif /* AUDITION_H */
