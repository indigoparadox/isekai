
#ifndef INPUT_H
#define INPUT_H

#include "client.h"

#include <stdint.h>

typedef struct {
    void* event;
    CLIENT* client;
} INPUT;

void input_init( INPUT* p );
int16_t input_get_char( INPUT* input );

#endif /* INPUT_H */
