
#ifndef BSTRGLUE_H
#define BSTRGLUE_H

#include "bstrlib/bstrlib.h"

struct VECTOR;

struct VECTOR* bgsplit( const_bstring str, char split );
int bgtoi( bstring i_b );

#endif /* BSTRGLUE_H */
