

void channel_vm_start( struct CHANNEL* l, bstring code ) {
#ifdef USE_DUKTAPEx
   if( NULL == l->vm ) {
      l->vm =
         duk_create_heap( NULL, NULL, NULL, l, duktape_helper_channel_crash );
      /* duk_push_c_function( l->vm, dukt ) */
   }
   duk_peval_string( l->vm, bdata( code ) );
#endif /* USE_DUKTAPE */
#ifdef USE_TINYPY
   tp_obj tp_code_str;
   tp_obj compiled;
   tp_obj r = None;

   /* For macro compatibility. */
   tp_vm* tp = l->vm;

   /* Init the VM and load the code into it. */
   l->vm = tp_init( 0, NULL );
   tp_code_str = tp_string_n( bdata( code ), blength( code ) );
   compiled = tp_compile( l->vm, tp_code_str, tp_string( "<eval>" ) );
   tp_frame( l->vm, l->vm->builtins, (void*)compiled.string.val, &r );
   l->vm_cur = l->vm->cur;

   /* Lock the VM and prepare it to run. (tp_run) */
   if( l->vm->jmp ) {
      tp_raise( , "tp_run(%d) called recusively", l->vm->cur );
   }
   l->vm->jmp = 1;
#ifdef TINYPY_SJLJ
   if( setjmp( l->vm->buf ) ) {
      tp_handle( l->vm );
      scaffold_print_error( &module, "Error executing script for channel.\n" );
   }
#endif /* TINYPY_SJLJ */
#endif /* USE_TINYPY */
}

void channel_vm_step( struct CHANNEL* l ) {
#ifdef USE_TINYPY
   /* void tp_run(tp_vm *tp,int cur) { */
   if( l->vm->cur >= l->vm_cur && l->vm_step_ret != -1 ) {
      l->vm_step_ret = tp_step( l->vm );
   } else {
      scaffold_print_error( &module, "Channel VM stopped: %b\n", l->name );
   }
#endif /* USE_TINYPY */
}

void channel_vm_end( struct CHANNEL* l ) {
#ifdef USE_DUKTAPEx
   if( NULL != l->vm ) {
      duk_destroy_heap( l->vm );
      l->vm = NULL;
   }
#endif /* USE_DUKTAPE */
#ifdef USE_TINYPY
   l->vm->cur = l->vm_cur - 1;
   l->vm->jmp = 0;
   tp_deinit( l->vm );
   l->vm = NULL;
#endif /* USE_TINYPY */
}

BOOL channel_vm_can_step( struct CHANNEL* l ) {
   BOOL retval = FALSE;

#if defined( USE_TINYPY ) || defined( USE_DUKTAPE )
   if( NULL != l->vm ) {
      retval = TRUE;
   }
#endif /* USE_TINYPY || USE_DUKTAPE */

   return retval;
}
