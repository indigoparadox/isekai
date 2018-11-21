
#define VM_C
#include "vm.h"

#ifdef USE_VM

/* This module contains glue applicable to all VMs. A small amount of
 * VM-specific code is allowed, but if there are more than a couple lines
 * of VM-specific code then they should go in that VM's glue module to
 * keep things unjumbled.
 */

#include "mobile.h"
#include "callback.h"

#ifdef USE_DUKTAPE
#include "duktape/duktape.h"
#endif /* USE_DUKTAPE */

static SCAFFOLD_SIZE vm_tick_count = 0;

VBOOL vm_caddy_started( struct VM_CADDY* vmc ) {
   return vmc->vm_started;
}

/** \brief Create a VM caddy, but don't start it.
 */
void vm_caddy_init( struct VM_CADDY* vmc ) {
   scaffold_assert( NULL != vmc );

   vmc->vm = NULL;
   vmc->vm_started = VFALSE;
   hashmap_init( &(vmc->vm_scripts) );
   hashmap_init( &(vmc->vm_globals) );
}

/** \brief Start a created VM caddy.
 */
void vm_caddy_start( struct VM_CADDY* vmc ) {
   scaffold_assert( NULL != vmc );

   switch( vmc->lang ) {
   case VM_LANG_NONE:
      goto cleanup;
#ifdef USE_DUKTAPE
   case VM_LANG_JS:
      /* Setup the VM depending on caller type. */
      switch( vmc->caller_type ) {
      case VM_CALLER_NONE:
         goto cleanup;
      case VM_CALLER_MOBILE:
         vmc->vm = (duk_context*)duk_create_heap(
            NULL, NULL, NULL, vmc, duktape_helper_mobile_crash );
         break;
      case VM_CALLER_CHANNEL:
         vmc->vm = (duk_context*)duk_create_heap(
            NULL, NULL, NULL, vmc, duktape_helper_channel_crash );
         break;
      }
      lgc_null_msg( vmc->vm, "Unable to initialize VM." );

      /* Push all the globals into every new VM. */
      hashmap_iterate( &(vmc->vm_globals), duktape_set_globals_cb, vmc );
      break;
#endif /* USE_DUKTAPE */
   }

   vmc->vm_started = VTRUE;

cleanup:
   return;
}

void vm_caddy_end( struct VM_CADDY* vmc ) {
   scaffold_assert( NULL != vmc );

   switch( vmc->lang ) {
   case VM_LANG_NONE:
      goto cleanup;
#ifdef USE_DUKTAPE
   case VM_LANG_JS:
      if( NULL != vmc->vm ) {
         duk_destroy_heap( vmc->vm );
         vmc->vm = NULL;
      }
      break;
#endif /* USE_DUKTAPE */
   }

   vmc->vm_started = VFALSE;

cleanup:
   return;
}

void vm_tick() {
   vm_tick_count++;
}

/** \brief Returns VTRUE if mobiles should be acting.
 */
VBOOL vm_get_tick( SCAFFOLD_SIZE vm_tick_prev ) {
#ifdef USE_TURNS
   return vm_tick_count != vm_tick_prev;
#else
   if( 0 == vm_tick_count % VM_TICK_FREQUENCY ) {
      /* lg_debug( __FILE__, "%d\n", vm_tick_count % VM_TICK_FREQUENCY ); */
      return VTRUE;
   }
   return VFALSE;
#endif /* USE_TURNS */
}

void vm_caddy_free( struct VM_CADDY* vmc ) {

   lgc_null( vmc );

   hashmap_remove_cb( &(vmc->vm_scripts), callback_h_free_strings, NULL );
   hashmap_cleanup( &(vmc->vm_scripts) );

   hashmap_remove_cb( &(vmc->vm_globals), callback_h_free_strings, NULL );
   hashmap_cleanup( &(vmc->vm_globals) );

cleanup:
   return;
}

VBOOL vm_caddy_has_event( struct VM_CADDY* vmc, const bstring event ) {
   VBOOL retval = VFALSE;

   if(
      NULL != vmc &&
      0 != vm_caddy_scripts_count( vmc ) &&
      NULL != hashmap_get( &(vmc->vm_scripts), event )
   ) {
      retval = VTRUE;
   }

/* cleanup: */
   return retval;
}

void vm_caddy_do_event( struct VM_CADDY* vmc, const bstring event ) {
   bstring tick_script = NULL;

   if( NULL == vmc->vm ) {
      goto cleanup; /* Silently. Not all entities have scripts. */
   }

   vmc->exec_start = graphics_get_ticks();

   tick_script = hashmap_get( &(vmc->vm_scripts), event );
   if( NULL != tick_script ) {
      switch( vmc->lang ) {
      case VM_LANG_NONE:
         goto cleanup;
#ifdef USE_DUKTAPE
      case VM_LANG_JS:
         switch( vmc->caller_type ) {
         case VM_CALLER_NONE:
            goto cleanup;
         case VM_CALLER_MOBILE:
            duk_vm_mobile_run( vmc, tick_script );
            break;
         }
         break;
#endif /* USE_DUKTAPE */
      }
   }

cleanup:
   return;
}

VBOOL vm_caddy_put(
   struct VM_CADDY* vmc, VM_MEMBER type, const bstring key, const bstring val
) {
   struct HASHMAP* dest = NULL;
   VBOOL retval = VFALSE;

   scaffold_assert( NULL != vmc );

   switch( type ) {
   case VM_MEMBER_NONE:
      goto cleanup;
   case VM_MEMBER_GLOBAL:
      dest = &(vmc->vm_globals);
      break;
   case VM_MEMBER_SCRIPT:
      dest = &(vmc->vm_scripts);
      break;
   }

   scaffold_assert( NULL != dest );

   if( hashmap_put(
      dest, key, bstrcpy( val ), VFALSE
   ) ) {
      lg_error( __FILE__,
         "Attempted to double-put script element \"%b\"\n", key );
      retval = VFALSE;
   }

   lg_debug(
      __FILE__, "Stored script element \"%b\"\n", key );

cleanup:
   return retval;
}

SCAFFOLD_SIZE vm_caddy_scripts_count( const struct VM_CADDY* vmc ) {
   SCAFFOLD_SIZE count = 0;
   lgc_null( vmc );
   count = hashmap_count( &(vmc->vm_scripts) );
cleanup:
   return count;
}

#endif /* USE_VM */
