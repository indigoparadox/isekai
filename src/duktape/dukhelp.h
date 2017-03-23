
#ifndef DUKHELP_H
#define DUKHELP_H

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

#endif /* DUKHELP_H */
