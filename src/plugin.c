
#define PLUGIN_C
#include "scaffold.h"
#include "plugin.h"

#include "client.h"
#include "channel.h"
#include "input.h"
#include "files.h"
#include "callback.h"
#include "action.h"

#ifdef USE_DYNAMIC_PLUGINS
#include <dlfcn.h>

static struct tagbstring str_mode = bsStatic( "mode" );

#define plugin_setup( format, hook ) \
   hook_name = bformat( format, bdata( plug ) ); \
   lgc_null( hook_name ); \
   f.hook = dlsym( handle, bdata( hook_name ) ); \
   lgc_null( f.hook );

#define plugin_f( method, ... ) \
    f.method( __VA_ARGS__ )

#else

#define plugin_setup( format, hook ) \
   hook_name = bformat( format, bdata( plug ) ); \
   lgc_null( hook_name ); \
   f.hook = hashmap_get( plugin_list_mode, hook_name ); \
   lgc_null( f.hook );

#define plugin_f( method, ... ) \
    f.method( __VA_ARGS__ )

#endif /* USE_DYNAMIC_PLUGINS */

typedef PLUGIN_RESULT plugin_mode_init();
typedef PLUGIN_RESULT plugin_mode_update( struct CLIENT* c, struct CHANNEL* l );
typedef PLUGIN_RESULT plugin_mode_draw( struct CLIENT* c, struct CHANNEL* l );
typedef PLUGIN_RESULT plugin_mode_poll_input( struct CLIENT* c, struct CHANNEL* l, struct INPUT* p );
typedef PLUGIN_RESULT plugin_mode_free( struct CLIENT* c );
typedef PLUGIN_RESULT plugin_mode_mobile_action_client( struct ACTION_PACKET* update );
typedef PLUGIN_RESULT plugin_mode_mobile_action_server( struct ACTION_PACKET* update );
typedef PLUGIN_RESULT plugin_mode_mobile_init( struct MOBILE* o, struct CHANNEL* l );
typedef PLUGIN_RESULT plugin_mode_client_init( struct CLIENT* c, struct CHANNEL* l );
typedef PLUGIN_RESULT plugin_mode_mobile_free( struct MOBILE* o );
typedef PLUGIN_RESULT plugin_mode_client_free( struct CLIENT* c );

union PLUGIN_CALL_FUNC {
   plugin_mode_init* init;
   plugin_mode_update* update;
   plugin_mode_draw* draw;
   plugin_mode_poll_input* poll_input;
   plugin_mode_free* free;
   plugin_mode_mobile_action_client* mobile_action_client;
   plugin_mode_mobile_action_server* mobile_action_server;
   plugin_mode_mobile_init* mobile_init;
   plugin_mode_client_init* client_init;
   plugin_mode_mobile_free* mobile_free;
   plugin_mode_client_free* client_free;
};

struct PLUGIN_CALL_CONTAINER {
   plugin_mode_init* init;
   plugin_mode_update* update;
   plugin_mode_draw* draw;
   plugin_mode_poll_input* poll_input;
   plugin_mode_free* free;
   plugin_mode_mobile_action_client* mobile_action_client;
   plugin_mode_mobile_action_server* mobile_action_server;
   plugin_mode_mobile_init* mobile_init;
   plugin_mode_client_init* client_init;
   plugin_mode_mobile_free* mobile_free;
   plugin_mode_client_free* client_free;
};

