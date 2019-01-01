
#include "../storage.h"

#include <sqlite3.h>

static sqlite3* db = NULL;

struct tagbstring str_version = bsStatic( "version" );
struct tagbstring str_settings = bsStatic( "settings" );
struct tagbstring str_sql_select = bsStatic( "SELECT value FROM %s WHERE half = ? AND key = ?" );
struct tagbstring str_sql_insert = bsStatic( "INSERT INTO %s (value, half, key) VALUES (?, ?, ?)" );
struct tagbstring str_sql_update = bsStatic( "UPDATE %s SET value=? WHERE half = ? AND key = ?" );

VBOOL storage_init() {
   int res = VFALSE;
   int version_scalar = 0;

   res = sqlite3_open( "storage.db", &db );

   sqlite3_exec(
      db,
      "CREATE TABLE IF NOT EXISTS settings ("
         "key VARCHAR(255) NOT NULL, "
         "half INTEGER NOT NULL, "
         "value VARCHAR(255) NOT NULL, "
         "PRIMARY KEY(key, half)"
      ")",
      NULL,
      NULL,
      NULL
   );

   if( 0 == storage_get( &str_settings, STORAGE_HALF_SERVER, &str_version, STORAGE_VALUE_INT, &version_scalar ) ) {
      lg_error( __FILE__, "Version key missing. Data corrupt!\n" );
      version_scalar = STORAGE_VERSION_CURRENT;
      storage_set( &str_settings, STORAGE_HALF_SERVER, &str_version, STORAGE_VALUE_INT, &version_scalar );
      res = VTRUE;
      goto cleanup;
   } else if( STORAGE_VERSION_CURRENT < version_scalar ) {
      lg_error( __FILE__, "Client storage version is too new: %d\n", version_scalar );
      res = VTRUE;
      goto cleanup;
   } else if( STORAGE_VERSION_CURRENT > version_scalar ) {
      lg_error( __FILE__, "Client storage version is too old: %d\n", version_scalar );
      /* TODO: Upgrade. */
      //res = VTRUE;
      //goto cleanup;

      /* Save the new version to database after upgrading (if applicable). */
      version_scalar = STORAGE_VERSION_CURRENT;
      storage_set( &str_settings, STORAGE_HALF_SERVER, &str_version, STORAGE_VALUE_INT, &version_scalar );
   }

cleanup:
   return res;
}

void storage_free() {
   sqlite3_close( db );
   db = NULL;
}

int storage_get(
   bstring store,
   enum STORAGE_HALF half,
   bstring key,
   enum STORAGE_VALUE_TYPE type,
   void* out
) {
   sqlite3_stmt* stmt = NULL;
   int res = 0,
      ret_count = -1;
   bstring str_sql_temp = NULL;

   str_sql_temp = bformat( bdata( &str_sql_select ), bdata( store ) );
   lgc_null( str_sql_temp );

   lg_debug( __FILE__, "SQL: %b\n", str_sql_temp );
   sqlite3_prepare_v2( db, bdata( str_sql_temp ), -1, &stmt, NULL );
   lgc_null( stmt );

   res = sqlite3_bind_int( stmt, 1, (int)half );
   lgc_equal( res, SQLITE_OK );

   res = sqlite3_bind_text( stmt, 2, bdata( key ), blength( key ), SQLITE_STATIC );
   lgc_equal( res, SQLITE_OK );

   /* We were able to setup without erroring out, so set test count to 0. */
   ret_count = 0;
   while( SQLITE_ROW == sqlite3_step( stmt ) ) {
      switch( type ) {
      case STORAGE_VALUE_INT:
         if( NULL != out ) {
            /* Only give the value if asked (by non-null out). */
            *(int*)out = sqlite3_column_int( stmt, 0 );
         }
         ret_count++;
         lg_debug( __FILE__, "Storage retrieved: %b: %d\n", key, *(int*)out );
         break;

      case STORAGE_VALUE_STRING:
         if( NULL != out ) {
            /* Only give the value if asked (by non-null out). */
            out = sqlite3_column_text( stmt, 0 );
            lgc_null( out );
            out = bfromcstr( out );
         }
         ret_count++;
         lg_debug( __FILE__, "Storage retrieved: %b: %d\n", key, (bstring)out );
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

int storage_set(
   bstring store,
   enum STORAGE_HALF half,
   bstring key,
   enum STORAGE_VALUE_TYPE type,
   void* value
) {
   sqlite3_stmt* stmt = NULL;
   int res = 0,
      test_count = 0,
      ret_count = 0;
   bstring str_sql_temp = NULL;

   test_count = storage_get( store, half, key, type, NULL );

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

   res = sqlite3_bind_int( stmt, 2, (int)half );
   lgc_equal( res, SQLITE_OK );

   res = sqlite3_bind_text( stmt, 3, bdata( key ), blength( key ), SQLITE_STATIC );
   lgc_equal( res, SQLITE_OK );

   switch( type ) {
   case STORAGE_VALUE_INT:
      res = sqlite3_bind_int( stmt, 1, *(int*)value );
      lgc_equal( res, SQLITE_OK );

      res = sqlite3_step( stmt );
      lgc_equal( res, SQLITE_DONE );

      lg_debug( __FILE__, "Storage set: %b: %d\n", key, *(int*)value );
      ret_count++;
      break;

   case STORAGE_VALUE_STRING:
      res = sqlite3_bind_text( stmt, 1, bdata( ((bstring)value) ), -1, SQLITE_STATIC );
      lgc_equal( res, SQLITE_OK );

      res = sqlite3_step( stmt );
      lgc_equal( res, SQLITE_DONE );

      lg_debug( __FILE__, "Storage set: %b: %b\n", key, ((bstring)value) );
      ret_count++;
      break;
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
