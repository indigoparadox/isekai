
#ifndef INPUT_H
#define INPUT_H

typedef struct {
    void* event;
} INPUT;

void input_init( INPUT* p );
int input_get_char( INPUT* input );

#endif /* INPUT_H */