#ifdef USE_DYNAMIC_PLUGINS
static void* cb_plugin_load( size_t idx, void* iter, void* arg ) {
   bstring entry = (bstring)iter;
   bstring entry_base = NULL;
   int found = 0;
   int entry_pos = 0;
   char i = '\0';
   PLUGIN_TYPE ptype = *(PLUGIN_TYPE*)arg;

   entry_base = bfromcstr( "" );
   while( blength( entry ) > entry_pos ) {
      i = bchar( entry, entry_pos );
      if( '/' == i ) {
         /* Whatever we were looking at was just a directory. */
         btrunc( entry_base, 0 );
         found = false;
      } else {
         if(
            (0 == found && 'l' == i) ||
            (1 == found && 'i' == i) ||
            (2 == found && 'b' == i)
         ) {
            /* All lib names start with lib. */
            found++;
         } else if( 3 == found ) {
            if( '.' == i ) {
               /* Library name complete. */
               found = 4;
            } else {
               /* Add characters to the lib name until we hit a . */
               bconchar( entry_base, i );
            }
         } else if(
            (4 == found && 's' == i) ||
            (5 == found && 'o' == i)
         ) {
            found++;
         }
      }
      entry_pos++;
   }

   if( 6 == found ) {
      lg_info( __FILE__, "Plugin found: %b\n", entry_base );
      plugin_load( ptype, entry_base );
   }

/* cleanup: */
   bdestroy( entry_base );
   return NULL;
}

static bstring plugin_get_path() {
   bstring plugin_path = NULL;
   plugin_path = bfromcstr( getenv( "ISEKAI_PLUGINS" ) );
   /* TODO: Plugins directory fallback. */
   if( NULL == plugin_path ) {
      plugin_path = bfromcstr( "" );
   }
   if( 0 >= blength( plugin_path ) ) {
      bassignStatic( plugin_path, "plugins/bin" );
   }
   return plugin_path;
}

#else

#if defined( USE_STATIC_MODE_ISOMETRIC )
struct tagbstring mode_name_isometric = bsStatic( "Isometric" );
struct PLUGIN_CALL_CONTAINER ctr_isometer = {
   mode_pov_init,
   mode_pov_update,
   mode_pov_draw,
   mode_pov_poll_input,
   mode_pov_free,
   mode_pov_mobile_action_client,
   mode_pov_mobile_action_server,
   mode_pov_mobile_init,
   mode_pov_client_init,
   mode_pov_mobile_free,
   mode_pov_client_free
};
#endif /* USE_STATIC_MODE_ISOMETRIC */

#if defined( USE_STATIC_MODE_POV )
struct tagbstring mode_name_pov = bsStatic( "POV" );
struct PLUGIN_CALL_CONTAINER ctr_pov = {
   mode_pov_init,
   mode_pov_update,
   mode_pov_draw,
   mode_pov_poll_input,
   mode_pov_free,
   mode_pov_mobile_action_client,
   mode_pov_mobile_action_server,
   mode_pov_mobile_init,
   mode_pov_client_init,
   mode_pov_mobile_free,
   mode_pov_client_free
};
#endif /* USE_STATIC_MODE_POV */

#if defined( USE_STATIC_MODE_POV )
struct tagbstring mode_name_topdown = bsStatic( "Top Down" );
struct PLUGIN_CALL_CONTAINER ctr_topdown = {
   mode_topdown_init,
   mode_topdown_update,
   mode_topdown_draw,
   mode_topdown_poll_input,
   mode_topdown_free,
   mode_topdown_mobile_action_client,
   mode_topdown_mobile_action_server,
   mode_topdown_mobile_init,
   mode_topdown_client_init,
   mode_topdown_mobile_free,
   mode_topdown_client_free
};
#endif /* USE_STATIC_MODE_POV */

#endif /* USE_DYNAMIC_PLUGINS */

/*
extern struct tagbstring mode_key;
extern struct tagbstring mode_name;
*/

