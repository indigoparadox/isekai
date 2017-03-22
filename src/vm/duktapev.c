
#define VM_C

#include "../vm.h"

#include "../graphics.h"
#include "../mobile.h"

#define DUK_USE_EXEC_TIMEOUT_CHECK duktape_use_exec_timeout_check
#define OBJECT_VM( o ) (duk_context*)( o->vm )

#ifdef USE_TURNS
#define VM_TICK_FREQUENCY 1
#else
#define VM_TICK_FREQUENCY 20
#endif /* USE_TURNS */

#include "../duktape/duktape.h"

static SCAFFOLD_SIZE vm_tick_count = 0;

typedef enum VM_CALLER_TYPE {
   VM_CALLER_NONE,
   VM_CALLER_MOBILE,
   VM_CALLER_CHANNEL
} SCRIPT_CALLER_TYPE;

struct VM_CADDY {
   void* caller;
   enum VM_CALLER_TYPE caller_type;
   void* arg;
   SCAFFOLD_SIZE exec_start;
};

static void duktape_helper_channel_crash( void* udata, const char* msg ) {
   struct CHANNEL* l = (struct CHANNEL*)udata;

   scaffold_print_error(
      &module, "Script (%b): %s", l->name, (msg ? msg : "No message.")
   );
}

static void duktape_helper_mobile_crash( void* udata, const char* msg ) {
   struct MOBILE* o = (struct MOBILE*)udata;

   scaffold_print_error(
      &module, "Script (%d): %s", o->serial, (msg ? msg : "No message.")
   );
}

static duk_bool_t duktape_use_exec_timeout_check( void* udata ) {
   struct VM_CADDY* caddy = (struct VM_CADDY*)udata;
   uint64_t now;
   struct MOBILE* caller_o = NULL;
   struct CHANNEL* caller_l = NULL;

   now = graphics_get_ticks();

   switch( caddy->caller_type ) {
   case VM_CALLER_MOBILE:
      caller_o = (struct MOBILE*)caddy->caller;
      break;
   case VM_CALLER_CHANNEL:
      caller_l = (struct CHANNEL*)caddy->caller;
      break;
   }

   if(
      0 != caddy->exec_start &&
      now > (caddy->exec_start + VM_EXEC_TIMEOUT_MS)
   ) {
      return TRUE;
   } else {
      return FALSE;
   }
}

const duk_number_list_entry mobile_update_enum[] = {
   { "moveUp", MOBILE_UPDATE_MOVEUP },
   { "moveDown", MOBILE_UPDATE_MOVEDOWN },
   { "moveLeft", MOBILE_UPDATE_MOVELEFT },
   { "moveRight", MOBILE_UPDATE_MOVERIGHT },
   { "attack", MOBILE_UPDATE_ATTACK },
   { NULL, 0.0 }
};

static void vm_debug( duk_context* vm ) {
   char* text = duk_to_string( vm, -1 );

   scaffold_print_debug( &module, "%s\n", text  );
}

static duk_ret_t vm_random( duk_context* vm ) {
   duk_push_int( vm, rand() );
   return 1;
}

static void vm_update( duk_context* vm ) {
   MOBILE_UPDATE action = (MOBILE_UPDATE)duk_to_int( vm, -1 );
   struct MOBILE_UPDATE_PACKET update = { 0 };
   struct VM_CADDY* caddy = NULL;
   struct MOBILE* o = NULL;

   caddy = (struct VM_CADDY*)duk_heap_udata( vm );

   switch( caddy->caller_type ) {
   case VM_CALLER_MOBILE:
      o = (struct MOBILE*)caddy->caller;

      /* TODO: Call mobile_walk(); */
      /* TODO: Verification that script has permission? */
      update.update = action;
      update.l = o->channel;
      update.o = o;

      mobile_apply_update( &update, TRUE );
      break;

   }
}

static void* vm_global_set_cb( bstring key, void* iter, void* arg ) {
   struct MOBILE* o = (struct MOBILE*)arg;
   bstring value = (bstring)iter;
   duk_idx_t idx;

   scaffold_assert( NULL != value );
   scaffold_assert( NULL != arg );

   duk_push_global_object( OBJECT_VM( o ) );
   duk_push_string( OBJECT_VM( o ), bdata( value ) );
   duk_put_prop_string( OBJECT_VM( o ), -2, bdata( key ) );
   duk_pop( OBJECT_VM( o ) );

   return NULL;
}

/*
static void mobile_vm_global_get_cb( bstring key, void* iter, void* arg ) {
   struct MOBILE* o = (struct MOBILE*)arg;
   bstring value = (bstring)iter;
   char* new_value = NULL;

   scaffold_assert( NULL != value );
   scaffold_assert( NULL != arg );

   //new_value = duk_require_string( OBJECT_VM( o ), 0 );



   return NULL;
}
*/

static duk_ret_t vm_unsafe( duk_context* vm, void* udata ) {
   //VM_CADDY* caddy = (VM_CADDY*)udata;
   const char* code_c = (const char*)udata;

   duk_push_string( vm, code_c );
   duk_eval( vm );

   return 0;
}

