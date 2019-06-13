
#include "../storage.h"

#include <sqlite3.h>

static sqlite3* db = NULL;

struct tagbstring str_version = bsStatic( "version" );
struct tagbstring str_settings = bsStatic( "settings" );
struct tagbstring str_sql_select = bsStatic( "SELECT value FROM %s WHERE half = ? AND key = ?" );
struct tagbstring str_sql_select_index = bsStatic( "SELECT value FROM %s WHERE half = ? AND key = ? AND idx = ?" );
struct tagbstring str_sql_insert = bsStatic( "INSERT INTO %s (value, half, key, idx) VALUES (?, ?, ?, ?)" );
struct tagbstring str_sql_update = bsStatic( "UPDATE %s SET value=? WHERE half = ? AND key = ? AND idx = ?" );

bool storage_init() {
   int res = false;
   int* version_scalar = NULL;

   res = sqlite3_open( "storage.db", &db );

   res = sqlite3_exec(
      db,
      "CREATE TABLE IF NOT EXISTS settings ("
         "key VARCHAR(255) NOT NULL, "
         "half INTEGER NOT NULL, "
         "idx INTEGER NOT NULL, "
         "value VARCHAR(255) NOT NULL, "
         "PRIMARY KEY(key, half, idx)"
      ")",
      NULL,
      NULL,
      NULL
   );
   if( res ) {
      lg_error( __FILE__, "%s\n", sqlite3_errstr( sqlite3_errcode( db ) ) );
   }

   if( 0 == storage_get_single( &str_settings, STORAGE_HALF_SERVER, &str_version, 0, STORAGE_VALUE_INT, (void**)&version_scalar ) ) {
      lg_error( __FILE__, "Version key missing. Data corrupt!\n" );
      version_scalar = mem_alloc( 1, int );
      lgc_null( version_scalar );
      *version_scalar = STORAGE_VERSION_CURRENT;
      storage_set_single( &str_settings, STORAGE_HALF_SERVER, &str_version, 0, STORAGE_VALUE_INT, version_scalar );
      res = true;
      goto cleanup;
   } else if( STORAGE_VERSION_CURRENT < *version_scalar ) {
      lg_error( __FILE__, "Client storage version is too new: %d\n", *version_scalar );
      res = true;
      goto cleanup;
   } else if( STORAGE_VERSION_CURRENT > *version_scalar ) {
      lg_error( __FILE__, "Client storage version is too old: %d\n", *version_scalar );
      /* TODO: Upgrade. */
      //res = true;
      //goto cleanup;

      /* Save the new version to database after upgrading (if applicable). */
      *version_scalar = STORAGE_VERSION_CURRENT;
      storage_set_single( &str_settings, STORAGE_HALF_SERVER, &str_version, 0, STORAGE_VALUE_INT, version_scalar );
   }

cleanup:
   mem_free( version_scalar );
   return res;
}

void storage_free() {
   sqlite3_close( db );
   db = NULL;
}

int storage_get_single(
   bstring store,
   enum STORAGE_HALF half,
   bstring key,
   unsigned int index,
   enum STORAGE_VALUE_TYPE type,
   void** out
) {
   struct VECTOR* temp;
   int ret_count = 0;

   if( NULL != out ) {
      temp = vector_new();
      lgc_null( temp );
   }
   ret_count = storage_get( store, half, key, (int)index, type, temp );
   if( 0 < ret_count && NULL != out ) {
      *out = vector_get( temp, 0 );
      vector_remove( temp, 0 );
   }

cleanup:
   vector_free( &temp );
   return ret_count;
}

int storage_get(
   bstring store,
   enum STORAGE_HALF half,
   bstring key,
   int index,
   enum STORAGE_VALUE_TYPE type,
   struct VECTOR* out
) {
   sqlite3_stmt* stmt = NULL;
   int res = 0,
      ret_count = -1;
   bstring str_sql_temp = NULL;
   int* out_int_ptr = NULL;
   bstring out_str = NULL;
   char* out_str_c = NULL;

   if( 0 <= index ) {
      /* We're looking for a specific index. */
      str_sql_temp = bformat( bdata( &str_sql_select_index ), bdata( store ) );
   } else {
      /* Get all values. */
      str_sql_temp = bformat( bdata( &str_sql_select ), bdata( store ) );
   }
   lgc_null( str_sql_temp );

   lg_debug( __FILE__, "SQL: %b\n", str_sql_temp );
   sqlite3_prepare_v2( db, bdata( str_sql_temp ), -1, &stmt, NULL );
   lgc_null( stmt );

   res = sqlite3_bind_int( stmt, 1, (int)half );
   lgc_equal( res, SQLITE_OK );

   res = sqlite3_bind_text( stmt, 2, bdata( key ), blength( key ), SQLITE_STATIC );
   lgc_equal( res, SQLITE_OK );

   if( 0 <= index ) {
      /* This field only exists if we used the _index statement above. */
      res = sqlite3_bind_int( stmt, 3, index );
      lgc_equal( res, SQLITE_OK );
   }

   /* We were able to setup without erroring out, so set test count to 0. */
   ret_count = 0;
   while( SQLITE_ROW == sqlite3_step( stmt ) ) {
      switch( type ) {
      case STORAGE_VALUE_INT:
         if( NULL != out ) {
            /* Only give the value if asked (by non-null out). */
            out_int_ptr = mem_alloc( 1, int );
            lgc_null( out_int_ptr );
            *out_int_ptr = sqlite3_column_int( stmt, 0 );
            vector_add( out, out_int_ptr );
            lg_debug( __FILE__, "Storage retrieved: %b: %d\n", key, *out_int_ptr );
            out_int_ptr = NULL;
         }
         ret_count++;
         break;

      case STORAGE_VALUE_STRING:
         if( NULL != out ) {
            /* Only give the value if asked (by non-null out). */
            out_str_c = sqlite3_column_text( stmt, 0 );
            lgc_null( out_str_c );
            out_str = bfromcstr( out_str_c );
            lgc_null( out_str );
            vector_add( out, out_str );
            out_str_c = NULL;
            lg_debug( __FILE__, "Storage retrieved: %b: %d\n", key, out_str );
            out_str = NULL;
         }
         ret_count++;
         break;

      case STORAGE_VALUE_NONE:
         /* PASS */
         break;
      }
   }

cleanup:
   if( NULL == stmt ) {
      lg_error( __FILE__, "%s\n", sqlite3_errstr( sqlite3_errcode( db ) ) );
   } else {
      sqlite3_finalize( stmt );
   }
   bdestroy( str_sql_temp );
   return ret_count;
}