PLUGIN_RESULT plugin_load_all( PLUGIN_TYPE ptype ) {
   PLUGIN_RESULT ret = PLUGIN_FAILURE;
   bstring plugin_path = NULL;
#ifdef USE_DYNAMIC_PLUGINS
   struct VECTOR* plugin_dir;

#endif /* USE_DYNAMIC_PLUGINS */

   mode_list_pretty = vector_new();
   mode_list_short = vector_new();

   plugin_list_mode = hashmap_new();

#ifdef USE_DYNAMIC_PLUGINS
   plugin_dir = vector_new();

   plugin_path = plugin_get_path();

   lg_debug( __FILE__, "Loading mode plugins from path: %b\n", plugin_path );
   files_list_dir( plugin_path, plugin_dir, NULL, false, false );

   vector_iterate( plugin_dir, cb_plugin_load, &ptype );

   bdestroy( plugin_path );
   vector_remove_cb( plugin_dir, callback_v_free_strings, NULL );
   vector_free( &plugin_dir );
#else

#if defined( USE_STATIC_MODE_ISOMETRIC )
   ret = mode_isometer_init();
   if( PLUGIN_FAILURE == ret ) {
      goto cleanup;
   }
   hashmap_put( plugin_list_mode, &mode_name_isometric, &ctr_isometer, false );
   vector_add( mode_list_pretty, &mode_name_isometric );
   vector_add( mode_list_short, bfromcstr( "isometer" ) );
#endif /* USE_STATIC_MODE_ISOMETRIC */
#if defined( USE_STATIC_MODE_POV )
   ret = mode_pov_init();
   if( PLUGIN_FAILURE == ret ) {
      goto cleanup;
   }
   hashmap_put( plugin_list_mode, &mode_name_pov, &ctr_pov, false );
   vector_add( mode_list_pretty, &mode_name_pov );
   vector_add( mode_list_short, bfromcstr( "pov" ) );
#endif /* USE_STATIC_MODE_POV */
#if defined( USE_STATIC_MODE_TOPDOWN )
   ret = mode_topdown_init();
   if( PLUGIN_FAILURE == ret ) {
      goto cleanup;
   }
   hashmap_put( plugin_list_mode, &mode_name_topdown, &ctr_topdown, false );
   vector_add( mode_list_pretty, &mode_name_topdown );
   vector_add( mode_list_short, bfromcstr( "topdown" ) );
#endif /* USE_STATIC_MODE_TOPDOWN */

#endif /* USE_DYNAMIC_PLUGINS */

cleanup:
   return ret;
}

#ifdef USE_DYNAMIC_PLUGINS
PLUGIN_RESULT plugin_load( PLUGIN_TYPE ptype, bstring plugin_name ) {
   PLUGIN_RESULT ret = PLUGIN_FAILURE;
   void* handle = NULL;
   bstring plugin_path = NULL;
   bstring plugin_filename = NULL;
   bstring mode_name = NULL;

   plugin_filename = bformat( "lib%s.so", bdata( plugin_name ) );
   plugin_path = plugin_get_path();

   switch( ptype ) {
   case PLUGIN_MODE:
      files_join_path( plugin_path, &str_mode );
      break;
   }
   files_join_path( plugin_path, plugin_filename );

   lg_debug( __FILE__, "Opening plugin from: %s\n", bdata( plugin_path ) );
   handle = dlopen( bdata( plugin_path ), RTLD_LAZY );

   if( NULL == handle ) {
      lg_error( __FILE__, "Could not open plugin: %s\n", dlerror() );
      goto cleanup;
   }

   /* TODO: Error check. */
   hashmap_put( plugin_list_mode, plugin_name, handle, false );

   if( PLUGIN_MODE == ptype ) {
      mode_name = dlsym( handle, "mode_name" );
      lg_debug( __FILE__, "Adding %s to mode name list...\n", bdata( mode_name ) );
      vector_add( mode_list_pretty, mode_name );
      vector_add( mode_list_short, bstrcpy( plugin_name ) );
   }

   ret = plugin_call( ptype, plugin_name, PLUGIN_INIT );
   lg_info( __FILE__,  "Plugin %b init returned: %d\n", plugin_name, ret );
   if( PLUGIN_SUCCESS != ret ) {
      goto cleanup;
   }

cleanup:
   bdestroy( plugin_path );
   bdestroy( plugin_filename );
   ret = plugin_call( ptype, plugin_name, PLUGIN_INIT );
   return ret;
}
#endif /* USE_DYNAMIC_PLUGINS */

