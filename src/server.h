
#ifndef SERVER_H
#define SERVER_H

#include "vector.h"
#include "connection.h"

typedef struct {
    VECTOR* connections;
} SERVER;

#endif /* SERVER_H */
