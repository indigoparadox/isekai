
#ifndef VM_H
#define VM_H

#include "scaffold.h"
#include "hashmap.h"

#ifdef USE_DUKTAPE
#ifndef USE_VM
#define USE_VM
#endif /* !USE_VM */
#endif /* USE_DUKTAPE */

#ifdef USE_TURNS
#define VM_TICK_FREQUENCY 1
#else
#define VM_TICK_FREQUENCY 20
#endif /* USE_TURNS */

#define VM_EXEC_TIMEOUT_MS 5000

typedef void (vm_crash_helper)( void* udata, const char* msg );

typedef enum {
   VM_CALLER_NONE,
   VM_CALLER_MOBILE,
   VM_CALLER_CHANNEL
} VM_CALLER_TYPE;

typedef enum {
   VM_LANG_NONE,
   VM_LANG_JS,
   VM_LANG_PYTHON
} VM_LANG;

typedef enum {
   VM_MEMBER_NONE,
   VM_MEMBER_SCRIPT,
   VM_MEMBER_GLOBAL
} VM_MEMBER;

struct VM_CADDY {
   void* caller;
   VM_CALLER_TYPE caller_type;
   void* arg;
   SCAFFOLD_SIZE exec_start; /** When the current script started in ticks. */
   VM_LANG lang;
   struct VM* vm;
   BOOL vm_started;
   struct HASHMAP vm_scripts;
   struct HASHMAP vm_globals;
};

struct MOBILE;
struct CHANNEL;

#define vm_caddy_new( vmc ) \
    vmc = mem_alloc( 1, struct VM_CADDY ); \
    scaffold_check_null( vmc ); \
    vm_caddy_init( vmc );

#ifdef USE_VM
void vm_tick();
BOOL vm_get_tick( SCAFFOLD_SIZE vm_tick_prev );
void vm_caddy_init( struct VM_CADDY* vmc );
void vm_caddy_start( struct VM_CADDY* vmc );
void vm_caddy_do_event( struct VM_CADDY* vmc, const bstring event );
void vm_caddy_end( struct VM_CADDY* vmc );
void vm_caddy_free( struct VM_CADDY* vmc );
BOOL vm_caddy_has_event( const struct VM_CADDY* vmc, const bstring event );
BOOL vm_caddy_put(
   struct VM_CADDY* vmc, VM_MEMBER type, const bstring key, const bstring val
);
SCAFFOLD_SIZE vm_caddy_scripts_count( const struct VM_CADDY* vmc );
#endif /* USE_VM */

#ifdef USE_DUKTAPE
void duktape_helper_channel_crash( void* udata, const char* msg );
void duktape_helper_mobile_crash( void* udata, const char* msg );
void* duktape_set_globals_cb(
   struct CONTAINER_IDX* idx, void* parent, void* iter, void* arg );
void duk_vm_mobile_run( struct VM_CADDY* vmc, const bstring code );
#endif /* USE_DUKTAPE */

#ifdef VM_C
SCAFFOLD_MODULE( "vm.c" );
const struct tagbstring str_vm_global = bsStatic( "global" );
const struct tagbstring str_vm_script = bsStatic( "script" );
const struct tagbstring str_vm_tick = bsStatic( "tick" );
#else
extern struct tagbstring str_vm_global;
extern struct tagbstring str_vm_script;
extern struct tagbstring str_vm_tick;
#endif /* VM_C */

#endif /* VM_H */
