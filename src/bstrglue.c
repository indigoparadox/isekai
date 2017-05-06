
#define BSTRGLUE_C
#include "bstrglue.h"

#include "scaffold.h"

SCAFFOLD_MODULE( "bstrglue.c" );

#include "vector.h"

struct VECTOR* bgsplit( const_bstring str, char split ) {
   int i = 0;
   unsigned char* chr;
   struct VECTOR* v = NULL;
   bstring current_str = NULL;

   vector_new( v );

   current_str = bfromcstr( "" );
   scaffold_check_null( current_str );
   vector_add( v, current_str );

   while( str->slen > i ) {
      chr = &(str->data[i]);

      if( *chr == split ) {
         if( 0 == blength( current_str ) ) {
            /* Skip all whitespace until the next chunk starts. */
            continue;
         }
         current_str = bfromcstr( "" );
         scaffold_check_null( current_str );
         vector_add( v, current_str );
      } else {
         bconchar( current_str, *chr );
      }

      i++;
   }

cleanup:
   return v;
}

int bgtoi( bstring i_b ) {
   const char* i_c = NULL;
   int i_out = 0;

   i_c = bdata( i_b );
   scaffold_check_null( i_c );

   i_out = atoi( i_c );

cleanup:
   return i_out;
}