int storage_set_single(
   bstring store,
   enum STORAGE_HALF half,
   bstring key,
   int index,
   enum STORAGE_VALUE_TYPE type,
   void* value
) {
   struct VECTOR* temp = NULL;
   int ret_count = 0;

   temp = vector_new();
   lgc_null( temp );

   vector_add( temp, value );

   ret_count = storage_set( store, half, key, index, type, temp );
   vector_remove( temp, 0 );

cleanup:
   vector_free( &temp );
   return ret_count;
}

int storage_set(
   bstring store,
   enum STORAGE_HALF half,
   bstring key,
   int index,
   enum STORAGE_VALUE_TYPE type,
   struct VECTOR* value
) {
   sqlite3_stmt* stmt = NULL;
   int res = 0,
      test_count = 0,
      ret_count = 0,
      * value_int_ptr = NULL;
   bstring str_sql_temp = NULL,
      value_str_ptr = NULL;

   test_count = storage_get( store, half, key, index, type, NULL );

   if( 0 < test_count ) {
      /* Item exists, so update. */
      str_sql_temp = bformat( bdata( &str_sql_update ), bdata( store ) );
      lgc_null( str_sql_temp );

      lg_debug( __FILE__, "SQL: %b\n", str_sql_temp );
      sqlite3_prepare_v2( db, bdata( str_sql_temp ), -1, &stmt, NULL );
      lgc_null( stmt );
   } else  {
      /* Item does not exist, so insert. */
      str_sql_temp = bformat( bdata( &str_sql_insert ), bdata( store ) );
      lgc_null( str_sql_temp );

      lg_debug( __FILE__, "SQL: %b\n", str_sql_temp );
      sqlite3_prepare_v2( db, bdata( str_sql_temp ), -1, &stmt, NULL );
      lgc_null( stmt );
   }

   if( 0 > index ) {
      /* If negative index, then use the highest index +1. */
      index = test_count;
   }

   res = sqlite3_bind_int( stmt, 2, (int)half );
   lgc_equal( res, SQLITE_OK );

   res = sqlite3_bind_text( stmt, 3, bdata( key ), blength( key ), SQLITE_STATIC );
   lgc_equal( res, SQLITE_OK );

   res = sqlite3_bind_int( stmt, 4, index );
   lgc_equal( res, SQLITE_OK );

   switch( type ) {
   case STORAGE_VALUE_INT:
      value_int_ptr = vector_get( value, 0 );
      res = sqlite3_bind_int( stmt, 1, *value_int_ptr );
      lgc_equal( res, SQLITE_OK );

      res = sqlite3_step( stmt );
      lgc_equal( res, SQLITE_DONE );

      lg_debug( __FILE__, "Storage set: %b: %d\n", key, *value_int_ptr );
      ret_count++;
      break;

   case STORAGE_VALUE_STRING:
      value_str_ptr = vector_get( value, 0 );
      res = sqlite3_bind_text( stmt, 1, bdata( value_str_ptr ), -1, SQLITE_STATIC );
      lgc_equal( res, SQLITE_OK );

      res = sqlite3_step( stmt );
      lgc_equal( res, SQLITE_DONE );

      lg_debug( __FILE__, "Storage set: %b: %b\n", key, value_str_ptr );
      ret_count++;
      break;

   case STORAGE_VALUE_NONE:
      /* PASS */
      break;
   }

cleanup:
   if( 0 != res || NULL == stmt ) {
      lg_error( __FILE__, "%s\n", sqlite3_errstr( sqlite3_errcode( db ) ) );
   } else {
      sqlite3_finalize( stmt );
   }
   bdestroy( str_sql_temp );
   return ret_count;
}
