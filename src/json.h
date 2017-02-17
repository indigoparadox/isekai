
#ifndef JSON_H
#define JSON_H

#include "bstrlib/bstrlib.h"
#include "vector.h"

typedef enum {
    JSON_OBJECT_TYPE_OBJECT,
    JSON_OBJECT_TYPE_LIST,
} JSON_OBJECT_TYPE;

typedef struct {
    JSON_OBJECT_TYPE type;
    bstring name;
    bstring value;
    VECTOR children;
    uint8_t depth;
} JSON;

#define JSON_BUFFER_ALLOC 30

#endif /* JSON_H */