PLUGIN_RESULT plugin_unload_all( PLUGIN_TYPE ptype ) {
   vector_free( &mode_list_pretty ); /* These are static strings. */
   vector_free( &mode_list_short );
}

#ifdef USE_DYNAMIC_PLUGINS
PLUGIN_RESULT plugin_unload( PLUGIN_TYPE ptype, bstring plugin_name ) {
   void* handle = NULL;
   PLUGIN_RESULT ret = PLUGIN_FAILURE;

   switch( ptype ) {
      case PLUGIN_MODE:
         handle = hashmap_get( plugin_list_mode, plugin_name );
         break;
   }
   lgc_null( handle );

   ret = plugin_call( ptype, plugin_name, PLUGIN_FREE );
   lg_info( __FILE__,  "Plugin %b free returned: %d\n", plugin_name, ret );
   if( PLUGIN_SUCCESS != ret ) {
      goto cleanup;
   }

   if( 0 != dlclose( handle ) ) {
      lg_error( __FILE__, "Could not close plugin: %s\n", dlerror() ) ;
      goto cleanup;
   }

cleanup:
   return ret;
}
#endif /* USE_DYNAMIC_PLUGINS */

PLUGIN_RESULT plugin_call_all( PLUGIN_TYPE ptype, PLUGIN_CALL hook, ... ) {
}

