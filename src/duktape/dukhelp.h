
#ifndef DUKHELP_H
#define DUKHELP_H

static void duktape_helper_channel_crash( void* udata, const char* msg ) {
   struct CHANNEL* l = (struct CHANNEL*)udata;

   scaffold_print_error(
      &module, "Script (%b): %s", l->name, (msg ? msg : "No message.")
   );
}

#endif /* DUKHELP_H */
