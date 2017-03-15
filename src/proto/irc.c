
#define PROTO_C
#include "../proto.h"

#include "../callback.h"
#include "../chunker.h"
#include "../datafile.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef enum _IRC_ERROR {
   ERR_NONICKNAMEGIVEN = 431,
   ERR_NICKNAMEINUSE = 433,
   ERR_NOMOTD = 422
} IRC_ERROR;

const struct tagbstring irc_reply_error_text[35] = {
   /*  1 */ bsStatic( "Nick :No such nick/channel" ),
   bsStatic( "" ),
   /*  3 */ bsStatic( "Channel :No such channel" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   /* 21 */ bsStatic( "Command :Unknown command" ),
   /* 22 */ bsStatic( ":MOTD File is missing" ),
   bsStatic( "" ),
   /* 24 */ bsStatic( ":File error doing operation on file" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   bsStatic( "" ),
   /* 31 */ bsStatic( ":No nickname given" ),
   bsStatic( "" ),
   /* 33 */ bsStatic( "Nick :Nickname is already in use" ),
};

#define IRC_STANZA_ALLOC 80

#define irc_error( enum_index ) \
   client_printf( \
      c, ":%b %d %b %b", \
      s->self.remote, enum_index, c->nick, irc_reply_error_text[enum_index - 400] \
   );

#define irc_detect_malformed( args_qty, expression ) \
   if( args_qty != args->qty ) { \
      scaffold_print_error( \
         &module, "IRC: Malformed " expression " expression received.\n" ); \
      scaffold_error = SCAFFOLD_ERROR_MISC; \
      goto cleanup; \
   }

void proto_register( struct CLIENT* c ) {
   client_printf( c, "NICK %b", c->nick );
   client_printf( c, "USER %b", c->realname );
}

void proto_send_chunk(
   struct CLIENT* c, struct CHUNKER* h, SCAFFOLD_SIZE start_pos,
   const bstring filename, const bstring data
) {
   scaffold_assert_server();
   /* Note the starting point and progress for the client. */
   client_printf(
      c, ":server GDB %b TILEMAP %b %d %d %d %d : %b",
      c->nick, filename, h->type, start_pos,
      h->tx_chunk_length, h->raw_length, data
   );
   return;
}

void proto_abort_chunker( struct CLIENT* c, struct CHUNKER* h ) {
   scaffold_assert_client();
   scaffold_print_debug(
      &module,
      "Client: Aborting transfer of %s from server due to cached copy.\n",
      bdata( h->filename )
   );
   client_printf( c, "GDA %b", h->filename );
}

/** \brief This should ONLY be called by client_request_file(). All requests for
 *         files should go through that function.
 * \param
 * \param
 * \return
 */
void proto_request_file( struct CLIENT* c, const bstring filename, CHUNKER_DATA_TYPE type ) {
   scaffold_assert_client();
   scaffold_print_debug(
      &module, "Client: Requesting file: %s\n", bdata( filename )
   );
   client_printf( c, "GRF %d %b", type, filename );
}

void proto_send_mob( struct CLIENT* c, struct MOBILE* o ) {
   scaffold_assert_server();
   client_printf(
      c, "MOB %b %d %b %b %b %d %d",
      o->channel->name, o->serial, o->mob_id, o->def_filename, o->owner->nick, o->x, o->y
   );
}

void proto_client_send_update( struct CLIENT* c, struct MOBILE_UPDATE_PACKET* update ) {
   scaffold_assert_client();
   client_printf(
      c, "GU %b %d %d",
      update->l->name, update->o->serial, update->update
   );
}

void proto_server_send_update( struct CLIENT* c, struct MOBILE_UPDATE_PACKET* update ) {
   scaffold_assert_server();
   client_printf(
      c, "GU %b %d %d",
      update->l->name, update->o->serial, update->update
   );
}

void proto_client_stop( struct CLIENT* c ) {
   scaffold_assert_client();
   client_printf( c, "QUIT" );
}

void proto_client_request_mobs( struct CLIENT* c, struct CHANNEL* l ) {
   client_printf( c, "WHO %b", l->name );
}

#ifdef DEBUG_VM

void proto_client_debug_vm(
   struct CLIENT* c, struct CHANNEL* l, const bstring code
) {
   client_printf( c, "DEBUGVM %b %b", l->name, code );
}

#endif /* DEBUG_VM */

/* This file contains our (possibly limited, slightly incompatible) version *
 * of the IRC protocol, as it interacts with our server and client objects. oopen game datapen game data*/

static void irc_server_reply_welcome( struct CLIENT* c, struct SERVER* s ) {

   client_printf(
      c, ":%b 001 %b :Welcome to the Internet Relay Network %b!%b@%b",
      s->self.remote, c->nick, c->nick, c->username, c->remote
   );

   client_printf(
      c, ":%b 002 %b :Your host is ProCIRCd, running version 0.1",
      s->self.remote, c->nick
   );

   client_printf(
      c, ":%b 003 %b :This server was created 01/01/1970",
      s->self.remote, c->nick
   );

   client_printf(
      c, ":%b 004 %b :%b ProCIRCd-0.1 abBcCFiIoqrRswx abehiIklmMnoOPqQrRstvVz",
      s->self.remote, c->nick, s->self.remote
   );

   client_printf(
      c, ":%b 251 %b :There are %d users and 0 services on 1 servers",
      s->self.remote, c->nick, hashmap_count( &(s->clients) )
   );

   c->flags |= CLIENT_FLAGS_HAVE_WELCOME;
}

struct IRC_WHO_REPLY {
   struct CLIENT* c;
   struct CHANNEL* l;
   struct SERVER* s;
};

static void* irc_callback_reply_who( bstring key, void* iter, void* arg ) {
   struct IRC_WHO_REPLY* who = (struct IRC_WHO_REPLY*)arg;
   struct CLIENT* c_iter = (struct CLIENT*)iter;
   client_printf(
      who->c,
      ":%b 352 %b %b %b %b server %b H :0 %b",
      who->s->self.remote,
      who->c->nick,
      who->l->name,
      c_iter->nick,
      c_iter->remote,
      c_iter->username,
      c_iter->realname
   );
   return NULL;
}

#if 0
static void irc_server_reply_motd( struct CLIENT* c, struct SERVER* s ) {
   if( 1 > blength( c->nick ) ) {
      goto cleanup;
   }

   if( c->flags & CLIENT_FLAGS_HAVE_MOTD ) {
      goto cleanup;
   }

   irc_error( ERR_NOMOTD );

   c->flags |= CLIENT_FLAGS_HAVE_MOTD;

cleanup:
   return;
}
#endif

static void irc_server_user(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
   int i,
       consumed = 0;
   char* c_mode = NULL;
   int bstr_result = 0;

   /* Start at 1, skipping the command, itself. */
   for( i = 1 ; args->qty > i ; i++ ) {
      if( 0 == consumed ) {
         /* First arg: User */
         bstr_result = bassign( c->username, args->entry[i] );
         scaffold_check_nonzero( bstr_result );
      } else if( 1 == consumed && scaffold_is_numeric( args->entry[i] ) ) {
         /* Second arg: Mode */
         c_mode = bdata( args->entry[i] );
         scaffold_check_null( c_mode );
         c->mode = atoi( c_mode );
      } else if( 1 == consumed || 2 == consumed ) {
         /* Second or Third arg: * */
         if( 1 == consumed ) {
            consumed++;
         }
      } else if( 3 == consumed && ':' != args->entry[i]->data[0] ) {
         /* Third or Fourth arg: Remote Host */
         bstr_result = bassign( c->remote, args->entry[i] );
         scaffold_check_nonzero( bstr_result );
      } else if( 3 == consumed || 4 == consumed ) {
         /* Fourth or Fifth arg: Real Name */
         bstr_result = bassign( c->realname, args->entry[i] );
         scaffold_check_nonzero( bstr_result );
         if( 3 == consumed ) {
            consumed++; /* Extra bump for missing host. */
         }
      } else if( 4 < consumed ) {
         /* More real name. */
         bstr_result = bconchar( c->realname, ' ' );
         scaffold_check_nonzero( bstr_result );
         bstr_result = bconcat( c->realname, args->entry[i] );
         scaffold_check_nonzero( bstr_result );
      }
      consumed++;
   }

   c->flags |= CLIENT_FLAGS_HAVE_USER;

   if( ( c->flags & CLIENT_FLAGS_HAVE_NICK ) ) {
      /* FIXME: irc_server_reply_welcome( local, remote ); */
   }

cleanup:
   return;
}

static void irc_server_nick(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
   bstring oldnick = NULL;
   bstring newnick = NULL;
   int bstr_result = 0;

   if( 2 >= args->qty ) {
      newnick = args->entry[1];
   }

   server_set_client_nick( s, c, newnick );
   if( SCAFFOLD_ERROR_NULLPO == scaffold_error ) {
      client_printf(
         c, ":%b 431 %b :No nickname given",
         s->self.remote, c->nick
      );
      goto cleanup;
   }

   if( SCAFFOLD_ERROR_NOT_NULLPO == scaffold_error ) {
      client_printf(
         c, ":%b 433 %b :Nickname is already in use",
         s->self.remote, c->nick
      );
      goto cleanup;
   }

   if( 0 < blength( c->nick ) ) {
      oldnick = bstrcpy( c->nick );
      scaffold_check_null( oldnick );
   } else {
      oldnick = bstrcpy( c->username );
      scaffold_check_null( oldnick );
   }

   bstr_result = bassign( c->nick, newnick );
   scaffold_check_nonzero( bstr_result );

   c->flags |= CLIENT_FLAGS_HAVE_NICK;

   /* Don't reply yet if there's been no USER statement yet. */
   if( !(c->flags & CLIENT_FLAGS_HAVE_USER) ) {
      scaffold_print_debug(
         &module, "Client %d quietly changed nick from: %s To: %s\n",
         c->link.socket, bdata( oldnick ), bdata( c->nick )
      );
      goto cleanup;
   }

   if( !(c->flags & CLIENT_FLAGS_HAVE_WELCOME) ) {
      irc_server_reply_welcome( c, s );
   }

   client_printf(
      c, ":%b %b :%b!%b@%b NICK %b",
      s->self.remote, c->nick, oldnick, c->username, c->remote, c->nick
   );

   scaffold_print_debug(
      &module, "Client %d changed nick from: %s To: %s\n",
      c->link.socket, bdata( oldnick ), bdata( c->nick )
   );

cleanup:

   bdestroy( oldnick );

   return;
}

static void irc_server_quit(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
   bstring message;
   bstring space;

   /* TODO: Send everyone parting message. */
   space = bfromcstr( " " );
   message = bjoin( args, space );

   client_printf(
      c, "ERROR :Closing Link: %b (Client Quit)",
      c->nick
   );

   server_drop_client( s, c->nick );

   bdestroy( message );
   bdestroy( space );
}

static void irc_server_ison(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
   struct VECTOR* ison = NULL;
   bstring response = NULL;
   struct CLIENT* c_iter = NULL;
   int i;
   int bstr_result;

   response = bfromcstralloc( IRC_STANZA_ALLOC, "" );
   scaffold_check_null( response );

   /* TODO: Root the command stuff out of args. */
   ison = hashmap_iterate_v( &(s->clients), callback_search_clients_l, (void*)args );
   scaffold_check_null( ison );
   vector_lock( ison, TRUE );
   for( i = 0 ; vector_count( ison ) > i ; i++ ) {
      c_iter = (struct CLIENT*)vector_get( ison, i );
      /* 1 for the main list + 1 for the vector. */
      assert( 2 <= c_iter->link.refcount.count );
      bstr_result = bconcat( response, c_iter->nick );
      scaffold_check_nonzero( bstr_result );
      bstr_result = bconchar( response, ' ' );
      scaffold_check_nonzero( bstr_result );
   }
   vector_lock( ison, FALSE );
   scaffold_check_null( response );

   client_printf( c, ":%b 303 %b :%b", s->self.remote, c->nick, response );

cleanup:
   if( NULL != ison ) {
      vector_remove_cb( ison, callback_free_clients, NULL );
      vector_free( ison );
      free( ison );
   }
   bdestroy( response );
   return;
}

static void irc_server_join(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
   struct CHANNEL* l = NULL;
   bstring namehunt = NULL;
   int8_t bstr_result = 0;
   bstring names = NULL;
   struct bstrList* cat_names = NULL;

   if( 2 > args->qty ) {
      client_printf(
         c, ":%b 461 %b %b :Not enough parameters",
         s->self.remote, c->username, args->entry[0]
      );
      goto cleanup;
   }

   /* Add the channel to the server if it does not exist. */
   namehunt = bstrcpy( args->entry[1] );
   bstr_result = btrimws( namehunt );
   scaffold_check_nonzero( bstr_result );

   if( TRUE != scaffold_string_is_printable( namehunt ) ) {
      client_printf(
         c, ":%b 403 %b %b :No such channel",
         s->self.remote, c->username, namehunt
      );
      goto cleanup;
   }

   l = server_add_channel( s, namehunt, c );
   scaffold_check_null( l );

   assert( NULL != c );
   assert( NULL != s );
   assert( NULL != c->nick );
   assert( NULL != c->username );
   assert( NULL != c->remote );
   assert( NULL != l->name );

   /* Announce the new join. */
   server_channel_printf(
      s, l, c, ":%b!%b@%b JOIN %b", c->nick, c->username, c->remote, l->name
   );

   /* Now tell the joining client. */
   client_printf(
      c, ":%b!%b@%b JOIN %b",
      c->nick, c->username, c->remote, l->name
   );

   cat_names = bstrListCreate();
   scaffold_check_null( cat_names );
   hashmap_iterate( &(l->clients), callback_concat_clients, cat_names );
   names = bjoinblk( cat_names, " ", 1 );

   client_printf(
      c, ":%b 332 %b %b :%b",
      s->self.remote, c->nick, l->name, l->topic
   );

   client_printf(
      c, ":%b 353 %b = %b :%b",
      s->self.remote, c->nick, l->name, names
   );

   client_printf(
      c, ":%b 366 %b %b :End of NAMES list",
      s->self.remote, c->nick, l->name
   );

   scaffold_assert( hashmap_count( &(c->channels) ) > 0 );
   scaffold_assert( hashmap_count( &(s->self.channels) ) > 0 );

cleanup:
   if( NULL != cat_names ) {
      bstrListDestroy( cat_names );
   }
   bdestroy( names );
   bdestroy( namehunt );
   return;
}

static void irc_server_part(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
}

static void irc_server_privmsg(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
   struct CLIENT* c_dest = NULL;
   struct CHANNEL* l_dest = NULL;
   bstring msg = NULL;
   struct bstrList* msg_list = NULL;
   int i;

   msg_list = bstrListCreate();
   bstrListAlloc( msg_list, args->qty - 1 );
   for( i = 1 ; args->qty > i ; i++ ) {
      msg_list->entry[i - 1] = bstrcpy( args->entry[i] );
   }

   scaffold_print_debug( &module, "%s\n", msg_list->entry[0]->data );

   msg = bjoin( msg_list, &scaffold_space_string );
   btrimws( msg );
   char* cc = line->data;

   c_dest = server_get_client( s, args->entry[1] );
   if( NULL != c_dest ) {
      client_printf(
         c_dest, ":%b!%b@%b %b", c->nick, c->username, c->remote, cc
      );
      goto cleanup;
   }

   /* Maybe it's for a channel, instead? */
   l_dest = client_get_channel_by_name( &(s->self), args->entry[1] );
   if( NULL != l_dest ) {
      server_channel_printf(
         s, l_dest, c, ":%b!%b@%b %b", c->nick, c->username, c->remote, cc
      );
      goto cleanup;
   }

   /* TODO: Handle error? */

cleanup:
   bdestroy( msg );
   bstrListDestroy( msg_list );
   return;
}

static void irc_server_who(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
   struct CHANNEL* l = NULL;
   struct IRC_WHO_REPLY who;

   irc_detect_malformed( 2, "WHO" );

   /* TODO: Handle non-channels. */
   if( '#' != args->entry[1]->data[0] ) {
      scaffold_print_error( &module, "Non-channel WHO not yet implemented.\n" );
      goto cleanup;
   }

   l = client_get_channel_by_name( &(s->self), args->entry[1] );
   scaffold_check_null( l );

   who.c = c;
   who.l = l;
   who.s = s;
   hashmap_iterate( &(l->clients), irc_callback_reply_who, &who );
   client_printf(
      c, ":server 315 %b :End of /WHO list.",
      c->nick, l->name
   );

   hashmap_iterate( &(l->clients), callback_send_mobs_to_channel, l );

cleanup:
   return;
}

static void irc_server_ping(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {

   if( 2 > args->qty ) {
      goto cleanup;
   }

   client_printf(
      c, ":%b PONG %b :%b", s->self.remote, s->self.remote, c->remote
   );

cleanup:
   return;
}

#ifdef USE_CHUNKS

static void irc_server_gamerequestfile(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
   char* type_c;
   CHUNKER_DATA_TYPE type;
   int bstr_result;
   bstring file_path_found = NULL;

   irc_detect_malformed( 3, "GRF" );

   type_c = bdata( args->entry[1] );
   scaffold_check_null( type_c );
   type = atoi( type_c );

   file_path_found = server_file_search( args->entry[2] );
   scaffold_check_null( file_path_found );

   client_send_file( c, type, &str_server_data_path, file_path_found );

cleanup:
   bdestroy( file_path_found );
   return;
}

#endif /* USE_CHUNKS */

static void irc_server_gameupdate(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
   char* serial_c,
      * update_c;
   SCAFFOLD_SIZE serial;
   struct MOBILE_UPDATE_PACKET update;

   update.l = client_get_channel_by_name( c, args->entry[1] );
   scaffold_check_null( update.l );

   serial_c = bdata( args->entry[2] );
   scaffold_check_null( serial_c );
   serial = atoi( serial_c );

   update.o = (struct MOBILE*)vector_get( &(update.l->mobiles), serial );
   scaffold_check_null( update.o );

   update_c = bdata( args->entry[3] );
   scaffold_check_null( update_c );
   update.update = (MOBILE_UPDATE)atoi( update_c );

   if( c == update.o->owner ) {
      update.update = mobile_apply_update( &update, TRUE );
   } else {
      scaffold_print_error(
         &module,
         "Client %s attempted to modify mobile %d to %d without permission.\n",
         bdata( c->nick ), update.o->serial, update.update
      );
      /* Cancel the update. */
      update.update = MOBILE_UPDATE_NONE;
   }

   hashmap_iterate( &(update.l->clients), callback_send_updates_to_client, &update );

cleanup:
   return;
}

#ifdef DEBUG_VM

static void irc_server_debugvm(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
   bstring code = NULL;
   bstring lname = args->entry[1];
   int i;
   struct CHANNEL* l = NULL;
   int bstr_ret;

   code = bfromcstr( "" );

   for( i = 2 /* Skip command and channel. */ ; args->qty > i ; i++ ) {
      bstr_ret = bconchar( code, ' ' );
      scaffold_check_nonzero( bstr_ret );
      bstr_ret = bconcat( code, args->entry[i] );
      scaffold_check_nonzero( bstr_ret );
   }

   l = server_get_channel_by_name( s, lname );
   scaffold_check_null( l );

   channel_vm_start( l, code );

cleanup:
   bdestroy( code );
   return;
}

#endif /* DEBUG_VM */

static void irc_client_gu(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
   char* serial_c,
      * update_c;
   SCAFFOLD_SIZE serial;
   struct MOBILE_UPDATE_PACKET update;

   update.l = client_get_channel_by_name( c, args->entry[1] );
   scaffold_check_null( update.l );

   serial_c = bdata( args->entry[2] );
   scaffold_check_null( serial_c );
   serial = atoi( serial_c );

   update.o = (struct MOBILE*)vector_get( &(update.l->mobiles), serial );
   scaffold_check_null( update.o );

   update_c = bdata( args->entry[3] );
   scaffold_check_null( update_c );
   update.update = (MOBILE_UPDATE)atoi( update_c );

   /* The client always trusts the server. */
   update.update = mobile_apply_update( &update, FALSE );

cleanup:
   return;
}

static void irc_client_join(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
   struct CHANNEL* l = NULL;
   bstring l_name = NULL;
   bstring l_filename = NULL;
   int bstr_res;

   scaffold_assert_client();

   scaffold_check_bounds( 3, args->qty );
   l_name = args->entry[3];

   /* Get the channel, or create it if it does not exist. */
   l = client_get_channel_by_name( c, l_name );
   if( NULL == l ) {
      channel_new( l, l_name, FALSE );
      client_add_channel( c, l );
      channel_add_client( l, c, FALSE );
      scaffold_print_info(
         &module, "Client created local channel mirror: %s\n", bdata( l_name )
      );
   }

   scaffold_print_info(
      &module, "Client joined channel: %s\n", bdata( l_name )
   );

   scaffold_assert( hashmap_count( &(c->channels) ) > 0 );

   /* Strip off the #. */
   l_filename = bmidstr( l->name, 1, blength( l->name ) - 1 );
   bstr_res = bcatcstr( l_filename, ".tmx" );
   scaffold_check_nonzero( bstr_res );
   scaffold_check_null( l_filename );

   client_request_file( c, CHUNKER_DATA_TYPE_TILEMAP, l_filename );

cleanup:
   bdestroy( l_filename );
   return;
}

static const struct tagbstring str_closing = bsStatic( ":Closing" );

static void irc_client_error(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
   if(
      2 <= args->qty &&
      0 == bstrcmp( &str_closing, args->entry[1] )
   ) {
      client_stop( c );
   }
}

#ifdef USE_CHUNKS

static void irc_client_gamedatablock(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
   const char* progress_c,
      * total_c,
      * length_c,
      * filename_c,
      * type_c;
   struct CHUNKER_PROGRESS progress;

   irc_detect_malformed( 11, "GDB" );

   scaffold_check_null( args->entry[5] );
   type_c = bdata( args->entry[5] );
   scaffold_check_null( type_c );

   scaffold_check_null( args->entry[6] );
   progress_c = bdata( args->entry[6] );
   scaffold_check_null( progress_c );

   scaffold_check_null( args->entry[7] );
   length_c = bdata( args->entry[7] );
   scaffold_check_null( length_c );

   scaffold_check_null( args->entry[8] );
   total_c = bdata( args->entry[8] );
   scaffold_check_null( total_c );

   progress.filename = args->entry[4];
   filename_c = bdata( progress.filename );
   scaffold_check_null( filename_c );

   progress.current = atoi( progress_c );
   progress.chunk_size = atoi( length_c );
   progress.total = atoi( total_c );
   progress.type = (CHUNKER_DATA_TYPE)atoi( type_c );

   progress.data = args->entry[10];
   scaffold_check_null( progress.data );

   client_process_chunk( c, &progress );

cleanup:
   return;
}

static void irc_server_gamedataabort(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
   irc_detect_malformed( 2, "GDA" );

   scaffold_print_info(
      &module,
      "Server: Terminating transfer of %s at request of client: %d\n",
      bdata( args->entry[1] ), c->link.socket
   );

   hashmap_remove_cb( &(c->chunkers), callback_free_chunkers, args->entry[1] );

cleanup:
   return;
}

#endif /* USE_CHUNKS */

static void irc_server_gamenewsprite(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
}

static void irc_client_gamenewsprite(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
}

static void irc_server_mob(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
   /* TODO: If the serial matches c's mob, update server. Otherwise send requested mob's details. */
}

static void irc_client_mob(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
   char* serial_c = NULL,
      * x_c = NULL,
      * y_c = NULL;
   uint8_t serial = 0;
   bstring def_filename = NULL,
      mob_id = NULL,
      nick = NULL;
   struct CHANNEL* l = NULL;
   SCAFFOLD_SIZE x = 0,
      y = 0;

   irc_detect_malformed( 8, "MOB" );

   l = client_get_channel_by_name( c, args->entry[1] );
   scaffold_check_null( l );

   scaffold_check_null( args->entry[2] );
   serial_c = bdata( args->entry[2] );
   scaffold_check_null( serial_c );
   serial = atoi( serial_c );

   mob_id = args->entry[3];
   scaffold_check_null( mob_id );

   def_filename = args->entry[4];
   scaffold_check_null( def_filename );

   nick = args->entry[5];
   scaffold_check_null( nick );

   scaffold_check_null( args->entry[6] );
   x_c = bdata( args->entry[6] );
   scaffold_check_null( x_c );
   x = atoi( x_c );

   scaffold_check_null( args->entry[7] );
   y_c = bdata( args->entry[7] );
   scaffold_check_null( y_c );
   y = atoi( y_c );

   channel_set_mobile( l, serial, mob_id, def_filename, nick, x, y );
   scaffold_assert( 0 == scaffold_error );

   scaffold_print_debug(
      &module, "Client: Local instance of mobile updated: %s (%d)\n",
      bdata( nick ), serial
   );

cleanup:
   return;
}

static void irc_client_privmsg(
   struct CLIENT* c, struct SERVER* s, const struct bstrList* args, bstring line
) {
   scaffold_print_debug( &module, "1: %s 2: %s 3: %s 4: %s",
                        args->entry[0]->data, args->entry[1]->data,
                        args->entry[2]->data, args->entry[2]->data );
}

IRC_COMMAND_TABLE_START( server ) = {
IRC_COMMAND_ROW( "USER", irc_server_user ),
IRC_COMMAND_ROW( "NICK", irc_server_nick ),
IRC_COMMAND_ROW( "QUIT", irc_server_quit ),
IRC_COMMAND_ROW( "ISON", irc_server_ison ),
IRC_COMMAND_ROW( "JOIN", irc_server_join ),
IRC_COMMAND_ROW( "PART", irc_server_part ),
IRC_COMMAND_ROW( "PRIVMSG", irc_server_privmsg ),
IRC_COMMAND_ROW( "WHO", irc_server_who ),
IRC_COMMAND_ROW( "PING", irc_server_ping ),
IRC_COMMAND_ROW( "GU", irc_server_gameupdate ),
#ifdef USE_CHUNKS
IRC_COMMAND_ROW( "GRF", irc_server_gamerequestfile ),
IRC_COMMAND_ROW( "GDA", irc_server_gamedataabort ),
#endif /* USE_CHUNKS */
IRC_COMMAND_ROW( "GNS", irc_server_gamenewsprite ),
IRC_COMMAND_ROW( "NOB", irc_server_mob ),
#ifdef DEBUG_VM
IRC_COMMAND_ROW( "DEBUGVM", irc_server_debugvm ),
#endif /* DEBUGVM */
IRC_COMMAND_TABLE_END() };

IRC_COMMAND_TABLE_START( client ) = {
IRC_COMMAND_ROW( "GU", irc_client_gu  ),
IRC_COMMAND_ROW( "366", irc_client_join ),
IRC_COMMAND_ROW( "ERROR", irc_client_error  ),
#ifdef USE_CHUNKS
IRC_COMMAND_ROW( "GDB", irc_client_gamedatablock ),
#endif /* USE_CHUNKS */
IRC_COMMAND_ROW( "GNS", irc_client_gamenewsprite ),
IRC_COMMAND_ROW( "MOB", irc_client_mob ),
IRC_COMMAND_ROW( "PRIVMSG", irc_client_privmsg ),
IRC_COMMAND_TABLE_END() };

static void irc_command_cleanup( const struct REF* ref ) {
   IRC_COMMAND* cmd = scaffold_container_of( ref, IRC_COMMAND, refcount );
   int i;

   /* Don't try to free the string or callback. */
   /* client_free( cmd->client ); */
   cmd->client = NULL;
   /* server_free( cmd->server ); */
   cmd->server = NULL;

   for( i = 0 ; cmd->args->qty > i ; i++ ) {
      bwriteallow( *(cmd->args->entry[i]) );
   }
   bstrListDestroy( (struct bstrList*)cmd->args );
   cmd->args = NULL;
}

void irc_command_free( IRC_COMMAND* cmd ) {
   refcount_dec( cmd, "command" );
}

IRC_COMMAND* irc_dispatch(
   const IRC_COMMAND* table, struct SERVER* s, struct CLIENT* c, const_bstring line
) {
   struct bstrList* args = NULL;
   const IRC_COMMAND* command = NULL;
   SCAFFOLD_SIZE i;
   bstring cmd_test = NULL; /* Don't free this. */
   IRC_COMMAND* out = NULL;

   args = bsplit( line, ' ' );
   scaffold_check_null( args );

   if( table == proto_table_server ) {
      scaffold_assert_server();
   } else if( table == proto_table_client ) {
      scaffold_assert_client();
   }

   for( i = 0 ; args->qty > i ; i++ ) {
      bwriteprotect( *(args->entry[i]) );
   }

   for( i = 0 ; i < args->qty && IRC_LINE_CMD_SEARCH_RANGE > i ; i++ ) {
      cmd_test = args->entry[i];
      for(
         command = &(table[0]);
         NULL != command->callback;
         command++
      ) {
         if( 0 == bstrncmp(
            cmd_test, &(command->command), blength( &(command->command) )
         ) ) {
#ifdef DEBUG
            if(
               0 != bstrncmp( cmd_test, &(proto_table_client[3].command), 3 ) &&
               0 != bstrncmp( cmd_test, &(proto_table_client[0].command), 2 )
            ) {
               if( table == proto_table_server ) {
                  scaffold_print_debug(
                     &module, "Server Parse: %s\n", bdata( line ) );
               } else {
                  scaffold_print_debug(
                     &module, "Client Parse: %s\n", bdata( line ) );
               }
            }
#endif /* DEBUG */

            out = (IRC_COMMAND*)calloc( 1, sizeof( IRC_COMMAND ) );
            scaffold_check_null( out );
            memcpy( out, command, sizeof( IRC_COMMAND ) );
            if( NULL != s ) {
               out->server = s;
               /* refcount_inc( &(s->self.link), "server" ); */
            }
            out->client = c;
            /* refcount_inc( &(c->link), "client" ); */
            out->args = args;
            out->line = bstrcpy( line );
            ref_init( &(out->refcount), irc_command_cleanup );

            scaffold_assert( 0 == bstrcmp( &(out->command), &(command->command) ) );

            /* Found a command, so short-circuit. */
            goto cleanup;
         }
      }
   }

   if( table == proto_table_server ) {
      scaffold_assert_server();
      scaffold_print_error(
         &module, "Server: Parser unable to interpret: %s\n", bdata( line ) );
   } else if( table == proto_table_client ) {
      scaffold_assert_client();
      scaffold_print_error(
         &module, "Client: Parser unable to interpret: %s\n", bdata( line ) );
   }


cleanup:
   if( NULL == out ) {
      for( i = 0 ; args->qty > i ; i++ ) {
         bwriteallow( *(args->entry[i]) );
      }
      bstrListDestroy( args );
   }
   return out;
}
