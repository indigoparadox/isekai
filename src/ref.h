
#ifndef REF_H
#define REF_H

#ifndef USE_THREADS

typedef struct _REF {
    void (*free)( const struct _REF* );
    int count;
} REF;

static inline void ref_inc( const struct _REF *ref ) {
    ((struct _REF*)ref)->count++;
}

static inline void ref_dec(const struct _REF *ref ) {
    if( 0 == --((struct _REF*)ref)->count ) {
        ref->free( ref );
    }
}

#endif /* USE_THREADS */

#endif /* REF_H */
