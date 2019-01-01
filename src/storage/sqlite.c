
#include "../storage.h"

#include <sqlite3.h>

static sqlite3* db = NULL;

struct tagbstring str_version = bsStatic( "version" );
struct tagbstring str_settings = bsStatic( "settings" );
struct tagbstring str_sql_select = bsStatic( "SELECT value FROM %s WHERE half = ? AND key = ?" );
struct tagbstring qry_insert = bsStatic( "INSERT INTO %s (half, key, value) VALUES (?, ?, ?)" );
struct tagbstring qry_update = bsStatic( "UPDATE %s SET value=? WHERE half = ? AND key = ?" );

VBOOL storage_init() {
   int res = VFALSE;
   struct VECTOR* version = NULL;
   int* version_scalar = NULL;

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

   version = vector_new();
   if( 0 == storage_get( &str_settings, STORAGE_HALF_SERVER, &str_version, STORAGE_VALUE_INT, version ) ) {
      lg_error( __FILE__, "Version key missing. Data corrupt!\n" );
      storage_client_set_int( &str_settings, &str_version, STORAGE_VERSION_CURRENT );
      res = VTRUE;
      goto cleanup;
   }
   version_scalar = vector_get( version, 0 );
   if( STORAGE_VERSION_CURRENT < *version_scalar ) {
      lg_error( __FILE__, "Client storage version is too new: %d\n", version );
      res = VTRUE;
      goto cleanup;
   } else if( STORAGE_VERSION_CURRENT > *version_scalar ) {
      lg_error( __FILE__, "Client storage version is too old: %d\n", version );
      /* TODO: Upgrade. */
      //res = VTRUE;
      //goto cleanup;

      /* Save the new version to database after upgrading (if applicable). */
      storage_client_set_int( &str_settings, &str_version, STORAGE_VERSION_CURRENT );
   }

cleanup:
   return res;
}

void storage_free() {
   sqlite3_close( db );
   db = NULL;
}

#if 0
static sqlite3_stmt* _storage_create_insert_update(
   bstring store, enum STORAGE_HALF half, bstring key
) {
   sqlite3_stmt* out = NULL,
      * test_present = NULL;
   int res = 0;

   _storage_create_select( store, half, key );

   if( SQLITE_ROW == sqlite3_step( test_present ) ) {
      lg_info( __FILE__, "Value exists, updating: %b\n", key );
   } else {
      lg_info( __FILE__, "Storing new value for: %b\n", key );
   }

   sqlite3_prepare( db, bdata( &str_sql_select ), blength( &str_sql_select ), &out, NULL );
   lgc_null( out );

   res = sqlite3_bind_text( out, 1, bdata( key ), blength( key ), SQLITE_STATIC );
   lgc_equal( res, SQLITE_OK );

cleanup:
   if( NULL == test_present ) {
      lg_error( __FILE__, "%s\n", sqlite3_errstr( sqlite3_errcode( db ) ) );
   } else {
      /* We won't need this again. */
      sqlite3_finalize( test_present );
   }
   return out;
}
#endif // 0

int storage_get(
   bstring store,
   enum STORAGE_HALF half,
   bstring key,
   enum STORAGE_VALUE_TYPE type,
   void* out
) {
   sqlite3_stmt* stmt = NULL;
   int res = 0,
      ret_count = 0,
      * ret_int = 0;
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

   while( SQLITE_ROW == sqlite3_step( stmt ) ) {
      switch( type ) {
      case STORAGE_VALUE_INT:
         ret_int = mem_alloc( 1, int );
         lgc_null( ret_int );
         *out = sqlite3_column_int( stmt, 0 );
         out = ret_int;
         ret_int = NULL;
         ret_count++;
         lg_debug( __FILE__, "Storage retrieved: %b: %d\n", key, *ret_int );
         break;
      }
   }

cleanup:
   if( NULL == stmt ) {
      lg_error( __FILE__, "%s\n", sqlite3_errstr( sqlite3_errcode( db ) ) );
   } else {
      sqlite3_finalize( stmt );
   }
   return ret_count;
}

void storage_set(
   bstring store,
   enum STORAGE_HALF half,
   bstring key,
   enum STORAGE_VALUE_TYPE type,
   void* value
) {
   sqlite3_stmt* stmt = NULL;
   int res = 0,
      test = 0;

   //stmt = _storage_create_insert_update( store, STORAGE_HALF_CLIENT, key );
   lgc_null( stmt );

   res = sqlite3_bind_int( stmt, 2, val );
   lgc_equal( res, SQLITE_OK );

   res = sqlite3_step( stmt );
   lgc_equal( res, SQLITE_DONE );

   lg_info( __FILE__, "Storage set: %b: %d\n", key, val );

cleanup:
   sqlite3_finalize( stmt );
   return;
}

