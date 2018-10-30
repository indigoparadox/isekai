
#include "scaffold.h"
#include "plugin.h"

#include "client.h"
#include "channel.h"
#include "input.h"
#include "files.h"

#include <dlfcn.h>

static struct tagbstring str_mode = bsStatic( "mode" );

#define plugin_setup( format, hook ) \
   hook_name = bformat( format, bdata( plug ) ); \
   lgc_null( hook_name ); \
   lg_debug( __FILE__, "Calling plugin function: %b\n", hook_name ); \
   f.hook = dlsym( handle, bdata( hook_name ) ); \
   lgc_null( f.hook );

typedef PLUGIN_RESULT plugin_mode_init();
typedef PLUGIN_RESULT plugin_mode_update( struct CLIENT* c, struct CHANNEL* l );
typedef PLUGIN_RESULT plugin_mode_draw( struct CLIENT* c, struct CHANNEL* l );
typedef PLUGIN_RESULT plugin_mode_poll_input( struct CLIENT* c, struct CHANNEL* l, struct INPUT* p );
typedef PLUGIN_RESULT plugin_mode_free( struct CLIENT* c );

union PLUGIN_CALL_FUNC {
   plugin_mode_init* init;
   plugin_mode_update* update;
   plugin_mode_draw* draw;
   plugin_mode_poll_input* poll_input;
   plugin_mode_free* free;
};

static struct HASHMAP plugin_list_mode;

static void* cb_plugin_load( size_t idx, void* iter, void* arg ) {
   bstring entry = (bstring)iter;
   bstring entry_base = NULL;
   int found = 0;
   int entry_pos = 0;
   char i = NULL;
   PLUGIN_TYPE ptype = *(PLUGIN_TYPE*)arg;

   entry_base = bfromcstr( "" );
   while( blength( entry ) > entry_pos ) {
      i = bchar( entry, entry_pos );
      if( '/' == i ) {
         /* Whatever we were looking at was just a directory. */
         btrunc( entry_base, 0 );
         found = FALSE;
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

cleanup:
   bdestroy( entry_base );
   return NULL;
}

PLUGIN_RESULT plugin_load_all( PLUGIN_TYPE ptype ) {
   bstring plugin_path = NULL;
   PLUGIN_RESULT ret = PLUGIN_FAILURE;
   struct VECTOR plugin_dir = { 0 };

   hashmap_init( &plugin_list_mode );
   vector_init( &plugin_dir );

   plugin_path = bfromcstr( getenv( "PROCIRCD_PLUGINS" ) );
   /* TODO: Plugins directory fallback. */

   lg_debug( __FILE__, "Loading mode plugins from path: %b\n", plugin_path );
   files_list_dir( plugin_path, &plugin_dir, NULL, FALSE, FALSE );

   vector_iterate( &plugin_dir, cb_plugin_load, &ptype );

   return ret;
}

PLUGIN_RESULT plugin_load( PLUGIN_TYPE ptype, bstring plugin_name ) {
   void* handle = NULL;
   bstring plugin_path = NULL;
   PLUGIN_RESULT ret = PLUGIN_FAILURE;
   bstring plugin_filename = NULL;

   plugin_filename = bformat( "lib%s.so", bdata( plugin_name ) );
   plugin_path = bfromcstr( getenv( "PROCIRCD_PLUGINS" ) );
   /* TODO: Plugins directory fallback. */
   switch( ptype ) {
   case PLUGIN_MODE:
      files_join_path( plugin_path, &str_mode );
      break;
   }
   files_join_path( plugin_path, plugin_filename );

   handle = dlopen( bdata( plugin_path ), RTLD_LAZY );

   if( NULL == handle ) {
      lg_error( __FILE__, "Could not open plugin: %s\n", dlerror() );
      goto cleanup;
   }

   /* TODO: Error check. */
   hashmap_put( &plugin_list_mode, bstrcpy( plugin_name ), handle, FALSE );

   ret = plugin_call( ptype, plugin_name, PLUGIN_INIT );
   lg_info( "Plugin %b init returned: %d\n", plugin_name, ret );
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

PLUGIN_RESULT plugin_call_all( PLUGIN_TYPE ptype, PLUGIN_CALL hook, ... ) {
}

PLUGIN_RESULT plugin_call(
   PLUGIN_TYPE ptype, const_bstring plug, PLUGIN_CALL hook, ...
) {
   union PLUGIN_CALL_FUNC f = { 0 };
   PLUGIN_RESULT ret = PLUGIN_FAILURE;
   void* handle = NULL;
   bstring hook_name = NULL;
   va_list varg;
   struct CLIENT* c = NULL;
   struct CHANNEL* l = NULL;
   struct INPUT* p = NULL;

   switch( ptype ) {
      case PLUGIN_MODE:
         handle = hashmap_get( &plugin_list_mode, plug );
         break;
   }
   lgc_null( handle );

   va_start( varg, hook );
   switch( hook ) {
      case PLUGIN_INIT:
         plugin_setup( "mode_%s_init", init );
         ret = f.init();
         break;

      case PLUGIN_UPDATE:
         plugin_setup( "mode_%s_update", update );
         c = va_arg( varg, struct CLIENT* );
         l = va_arg( varg, struct CHANNEL* );
         ret = f.update( c, l );
         break;

      case PLUGIN_DRAW:
         plugin_setup( "mode_%s_draw", draw );
         c = va_arg( varg, struct CLIENT* );
         l = va_arg( varg, struct CHANNEL* );
         ret = f.draw( c, l );
         break;

      case PLUGIN_POLL_INPUT:
         plugin_setup( "mode_%s_poll_input", poll_input );
         c = va_arg( varg, struct CLIENT* );
         l = va_arg( varg, struct CHANNEL* );
         p = va_arg( varg, struct INPUT* );
         ret = f.poll_input( c, l, p );
         break;


      case PLUGIN_FREE:
         plugin_setup( "mode_%s_free", free );
         c = va_arg( varg, struct CLIENT* );
         ret = f.free( c );
         break;
   }
   va_end( varg );

cleanup:
   bdestroy( hook_name );
   return ret;
}