PLUGIN_RESULT plugin_call(
   PLUGIN_TYPE ptype, const bstring plug, PLUGIN_CALL hook, ...
) {
   PLUGIN_RESULT ret = PLUGIN_FAILURE;
   bstring hook_name = NULL;
   va_list varg;
   struct CLIENT* c = NULL;
   struct CHANNEL* l = NULL;
   struct INPUT* p = NULL;
   struct ACTION_PACKET* update = NULL;
   struct MOBILE* o = NULL;

#ifdef USE_DYNAMIC_PLUGINS
   void* handle = NULL;
   union PLUGIN_CALL_FUNC f = { 0 };

   switch( ptype ) {
      case PLUGIN_MODE:
         handle = hashmap_get( plugin_list_mode, plug );
         break;
   }
   if( NULL == handle ) {
      lg_error( __FILE__, "Could not call plugin: %b\n", plug );
   }
   lgc_null( handle );
#else
   struct PLUGIN_CALL_CONTAINER f = { 0 };
#endif /* USE_DYNAMIC_PLUGINS */

   va_start( varg, hook );
   switch( hook ) {
      case PLUGIN_INIT:
         plugin_setup( "mode_%s_init", init );
         #ifdef DEBUG_PLUGIN_CALL
         lg_debug( __FILE__, "Calling plugin function: %b\n", hook_name );
         #endif /* DEBUG_PLUGIN_CALL */
         ret = plugin_f( init );
         break;

      case PLUGIN_UPDATE:
         plugin_setup( "mode_%s_update", update );
         #ifdef DEBUG_PLUGIN_CALL
         lg_debug( __FILE__, "Calling plugin function: %b\n", hook_name );
         #endif /* DEBUG_PLUGIN_CALL */
         c = va_arg( varg, struct CLIENT* );
         l = va_arg( varg, struct CHANNEL* );
         ret = plugin_f( update, c, l );
         break;

      case PLUGIN_DRAW:
         plugin_setup( "mode_%s_draw", draw );
         #ifdef DEBUG_PLUGIN_CALL
         lg_debug( __FILE__, "Calling plugin function: %b\n", hook_name );
         #endif /* DEBUG_PLUGIN_CALL */
         c = va_arg( varg, struct CLIENT* );
         l = va_arg( varg, struct CHANNEL* );
         ret = plugin_f( draw, c, l );
         break;

      case PLUGIN_POLL_INPUT:
         plugin_setup( "mode_%s_poll_input", poll_input );
         #ifdef DEBUG_PLUGIN_CALL
         lg_debug( __FILE__, "Calling plugin function: %b\n", hook_name );
         #endif /* DEBUG_PLUGIN_CALL */
         c = va_arg( varg, struct CLIENT* );
         l = va_arg( varg, struct CHANNEL* );
         p = va_arg( varg, struct INPUT* );
         ret = plugin_f( poll_input, c, l, p );
         break;

      case PLUGIN_MOBILE_ACTION_SERVER:
         plugin_setup( "mode_%s_mobile_action_server", mobile_action_server );
         #ifdef DEBUG_PLUGIN_CALL
         lg_debug( __FILE__, "Calling plugin function: %b\n", hook_name );
         #endif /* DEBUG_PLUGIN_CALL */
         update = va_arg( varg, struct ACTION_PACKET* );
         ret = plugin_f( mobile_action_server, update );
         break;

      case PLUGIN_MOBILE_ACTION_CLIENT:
         plugin_setup( "mode_%s_mobile_action_client", mobile_action_client );
         #ifdef DEBUG_PLUGIN_CALL
         lg_debug( __FILE__, "Calling plugin function: %b\n", hook_name );
         #endif /* DEBUG_PLUGIN_CALL */
         update = va_arg( varg, struct ACTION_PACKET* );
         ret = plugin_f( mobile_action_client, update );
         break;

      case PLUGIN_CLIENT_INIT:
         plugin_setup( "mode_%s_client_init", client_init );
         #ifdef DEBUG_PLUGIN_CALL
         lg_debug( __FILE__, "Calling plugin function: %b\n", hook_name );
         #endif /* DEBUG_PLUGIN_CALL */
         c = va_arg( varg, struct CLIENT* );
         l = va_arg( varg, struct CHANNEL* );
         ret = plugin_f( client_init, c, l );
         break;

      case PLUGIN_MOBILE_INIT:
         plugin_setup( "mode_%s_mobile_init", mobile_init );
         #ifdef DEBUG_PLUGIN_CALL
         lg_debug( __FILE__, "Calling plugin function: %b\n", hook_name );
         #endif /* DEBUG_PLUGIN_CALL */
         o = va_arg( varg, struct MOBILE* );
         l = va_arg( varg, struct CHANNEL* );
         ret = plugin_f( mobile_init, o, l );
         break;

      case PLUGIN_CLIENT_FREE:
         plugin_setup( "mode_%s_client_free", client_free );
         #ifdef DEBUG_PLUGIN_CALL
         lg_debug( __FILE__, "Calling plugin function: %b\n", hook_name );
         #endif /* DEBUG_PLUGIN_CALL */
         c = va_arg( varg, struct CLIENT* );
         ret = plugin_f( client_free, c );
         break;

      case PLUGIN_MOBILE_FREE:
         plugin_setup( "mode_%s_mobile_free", mobile_free );
         #ifdef DEBUG_PLUGIN_CALL
         lg_debug( __FILE__, "Calling plugin function: %b\n", hook_name );
         #endif /* DEBUG_PLUGIN_CALL */
         o = va_arg( varg, struct MOBILE* );
         ret = plugin_f( mobile_free, o );
         break;

      case PLUGIN_FREE:
         plugin_setup( "mode_%s_free", free );
         #ifdef DEBUG_PLUGIN_CALL
         lg_debug( __FILE__, "Calling plugin function: %b\n", hook_name );
         #endif /* DEBUG_PLUGIN_CALL */
         c = va_arg( varg, struct CLIENT* );
         ret = plugin_f( free, c );
         break;
   }
   va_end( varg );

cleanup:
   bdestroy( hook_name );
   return ret;
}

bstring plugin_get_mode_short( int mode ) {
   return vector_get( mode_list_short, mode );
}

bstring plugin_get_mode_name( int mode ) {
   return vector_get( mode_list_pretty, mode );
}

int plugin_count() {
   return vector_count( mode_list_short );
}

struct VECTOR* plugin_get_mode_name_list() {
   return mode_list_pretty;
}
