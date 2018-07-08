
#define VM_C
#include "vm.h"

/* This module contains glue applicable to all VMs. A small amount of
 * VM-specific code is allowed, but if there are more than a couple lines
 * of VM-specific code then they should go in that VM's glue module to
 * keep things unjumbled.
 */

#include "mobile.h"
#include "callback.h"

static SCAFFOLD_SIZE vm_tick_count = 0;

/** \brief Create a VM caddy, but don't start it.
 */
void vm_caddy_init( struct VM_CADDY* vmc ) {
   scaffold_assert( NULL != vmc );

   vmc->vm = NULL;
   vmc->vm_started = FALSE;
   hashmap_init( &(vmc->vm_scripts) );
   hashmap_init( &(vmc->vm_globals) );
}

/** \brief Start a created VM caddy.
 */
void vm_caddy_start( struct VM_CADDY* vmc ) {
   scaffold_assert( NULL != vmc );

   switch( vmc->lang ) {
#ifdef USE_DUKTAPE
   case VM_LANG_JS:
      /* Setup the VM depending on caller type. */
      switch( vmc->caller_type ) {
      case VM_CALLER_MOBILE:
         vmc->vm = (struct VM*)duk_create_heap(
            NULL, NULL, NULL, vmc, duktape_helper_mobile_crash );
         break;
      case VM_CALLER_CHANNEL:
         vmc->vm = (struct VM*)duk_create_heap(
            NULL, NULL, NULL, vmc, duktape_helper_channel_crash );
         break;
      }
      scaffold_check_null_msg( vmc->vm, "Unable to initialize VM." );

      /* Push all the globals into every new VM. */
      hashmap_iterate( &(vmc->vm_globals), duktape_set_globals_cb, vmc );
      break;
#endif /* USE_DUKTAPE */
   }

cleanup:
   return;
}

void vm_caddy_end( struct VM_CADDY* vmc ) {
   scaffold_assert( NULL != vmc );

   switch( vmc->lang ) {
#ifdef USE_DUKTAPE
   case VM_LANG_JS:
      if( NULL != vmc->vm ) {
         duk_destroy_heap( vmc->vm );
         vmc->vm = NULL;
      }
      break;
#endif /* USE_DUKTAPE */
   }

cleanup:
   return;
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

void vm_caddy_free( struct VM_CADDY* vmc ) {

   scaffold_check_null( vmc );

   hashmap_remove_cb( &(vmc->vm_scripts), callback_free_strings, NULL );
   hashmap_cleanup( &(vmc->vm_scripts) );

   hashmap_remove_cb( &(vmc->vm_globals), callback_free_strings, NULL );
   hashmap_cleanup( &(vmc->vm_globals) );

cleanup:
   return;
}

BOOL vm_caddy_has_event( struct VM_CADDY* vmc, bstring event ) {
   BOOL retval = FALSE;

   if(
      NULL != vmc &&
      0 != vm_caddy_scripts_count( vmc ) &&
      NULL != hashmap_get( &(vmc->vm_scripts), event )
   ) {
      retval = TRUE;
   }

/* cleanup: */
   return retval;
}

void vm_caddy_do_event( struct VM_CADDY* vmc, bstring event ) {
   bstring tick_script = NULL;

   if( NULL == vmc->vm ) {
      goto cleanup; /* Silently. Not all entities have scripts. */
   }

   vmc->exec_start = graphics_get_ticks();

   tick_script = hashmap_get( &(vmc->vm_scripts), event );
   if( NULL != tick_script ) {
      switch( vmc->lang ) {
#ifdef USE_DUKTAPE
      case VM_LANG_JS:
         switch( vmc->caller_type ) {
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

BOOL vm_caddy_put(
   struct VM_CADDY* vmc, VM_MEMBER type, bstring key, bstring val
) {
   struct HASHMAP* dest = NULL;
   bstring b_type = NULL;
   BOOL retval = FALSE;

   scaffold_assert( NULL != vmc );

   switch( type ) {
   case VM_MEMBER_GLOBAL:
      dest = &(vmc->vm_globals);
      b_type = &(str_vm_global);
      break;
   case VM_MEMBER_SCRIPT:
      dest = &(vmc->vm_scripts);
      b_type = &(str_vm_script);
      break;
   }

   scaffold_assert( NULL != dest );

   if( hashmap_put(
      dest, key, bstrcpy( val ), FALSE
   ) ) {
      scaffold_print_error( &module,
         "Attempted to double-put %b \"%b\"\n",
         b_type, key );
      retval = FALSE;
   }

   scaffold_print_debug(
      &module, "Stored %b \"%b\"\n",
      b_type, key
   );

/* cleanup: */
   return retval;
}

SCAFFOLD_SIZE vm_caddy_scripts_count( struct VM_CADDY* vmc ) {
   SCAFFOLD_SIZE count = 0;
   scaffold_check_null( vmc );
   count = hashmap_count( &(vmc->vm_scripts) );
cleanup:
   return count;
}
