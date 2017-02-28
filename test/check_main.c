
#include <check.h>

int main( void ) {
   int number_failed = 0;
   Suite* sv,
      * sc,
      * sh,
      * sb;
   SRunner* svr,
      * scr,
      * shr,
      * sbr;

   sv = vector_suite();
   svr = srunner_create( sv );
   srunner_run_all( svr, CK_NORMAL );
   number_failed += srunner_ntests_failed( svr );
   srunner_free( svr );

   sc = client_suite();
   scr = srunner_create( sc );
   srunner_run_all( scr, CK_NORMAL );
   number_failed += srunner_ntests_failed( scr );
   srunner_free( scr );

   sh = chunker_suite();
   shr = srunner_create( sh );
   srunner_run_all( shr, CK_NORMAL );
   number_failed += srunner_ntests_failed( shr );
   srunner_free( shr );

   return( number_failed == 0 ) ? 0 : 1;
}
