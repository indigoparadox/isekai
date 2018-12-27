
#include "../storage.h"

#include <sqlite3.h>

enum STORAGE_HALF {
   STORAGE_HALF_CLIENT,
   STORAGE_HALF_SERVER
};

static sqlite3* db = NULL;

static struct tagbstring str_version = bsStatic( "version" );
static struct tagbstring qry_stmt = bsStatic( "SELECT value FROM ? WHERE half=? AND key=?" );

VBOOL storage_init() {
   int res = VFALSE,
      version = 0;

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

   version = storage_client_get_int( &str_version );
   if( STORAGE_VERSION_CURRENT < version ) {
      lg_error( __FILE__, "Client storage version is too new: %d\n", version );
      res = VTRUE;
      goto cleanup;
   }

   storage_client_set_int( &str_version, STORAGE_VERSION_CURRENT );

cleanup:
   return res;
}

void storage_free() {
   sqlite3_close( db );
   db = NULL;
}

static sqlite3_stmt* _storage_create_select(
   enum STORAGE_HALF half, bstring key
) {
   sqlite3_stmt* out = NULL;
   int res = 0;

   sqlite3_prepare( db, bdata( &qry_stmt ), blength( &qry_stmt ), &out, NULL );
   lgc_null( out );

   res = sqlite3_bind_text( out, 1, bdata( key ), blength( key ), SQLITE_STATIC );
   lgc_equal( res, SQLITE_OK );

cleanup:
   return out;
}

static sqlite3_stmt* _storage_create_insert(
   enum STORAGE_HALF half, bstring key
) {
   sqlite3_stmt* out = NULL;
   int res = 0;

   sqlite3_prepare( db, bdata( &qry_stmt ), blength( &qry_stmt ), &out, NULL );
   lgc_null( out );

   res = sqlite3_bind_text( out, 1, bdata( key ), blength( key ), SQLITE_STATIC );
   lgc_equal( res, SQLITE_OK );

cleanup:
   return out;
}

bstring storage_client_get_string( bstring key ) {

}

void storage_client_set_string( bstring key, bstring str ) {
}

int storage_client_get_int( bstring key ) {
   sqlite3_stmt* stmt = NULL;
   int res = 0,
      out = 0;

   stmt = _storage_create_select( STORAGE_HALF_CLIENT, key );
   lgc_null( stmt );

   if( SQLITE_ROW == sqlite3_step( stmt ) ) {
      out = sqlite3_column_int( stmt, 0 );
      lg_info( __FILE__, "Storage retrieved: %b: %d\n", key, out );
   }

cleanup:
   sqlite3_finalize( stmt );
   return out;
}

void storage_client_set_int( bstring key, int val ) {
   sqlite3_stmt* stmt = NULL;
   int res = 0,
      test = 0;

   stmt = _storage_create_insert( STORAGE_HALF_CLIENT, key );
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

