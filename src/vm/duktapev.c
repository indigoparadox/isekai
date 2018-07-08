
//#define VM_C

#include "../vm.h"

#ifdef USE_DUKTAPE

SCAFFOLD_MODULE( "duktapev.c" );

#include "../graphics.h"
#include "../mobile.h"
#include "../proto.h"
#include "../channel.h"

#define DUK_USE_EXEC_TIMEOUT_CHECK duktape_use_exec_timeout_check
//#define OBJECT_VM( o ) (duk_context*)( o->vm )

#include "../duktape/duktape.h"

extern struct SERVER* main_server;

void duktape_helper_channel_crash( void* udata, const char* msg ) {
   struct CHANNEL* l = (struct CHANNEL*)udata;

   scaffold_print_error(
      &module, "Script (%b): %s",
      channel_get_name( l ), (msg ? msg : "No message.")
   );
}

void duktape_helper_mobile_crash( void* udata, const char* msg ) {
   struct MOBILE* o = (struct MOBILE*)udata;

   scaffold_print_error(
      &module, "Script (%d): %s", o->serial, (msg ? msg : "No message.")
   );
}

void* duktape_set_globals_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg
) {
   struct VM_CADDY* vmc = (struct VM_CADDY*)arg;
   bstring value = (bstring)iter;

   scaffold_assert( NULL != value );
   scaffold_assert( NULL != arg );
   scaffold_assert( CONTAINER_IDX_STRING == idx->type );

   /* Push each global onto the stack with its identifier. */
   duk_push_global_object( (duk_context*)vmc->vm );
   duk_push_string( (duk_context*)vmc->vm, bdata( value ) );
   duk_put_prop_string( (duk_context*)vmc->vm, -2, bdata( idx->value.key ) );
   duk_pop( (duk_context*)vmc->vm );

   return NULL;
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

static duk_number_list_entry mobile_update_enum[] = {
   { "moveUp", MOBILE_UPDATE_MOVEUP },
   { "moveDown", MOBILE_UPDATE_MOVEDOWN },
   { "moveLeft", MOBILE_UPDATE_MOVELEFT },
   { "moveRight", MOBILE_UPDATE_MOVERIGHT },
   { "attack", MOBILE_UPDATE_ATTACK },
   { NULL, 0.0 }
};

static duk_ret_t duk_cb_vm_debug( duk_context* vm ) {
   const char* text = NULL;

   text = duk_to_string( vm, -1 );

   scaffold_print_debug( &module, "%s\n", text  );

   return 0;
}

static duk_ret_t duk_cb_vm_speak( duk_context* vm ) {
   const char* text = NULL;
   struct VM_CADDY* caddy = NULL;
   bstring b_text = NULL;
   struct MOBILE* o = NULL;

   caddy = (struct VM_CADDY*)duk_heap_udata( vm );

   text = duk_to_string( vm, -1 );

   switch( caddy->caller_type ) {
   case VM_CALLER_MOBILE:
      o = (struct MOBILE*)caddy->caller;

      //mobile_speak( o, b_text );

      /* TODO: This assumes all scripts are server-side, sort of... */

      b_text = bfromcstr( text );
      proto_server_send_msg_channel( main_server, o->channel, o->display_name, b_text );
      break;
   }

cleanup:
   bdestroy( b_text );
   return 0;
}

static duk_ret_t duk_cb_vm_random( duk_context* vm ) {
   duk_push_int( vm, rand() );
   return 0;
}

static duk_ret_t duk_cb_vm_update( duk_context* vm ) {
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

      switch( action ) {
      case MOBILE_UPDATE_MOVEUP:
         update.x = o->x;
         update.y = o->y - 1;
         break;

      case MOBILE_UPDATE_MOVEDOWN:
         update.x = o->x;
         update.y = o->y + 1;
         break;

      case MOBILE_UPDATE_MOVELEFT:
         update.x = o->x - 1;
         update.y = o->y;
         break;

      case MOBILE_UPDATE_MOVERIGHT:
         update.x = o->x + 1;
         update.y = o->y;
         break;
      }

      update.l = o->channel;
      update.o = o;

      mobile_apply_update( &update, TRUE );
      break;

   }

   return 0;
}

static duk_ret_t vm_unsafe( duk_context* vm, void* udata ) {
   //VM_CADDY* caddy = (VM_CADDY*)udata;
   const char* code_c = (const char*)udata;

   duk_push_string( vm, code_c );
   duk_eval( vm );

   return 0;
}

