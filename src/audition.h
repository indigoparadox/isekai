
#ifndef AUDITION_H
#define AUDITION_H

#include <bstrlib/bstrlib.h>

enum AUDI_OP {
   AUDI_OP_NONE = 0,
   AUDI_OP_IF,
   AUDI_OP_CALL,
   AUDI_OP_ASSIGN
};

enum AUDI_STATE {
   AUDI_STATE_NONE = 0,
   AUDI_STATE_FUNC_PARMS,
   AUDI_STATE_SLASH,
   AUDI_STATE_NUMBER,
   AUDI_STATE_INVALID
};

struct AUDI_NODE;
struct AUDI_PARM;

struct AUDI_NODE* audition_node_new( enum AUDI_OP op );
void auditin_node_add_function(
   struct AUDI_NODE* parent, bstring name, struct AUDI_NODE* func
);
struct AUDI_NODE* audition_parse( bstring code );

#endif /* AUDITION_H */
