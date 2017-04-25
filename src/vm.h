
#ifndef VM_H
#define VM_H

#include "scaffold.h"

#define VM_EXEC_TIMEOUT_MS 5000

struct VM;
struct MOBILE;
struct CHANNEL;

#ifdef USE_VM
void vm_tick();
BOOL vm_get_tick( SCAFFOLD_SIZE vm_tick_prev );
void vm_channel_start( struct CHANNEL* l );
void vm_channel_exec( struct CHANNEL* l, bstring code );
void vm_channel_end( struct CHANNEL* l );
void vm_mobile_start( struct MOBILE* o );
void vm_mobile_do_event( struct MOBILE* o, const char* event );
void vm_mobile_end( struct MOBILE* o );
BOOL vm_mobile_has_event( struct MOBILE* o, const char* event );
#endif /* USE_VM */

#ifdef VM_C
SCAFFOLD_MODULE( "vm.c" );
#endif /* VM_C */

#endif /* VM_H */
