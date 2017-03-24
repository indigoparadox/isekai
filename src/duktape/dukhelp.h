
#ifndef DUKHELP_H
#define DUKHELP_H

#include "../scaffold.h"

#define JS_EXEC_TIMEOUT_MS 5000

struct MOBILE;
struct CHANNEL;

typedef enum SCRIPT_CALLER_TYPE {
   SCRIPT_CALLER_NONE,
   SCRIPT_CALLER_MOBILE,
   SCRIPT_CALLER_CHANNEL
} SCRIPT_CALLER_TYPE;

struct SCRIPT_CADDY {
   void* caller;
   enum SCRIPT_CALLER_TYPE caller_type;
   void* arg;
   SCAFFOLD_SIZE exec_start;
};

#ifdef CHANNEL_C
static void duktape_helper_channel_crash( void* udata, const char* msg ) {
   struct CHANNEL* l = (struct CHANNEL*)udata;

   scaffold_print_error(
      &module, "Script (%b): %s", l->name, (msg ? msg : "No message.")
   );
}
#endif /* CHANNEL_C */

#ifdef MOBILE_C
static void duktape_helper_mobile_crash( void* udata, const char* msg ) {
   struct MOBILE* o = (struct MOBILE*)udata;

   scaffold_print_error(
      &module, "Script (%d): %s", o->serial, (msg ? msg : "No message.")
   );
}
#endif /* MOBILE_C */

#if defined( CHANNEL_C ) || defined( MOBILE_C )

#include "duktape.h"

#endif /* MOBILE_C || CHANNEL_C */

#ifdef DUKTAPE_C

#include "duk_config.h"

static duk_bool_t duktape_use_exec_timeout_check( void* udata ) {
   struct SCRIPT_CADDY* caddy = (struct SCRIPT_CADDY*)udata;
   uint64_t now;
   struct MOBILE* caller_o = NULL;
   struct CHANNEL* caller_l = NULL;

   now = graphics_get_ticks();

   switch( caddy->caller_type ) {
   case SCRIPT_CALLER_MOBILE:
      caller_o = (struct MOBILE*)caddy->caller;
      break;
   case SCRIPT_CALLER_CHANNEL:
      caller_l = (struct CHANNEL*)caddy->caller;
      break;
   }

   if(
      0 != caddy->exec_start &&
      now > (caddy->exec_start + JS_EXEC_TIMEOUT_MS)
   ) {
      return TRUE;
   } else {
      return FALSE;
   }
}

#define DUK_USE_EXEC_TIMEOUT_CHECK duktape_use_exec_timeout_check

#endif /* DUKTAPE_C */

#endif /* DUKHELP_H */