void duk_vm_mobile_run( struct VM_CADDY* vmc, const bstring code ) {
   char* code_c = bdata( code );
   int duk_result = 0;

   scaffold_assert( NULL != code_c );

   duk_push_global_object( (duk_context*)vmc->vm );
   duk_push_object( (duk_context*)vmc->vm );
   duk_put_number_list( (duk_context*)vmc->vm, -1, mobile_update_enum );
   duk_put_prop_string( (duk_context*)vmc->vm, -2, "MobileUpdate" );
   duk_pop( (duk_context*)vmc->vm );

   duk_push_global_object( (duk_context*)vmc->vm );
   duk_push_c_function( (duk_context*)vmc->vm, duk_cb_vm_debug, 1 );
   duk_put_prop_string( (duk_context*)vmc->vm, -2, "debug" );
   duk_pop( (duk_context*)vmc->vm );

   duk_push_global_object( (duk_context*)vmc->vm );
   duk_push_c_function( (duk_context*)vmc->vm, duk_cb_vm_random, 0 );
   duk_put_prop_string( (duk_context*)vmc->vm, -2, "random" );
   duk_pop( (duk_context*)vmc->vm );

   duk_push_global_object( (duk_context*)vmc->vm );
   duk_push_c_function( (duk_context*)vmc->vm, duk_cb_vm_update, 1 );
   duk_put_prop_string( (duk_context*)vmc->vm, -2, "update" );
   duk_pop( (duk_context*)vmc->vm );

   duk_push_global_object( (duk_context*)vmc->vm );
   duk_push_c_function( (duk_context*)vmc->vm, duk_cb_vm_speak, 1 );
   duk_put_prop_string( (duk_context*)vmc->vm, -2, "speak" );
   duk_pop( (duk_context*)vmc->vm );

   duk_result = duk_safe_call( (duk_context*)vmc->vm, vm_unsafe, code_c, 0, 1 );

   if( 0 == duk_result ) {
      goto cleanup;
   }

   /* An error happened. */
   duk_get_prop_string( (duk_context*)vmc->vm, 0, "name" );
   duk_get_prop_string( (duk_context*)vmc->vm, 0, "message" );
   duk_get_prop_string( (duk_context*)vmc->vm, 0, "fileName" );
   duk_get_prop_string( (duk_context*)vmc->vm, 0, "lineNumber" );
   duk_get_prop_string( (duk_context*)vmc->vm, 0, "stack" );

   scaffold_print_error(
      &module, "Script error: %s: %s (%s:%s)\n",
      duk_safe_to_string( (duk_context*)vmc->vm, 1 ),
      //o->mob_id,
      duk_safe_to_string( (duk_context*)vmc->vm, 2 ),
      duk_safe_to_string( (duk_context*)vmc->vm, 3 ),
      duk_safe_to_string( (duk_context*)vmc->vm, 4 )
   );
   scaffold_print_error(
      &module, "Script stack: %s\n",
      duk_safe_to_string( (duk_context*)vmc->vm, 5 )
   );

   duk_pop( (duk_context*)vmc->vm );

cleanup:
   return;
}

#if 0
void vm_channel_start( struct CHANNEL* l ) {
   const char* code_c = NULL;
   int duk_result = 0;

   scaffold_check_not_null( OBJECT_VM( l ) );

   l->vm_started = FALSE;
   l->vm_caddy = mem_alloc( 1, struct VM_CADDY );
   scaffold_check_null( l->vm_caddy );

   scaffold_print_debug(
      &module, "Starting script VM for channel: %b\n", l->name
   );

   /* Setup the script caddy. */
   ((struct VM_CADDY*)l->vm_caddy)->exec_start = 0;
   ((struct VM_CADDY*)l->vm_caddy)->caller = l;
   ((struct VM_CADDY*)l->vm_caddy)->caller_type = VM_CALLER_CHANNEL;

   /* Setup the VM. */
   l->vm = (struct VM*)duk_create_heap(
      NULL, NULL, NULL,
      l->vm_caddy,
      duktape_helper_channel_crash
   );

   /* hashmap_iterate( &(l->vm_globals), vm_global_set_cb, l ); */

cleanup:
   return;
}

void vm_channel_exec( struct CHANNEL* l, bstring code ) {
}

void vm_channel_end( struct CHANNEL* l ) {
   scaffold_print_debug(
      &module, "Stopping script VM for channel: %b\n", l->name
   );

   if( NULL != OBJECT_VM( l ) ) {
      duk_destroy_heap( OBJECT_VM( l ) );
      l->vm = NULL;
   }

   if( NULL != l->vm_caddy ) {
      free( l->vm_caddy );
      l->vm_caddy = NULL;
   }
}

#endif // 0

#endif /* USE_DUKTAPE */