static void vm_mobile_run( struct MOBILE* o, const bstring code ) {
   const char* code_c = bdata( code );
   int duk_result = 0;

   scaffold_assert( NULL != code_c );

   duk_push_global_object( OBJECT_VM( o ) );
   duk_push_object( OBJECT_VM( o ) );
   duk_put_number_list( OBJECT_VM( o ), -1, mobile_update_enum );
   duk_put_prop_string( OBJECT_VM( o ), -2, "MobileUpdate" );
   duk_pop( OBJECT_VM( o ) );

   duk_push_global_object( OBJECT_VM( o ) );
   duk_push_c_function( OBJECT_VM( o ), vm_debug, 1 );
   duk_put_prop_string( OBJECT_VM( o ), -2, "debug" );
   duk_pop( OBJECT_VM( o ) );

   duk_push_global_object( OBJECT_VM( o ) );
   duk_push_c_function( OBJECT_VM( o ), vm_random, 0 );
   duk_put_prop_string( OBJECT_VM( o ), -2, "random" );
   duk_pop( OBJECT_VM( o ) );

   duk_push_global_object( OBJECT_VM( o ) );
   duk_push_c_function( OBJECT_VM( o ), vm_update, 1 );
   duk_put_prop_string( OBJECT_VM( o ), -2, "update" );
   duk_pop( OBJECT_VM( o ) );

   duk_result = duk_safe_call( OBJECT_VM( o ), vm_unsafe, code_c, 0, 1 );

   if( 0 == duk_result ) {
      goto cleanup;
   }

   /* An error happened. */
   duk_get_prop_string( OBJECT_VM( o ), 0, "name" );
   duk_get_prop_string( OBJECT_VM( o ), 0, "message" );
   duk_get_prop_string( OBJECT_VM( o ), 0, "fileName" );
   duk_get_prop_string( OBJECT_VM( o ), 0, "lineNumber" );
   duk_get_prop_string( OBJECT_VM( o ), 0, "stack" );

   scaffold_print_error(
      &module, "Script error: %s: %s (%s:%s)\n",
      duk_safe_to_string( OBJECT_VM( o ), 1 ), duk_safe_to_string( OBJECT_VM( o ), 2 ),
      duk_safe_to_string( OBJECT_VM( o ), 3 ), duk_safe_to_string( OBJECT_VM( o ), 4 )
   );
   scaffold_print_error(
      &module, "Script stack: %s\n",
      duk_safe_to_string( OBJECT_VM( o ), 5 )
   );

   duk_pop( OBJECT_VM( o ) );

cleanup:
   return;
}

void vm_mobile_start( struct MOBILE* o ) {
   const char* code_c = NULL;
   int duk_result = 0;

   scaffold_check_not_null( OBJECT_VM( o ) );

   o->vm_started = FALSE;
   o->vm_caddy = scaffold_alloc( 1, struct VM_CADDY );
   scaffold_check_null( o->vm_caddy );

   scaffold_print_debug(
      &module, "Starting script VM for mobile: %d (%b)\n",
      o->serial, o->mob_id
   );

   /* Setup the script caddy. */
   ((struct VM_CADDY*)o->vm_caddy)->exec_start = 0;
   ((struct VM_CADDY*)o->vm_caddy)->caller = o;
   ((struct VM_CADDY*)o->vm_caddy)->caller_type = VM_CALLER_MOBILE;

   /* Setup the VM. */
   o->vm = duk_create_heap(
      NULL, NULL, NULL,
      o->vm_caddy,
      duktape_helper_mobile_crash
   );

   hashmap_iterate( &(o->vm_globals), vm_global_set_cb, o );

cleanup:
   return;
}

void vm_mobile_do_event( struct MOBILE* o, const char* event ) {
   int duk_result = 0;
   bstring tick_script = NULL;

   if( NULL == OBJECT_VM( o ) ) {
      goto cleanup; /* Silently. Not all mobs have scripts. */
   }

   ((struct VM_CADDY*)o->vm_caddy)->exec_start = graphics_get_ticks();

   tick_script = hashmap_get_c( &(o->vm_scripts), event );
   const char* tick_script_c = bdata( tick_script );
   if( NULL != tick_script ) {
      vm_mobile_run( o, tick_script );
   }

cleanup:
   return;
}

void vm_mobile_end( struct MOBILE* o ) {
   scaffold_print_debug(
      &module, "Stopping script VM for mobile: %d (%b)\n", o->serial, o->mob_id
   );

   if( NULL != OBJECT_VM( o ) ) {
      duk_destroy_heap( OBJECT_VM( o ) );
      o->vm = NULL;
   }
   if( NULL != o->vm_caddy ) {
      free( o->vm_caddy );
      o->vm_caddy = NULL;
   }
}

BOOL vm_mobile_has_event( struct MOBILE* o, const char* event ) {
   if(
      NULL != o &&
      HASHMAP_SENTINAL == o->vm_scripts.sentinal &&
      NULL != hashmap_get_c( &(o->vm_scripts), event )
   ) {
      return TRUE;
   } else {
      return FALSE;
   }
}

void vm_tick() {
   vm_tick_count++;
}

/** \brief Returns TRUE if mobiles should be acting.
 */
BOOL vm_get_tick( SCAFFOLD_SIZE vm_tick_prev ) {
#ifdef USE_TURNS
   return vm_tick_count != vm_tick_prev;
#else
   if( 0 == vm_tick_count % VM_TICK_FREQUENCY ) {
      /* scaffold_print_debug( &module, "%d\n", vm_tick_count % VM_TICK_FREQUENCY ); */
      return TRUE;
   }
   return FALSE;
#endif /* USE_TURNS */
}
