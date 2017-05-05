
#define PROTO_C
#include "../proto.h"

#include "../callback.h"
#include "../chunker.h"
#include "../datafile.h"
#include "../backlog.h"

static struct TILEMAP_ITEM_CACHE* last_item_cache = NULL;

typedef enum _IRC_ERROR {
   ERR_NONICKNAMEGIVEN = 431,
   ERR_NICKNAMEINUSE = 433,
   ERR_NOMOTD = 422
} IRC_ERROR;

const struct tagbstring str_gdb = bsStatic( "GDB" );
const struct tagbstring str_gu = bsStatic( "GU" );
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

#define irc_detect_malformed( args_qty, expression, line_var ) \
   if( args_qty != vector_count( args ) ) { \
      scaffold_print_error( \
         &module, \
         "IRC: Malformed " expression " expression received: %b\n", \
         line_var \
      ); \
      scaffold_error = SCAFFOLD_ERROR_MISC; \
      goto cleanup; \
   }

void proto_register( struct CLIENT* c ) {
   client_printf( c, "NICK %b", c->nick );
   client_printf( c, "USER %b", c->realname );
}

#ifdef USE_CHUNKS

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
 * \param c          Local client object.
 * \param filename   Name/path of the file (relative to the server root).
 * \param type       Type of file requested. Used in return processing.
 */
void proto_request_file( struct CLIENT* c, const bstring filename, DATAFILE_TYPE type ) {
   scaffold_assert_client();
   scaffold_assert( 0 < blength( filename ) );
   /*
   scaffold_print_debug(
      &module, "Client: Requesting %b file: %s\n",
      chunker_type_names[type].data, bdata( filename )
   );
   */
   client_printf( c, "GRF %d %b", type, filename );
}

/** \brief Request spritesheet file name for the resource with the provided
 *         name.
 * \param c    Local client object.
 * \param type Resource type to request (e.g. CDT_ITEM_CATALOG_SPRITES)
 * \param name Name of the object (catalog/mobile type/etc) requesting for.
 */
void proto_client_request_spritesheet(
   struct CLIENT* c, bstring name, DATAFILE_TYPE type
) {
   scaffold_assert_client();
   scaffold_assert( 0 < blength( name ) );
   scaffold_print_debug(
      &module, "Client: Requesting %b: %b\n",
      &(chunker_type_names[type]), name
   );
   client_printf( c, "CREQ %d %b", type, name );
}

#endif /* USE_CHUNKS */

void proto_server_name_spritesheet(
   struct CLIENT* c, bstring filename, DATAFILE_TYPE type
) {
   scaffold_assert_client();
   scaffold_assert( 0 < blength( filename ) );
#ifdef USE_CHUNKS
   scaffold_print_debug(
      &module, "Server: Naming %b: %s\n",
      chunker_type_names[type].data, filename
   );
#endif /* USE_CHUNKS */
   client_printf( c, "CRES %d %b", type, filename );
}

void proto_send_mob( struct CLIENT* c, struct MOBILE* o ) {
   bstring owner_nick = NULL;

   scaffold_assert_server();
   scaffold_assert( NULL != o );
   scaffold_assert( NULL != o->mob_id );
   scaffold_assert( NULL != o->def_filename );
   scaffold_assert( NULL != o->channel );
   scaffold_assert( NULL != o->channel->name );

   if( NULL != o->owner ) {
      scaffold_assert( NULL != o->owner->nick );
      owner_nick = o->owner->nick;
   } else {
      owner_nick = &scaffold_null;
   }

   client_printf(
      c, "MOB %b %d %b %b %b %d %d",
      o->channel->name, o->serial, o->mob_id, o->def_filename,
      owner_nick, o->x, o->y
   );
}

static void* proto_send_item_cb(
   struct CONTAINER_IDX* idx, void* iter, void* arg
) {
   struct ITEM* e = (struct ITEM*)iter;
   struct CLIENT* c = (struct CLIENT*)arg;

   scaffold_assert_server();

   scaffold_assert( 0 < blength( e->catalog_name ) );

   client_printf(
      c, "ITEM %d %b %d %b %d",
      e->serial, e->display_name, e->count, e->catalog_name, e->sprite_id
   );

   return NULL;
}

void proto_send_container( struct CLIENT* c, struct ITEM* e ) {
   if( FALSE != item_is_container( e ) ) {

      client_printf(
         c, "CONTAINER_START %d",
         e->serial
      );

      vector_iterate( e->content.container, proto_send_item_cb, c );

      client_printf( c, "CONTAINER_END" );
   }
}

void proto_send_tile_cache(
   struct CLIENT* c, struct TILEMAP_ITEM_CACHE* cache
) {
   struct CHANNEL* l = NULL;

   scaffold_assert_server();

   l = scaffold_container_of( cache->tilemap, struct CHANNEL, tilemap );
   scaffold_check_equal( l->sentinal, CHANNEL_SENTINAL );

   client_printf(
      c, "IC_S %b %d %d",
      l->name, cache->position.x, cache->position.y
   );

   vector_iterate( &(cache->items), proto_send_item_cb, c );

   client_printf( c, "IC_E" );

cleanup:
   return;
}

static void* proto_send_tile_cache_channel_cb(
   struct CONTAINER_IDX* idx, void* iter, void* arg
) {
   struct CLIENT* c = (struct CLIENT*)iter;
   struct TILEMAP_ITEM_CACHE* cache = (struct TILEMAP_ITEM_CACHE*)arg;

   proto_send_tile_cache( c, cache );
}

void proto_send_tile_cache_channel(
   struct CHANNEL* l, struct TILEMAP_ITEM_CACHE* cache
) {
   hashmap_iterate(
      &(l->clients),
      proto_send_tile_cache_channel_cb,
      cache
   );
}

void proto_client_send_update( struct CLIENT* c, struct MOBILE_UPDATE_PACKET* update ) {
   SCAFFOLD_SIZE serial = 0;
   scaffold_assert_client();
   if( NULL != update->target ) {
      serial = update->target->serial;
   }
   client_printf(
      c, "GU %b %d %d %d %d %d",
      update->l->name, update->o->serial, update->update, update->x, update->y, serial
   );
}

void proto_server_send_update( struct CLIENT* c, struct MOBILE_UPDATE_PACKET* update ) {
   SCAFFOLD_SIZE serial = 0;
   scaffold_assert_server();
   if( NULL != update->target ) {
      serial = update->target->serial;
   }
   client_printf(
      c, "GU %b %d %d %d %d %d",
      update->l->name, update->o->serial, update->update, update->x, update->y, serial
   );
}

void proto_send_msg_channel( struct CLIENT* c, struct CHANNEL* ld, bstring msg ) {
   scaffold_assert_client();
   proto_send_msg( c, ld->name, msg );
}

void proto_send_msg_client( struct CLIENT* c, struct CLIENT* cd, bstring msg ) {
   scaffold_assert_client();
   proto_send_msg( c, cd->nick, msg );
}

void proto_send_msg( struct CLIENT* c, bstring dest, bstring msg ) {
   scaffold_assert_client();
   client_printf( c, "PRIVMSG %b :%b", dest, msg );
}

void proto_server_send_msg_channel(
   struct SERVER* s, struct CHANNEL* l, const bstring nick, const bstring msg
) {
   scaffold_assert_server();
   scaffold_check_null_msg( l, "Invalid channel to send message." );
   server_channel_printf(
      s, l, NULL, ":%b!%b@%b PRIVMSG %b :%b", nick, nick, s->self.remote, l->name, msg
   );
cleanup:
   return;
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

static void* irc_callback_reply_who( struct CONTAINER_IDX* idx, void* iter, void* arg ) {
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
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   SCAFFOLD_SIZE i,
       consumed = 0;
   int bstr_result = 0;

   /* Start at 1, skipping the command, itself. */
   for( i = 1 ; vector_count( args ) > i ; i++ ) {
      if( 0 == consumed ) {
         /* First arg: User */
         bstr_result = bassign( c->username, (bstring)vector_get( args, i ) );
         scaffold_check_nonzero( bstr_result );
      } else if( 1 == consumed && scaffold_is_numeric( (bstring)vector_get( args, i ) ) ) {
         /* Second arg: Mode */
         c->mode = bgtoi( (bstring)vector_get( args, i ) );
      } else if( 1 == consumed || 2 == consumed ) {
         /* Second or Third arg: * */
         if( 1 == consumed ) {
            consumed++;
         }
      } else if( 3 == consumed && ':' != ((bstring)vector_get( args, i ))->data[0] ) {
         /* Third or Fourth arg: Remote Host */
         bstr_result = bassign( c->remote, (bstring)vector_get( args, i ) );
         scaffold_check_nonzero( bstr_result );
      } else if( 3 == consumed || 4 == consumed ) {
         /* Fourth or Fifth arg: Real Name */
         bstr_result = bassign( c->realname, (bstring)vector_get( args, i ) );
         scaffold_check_nonzero( bstr_result );
         if( 3 == consumed ) {
            consumed++; /* Extra bump for missing host. */
         }
      } else if( 4 < consumed ) {
         /* More real name. */
         bstr_result = bconchar( c->realname, ' ' );
         scaffold_check_nonzero( bstr_result );
         bstr_result = bconcat( c->realname, (bstring)vector_get( args, i ) );
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
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   bstring oldnick = NULL;
   bstring newnick = NULL;
   int bstr_result = 0;

   if( 2 >= vector_count( args ) ) {
      newnick = (bstring)vector_get( args, 1 );
   }

   /* Disallow system nick "(null)". */
   if( 0 == bstrcmp( &scaffold_null, newnick ) ) {
      client_printf(
         c, ":%b 431 %b :No nickname given",
         s->self.remote, c->nick
      );
      goto cleanup;
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
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   bstring message;

   /* TODO: Send everyone parting message. */
   message = bfromcstr( "" );
   vector_iterate( args, callback_concat_strings, message );

   client_printf(
      c, "ERROR :Closing Link: %b (Client Quit)",
      c->nick
   );

   server_drop_client( s, c->nick );

   bdestroy( message );
}

static void irc_server_ison(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   struct VECTOR* ison = NULL;
   bstring response = NULL;
   struct CLIENT* c_iter = NULL;
   SCAFFOLD_SIZE i;
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
      vector_cleanup( ison );
      mem_free( ison );
   }
   bdestroy( response );
   return;
}

static void irc_server_join(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   struct CHANNEL* l = NULL;
   bstring namehunt = NULL;
   int8_t bstr_result = 0;
   bstring names = NULL;
   struct VECTOR* cat_names = NULL;

   if( 2 > vector_count( args ) ) {
      client_printf(
         c, ":%b 461 %b %b :Not enough parameters",
         s->self.remote, c->username, (bstring)vector_get( args, 0 )
      );
      goto cleanup;
   }

   /* Add the channel to the server if it does not exist. */
   namehunt = bstrcpy( (bstring)vector_get( args, 1 ) );
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

   vector_new( cat_names );
   hashmap_iterate( &(l->clients), callback_concat_clients, cat_names );
   names = bfromcstr( "" );
   scaffold_check_null( names );
   vector_iterate( cat_names, callback_concat_strings, names );

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
      vector_remove_cb( cat_names, callback_free_strings, NULL );
      vector_free( &cat_names );
   }
   bdestroy( names );
   bdestroy( namehunt );
   return;
}

static void irc_server_part(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
}

static void irc_server_privmsg(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   struct CLIENT* c_dest = NULL;
   struct CHANNEL* l_dest = NULL;
   bstring msg = NULL;
   struct VECTOR* msg_list = NULL;
   SCAFFOLD_SIZE i;

   vector_new( msg_list );
   for( i = 1 ; vector_count( args ) > i ; i++ ) {
      vector_add( msg_list, bstrcpy( (bstring)vector_get( args, i ) ) );
   }

   scaffold_print_debug( &module, "%s\n", ((bstring)vector_get( msg_list, 0 ))->data );

   msg = bfromcstr( "" );
   scaffold_check_null( msg );
   vector_iterate( msg_list, callback_concat_strings, msg );

   c_dest = server_get_client( s, (bstring)vector_get( args, 1 ) );
   if( NULL != c_dest ) {
      client_printf(
         c_dest, ":%b!%b@%b %b", c->nick, c->username, c->remote, line
      );
      goto cleanup;
   }

   /* Maybe it's for a channel, instead? */
   l_dest = client_get_channel_by_name( &(s->self), (bstring)vector_get( args, 1 ) );
   if( NULL != l_dest ) {
      server_channel_printf(
         s, l_dest, c, ":%b!%b@%b %b", c->nick, c->username, c->remote, line
      );
      goto cleanup;
   }

   /* TODO: Handle error? */

cleanup:
   bdestroy( msg );
   vector_free( &msg_list );
   return;
}

static void irc_server_who(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   struct CHANNEL* l = NULL;
   struct IRC_WHO_REPLY who;

   irc_detect_malformed( 2, "WHO", line );

   /* TODO: Handle non-channels. */
   if( '#' != ((bstring)vector_get( args, 1 ))->data[0] ) {
      scaffold_print_error( &module, "Non-channel WHO not yet implemented.\n" );
      goto cleanup;
   }

   l = client_get_channel_by_name( &(s->self), (bstring)vector_get( args, 1 ) );
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
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {

   if( 2 > vector_count( args ) ) {
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
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   DATAFILE_TYPE type;
   int bstr_result;
   bstring file_path_found = NULL;

   irc_detect_malformed( 3, "GRF", line );

   type = (DATAFILE_TYPE)bgtoi( (bstring)vector_get( args, 1 ) );

   file_path_found = files_search( (bstring)vector_get( args, 2 ) );
   scaffold_check_null( file_path_found );

   client_send_file( c, type, files_root( NULL ), file_path_found );

cleanup:
   bdestroy( file_path_found );
   return;
}

#endif /* USE_CHUNKS */

static void irc_server_gameupdate(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   char* serial_c,
      * target_c,
      * update_c,
      * x_c,
      * y_c;
   SCAFFOLD_SIZE serial,
      target_serial;
   struct MOBILE_UPDATE_PACKET update;

   irc_detect_malformed( 7, "GU", line )

   update.l = client_get_channel_by_name( c, (bstring)vector_get( args, 1 ) );
   scaffold_check_null( update.l );

   serial = bgtoi( (bstring)vector_get( args, 2 ) );
   update.update = (MOBILE_UPDATE)bgtoi( (bstring)vector_get( args, 3 ) );
   update.x = bgtoi( (bstring)vector_get( args, 4 ) );
   update.y = bgtoi( (bstring)vector_get( args, 5 ) );
   target_serial = bgtoi( (bstring)vector_get( args, 6 ) );

   update.o = (struct MOBILE*)vector_get( &(update.l->mobiles), serial );
   scaffold_check_null( update.o );

   update.target =
      (struct MOBILE*)vector_get( &(update.l->mobiles), target_serial );
   /* No NULL check. If it's NULL, it's NULL. */

   if( c == update.o->owner ) {
      update.update = mobile_apply_update( &update, TRUE );
   } else {
      scaffold_print_error(
         &module,
         "Client %s attempted to modify mobile %d to %d without permission.\n",
         bdata( c->nick ), update.o->serial, update.update
      );
   }

cleanup:
   return;
}

static void irc_server_resource_request(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   struct CHUNKER_PROGRESS progress;
   DATAFILE_TYPE type;
   bstring object_name = NULL;
   struct ITEM_SPRITESHEET* catalog = NULL;

   irc_detect_malformed( 3, "CREQ", line );

   type = (DATAFILE_TYPE)bgtoi( (bstring)vector_get( args, 1 ) );

   object_name = (bstring)vector_get( args, 2 );

   switch( type ) {
   case DATAFILE_TYPE_ITEM_CATALOG:
      catalog = hashmap_get( &(s->self.item_catalogs), object_name );
      scaffold_check_null_msg( catalog, "Item catalog not found." );
      proto_server_name_spritesheet( c, catalog->sprites_filename, type );
      break;
   }

cleanup:
   return;
}

static void irc_client_resource_response(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   struct CHUNKER_PROGRESS progress;
   DATAFILE_TYPE type;
   bstring filename = NULL;
   struct ITEM_SPRITESHEET* catalog = NULL;

   irc_detect_malformed( 3, "CRES", line );

   type = (DATAFILE_TYPE)bgtoi( (bstring)vector_get( args, 1 ) );

   filename = (bstring)vector_get( args, 2 );

   client_request_file( c, type, filename );

cleanup:
   return;
}

#ifdef DEBUG_VM

static void irc_server_debugvm(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   bstring code = NULL;
   bstring lname = NULL;
   SCAFFOLD_SIZE i;
   struct CHANNEL* l = NULL;
   int bstr_ret;

   lname = (bstring)vector_get( args, 1 );
   code = bfromcstr( "" );

   for( i = 2 /* Skip command and channel. */ ; vector_count( args ) > i ; i++ ) {
      bstr_ret = bconchar( code, ' ' );
      scaffold_check_nonzero( bstr_ret );
      bstr_ret = bconcat( code, (bstring)vector_get( args, i ) );
      scaffold_check_nonzero( bstr_ret );
   }

   l = server_get_channel_by_name( s, lname );
   scaffold_check_null( l );

   // FIXME: channel_vm_start( l, code );

cleanup:
   bdestroy( code );
   return;
}

#endif /* DEBUG_VM */

#ifdef ENABLE_LOCAL_CLIENT

static void irc_client_gu(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   SCAFFOLD_SIZE serial,
      target_serial;
   struct MOBILE_UPDATE_PACKET update;

   update.l = client_get_channel_by_name( c, (bstring)vector_get( args, 1 ) );
   scaffold_check_null( update.l );

   serial = bgtoi( (bstring)vector_get( args, 2 ) );

   update.o = (struct MOBILE*)vector_get( &(update.l->mobiles), serial );
   scaffold_check_null( update.o );

   update.update = (MOBILE_UPDATE)bgtoi( (bstring)vector_get( args, 3 ) );
   update.x = bgtoi( (bstring)vector_get( args, 4 ) );
   update.y = bgtoi( (bstring)vector_get( args, 5 ) );
   target_serial = bgtoi( (bstring)vector_get( args, 6 ) );

   update.target =
      (struct MOBILE*)vector_get( &(update.l->mobiles), target_serial );
   /* No NULL check. If it's NULL, it's NULL. */

   /* The client always trusts the server. */
   update.update = mobile_apply_update( &update, FALSE );

cleanup:
   return;
}

static void irc_client_join(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   struct CHANNEL* l = NULL;
   bstring l_name = NULL;
   bstring l_filename = NULL;
   int bstr_res;

   scaffold_assert_client();

   scaffold_check_bounds( 3, vector_count( args ) );
   l_name = (bstring)vector_get( args, 3 );

   /* Get the channel, or create it if it does not exist. */
   l = client_get_channel_by_name( c, l_name );
   if( NULL == l ) {
      /* Create a new client-side channel mirror. */
      channel_new( l, l_name, FALSE, c );
      client_add_channel( c, l );
      channel_add_client( l, c, FALSE );
      scaffold_print_debug(
         &module, "Client created local channel mirror: %s\n", bdata( l_name )
      );

      /* Strip off the #. */
      l_filename = bmidstr( l->name, 1, blength( l->name ) - 1 );
      bstr_res = bcatcstr( l_filename, ".tmx" );
      scaffold_check_nonzero( bstr_res );
      scaffold_check_null( l_filename );

      client_request_file( c, DATAFILE_TYPE_TILEMAP, l_filename );
   }

   scaffold_print_info(
      &module, "Client joined channel: %s\n", bdata( l_name )
   );

   scaffold_assert( hashmap_count( &(c->channels) ) > 0 );

cleanup:
   bdestroy( l_filename );
   return;
}

static const struct tagbstring str_closing = bsStatic( ":Closing" );

static void irc_client_error(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   if(
      2 <= vector_count( args ) &&
      0 == bstrcmp( &str_closing, (bstring)vector_get( args, 1 ) )
   ) {
      scaffold_print_debug( &module, "IRC Error received from server.\n" );
      client_stop( c );
   }
}

#endif /* ENABLE_LOCAL_CLIENT */

#ifdef USE_CHUNKS

#ifdef ENABLE_LOCAL_CLIENT

static void irc_client_gamedatablock(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   const char* progress_c,
      * total_c,
      * length_c,
      * filename_c,
      * type_c;
   struct CHUNKER_PROGRESS progress;

   irc_detect_malformed( 11, "GDB", line );

   progress.type = bgtoi( (bstring)vector_get( args, 5 ) );
   progress.current = bgtoi( (bstring)vector_get( args, 6 ) );
   progress.chunk_size = bgtoi( (bstring)vector_get( args, 7 ) );
   progress.total = bgtoi( (bstring)vector_get( args, 8 ) );
   progress.filename = (bstring)vector_get( args, 4 );
   progress.data = (bstring)vector_get( args, 10 );

   client_process_chunk( c, &progress );

cleanup:
   return;
}

#endif /* ENABLE_LOCAL_CLIENT */

static void irc_server_gamedataabort(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   irc_detect_malformed( 2, "GDA", line );

   scaffold_print_debug(
      &module,
      "Server: Terminating transfer of %s at request of client: %d\n",
      bdata( (bstring)vector_get( args, 1 ) ), c->link.socket
   );

   hashmap_remove_cb( &(c->chunkers), callback_free_chunkers, (bstring)vector_get( args, 1 ) );

cleanup:
   return;
}

#endif /* USE_CHUNKS */

static void irc_client_item_cache_start(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   struct CHANNEL* l = NULL;
   GFX_COORD_TILE x;
   GFX_COORD_TILE y;
   struct TILEMAP_ITEM_CACHE* cache = NULL;

   irc_detect_malformed( 4, "IC_S", line );

   l = client_get_channel_by_name( c, (bstring)vector_get( args, 1 ) );
   scaffold_check_null_msg( l, "Channel not found." );

   x = bgtoi( (bstring)vector_get( args, 2 ) );
   y = bgtoi( (bstring)vector_get( args, 3 ) );

   if( NULL != cache ) {
      scaffold_print_error(
         &module, "New item cache opened without closing previous: %b (%d, %d)",
         l->name, x, y
      );
   }

   cache = tilemap_get_item_cache( &(l->tilemap), x, y, TRUE );

   /* Empty the cache in preparation for a refresh. */
   vector_remove_cb( &(cache->items), callback_free_item_cache_items, NULL );
   last_item_cache = cache;

cleanup:
   return;
}

static void irc_client_item(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   SCAFFOLD_SIZE serial,
      count,
      sprite_id;
   struct ITEM* e = NULL;
   struct TILEMAP_ITEM_CACHE* cache = NULL;
   int retval;
   bstring display_name;
   struct ITEM_SPRITESHEET* catalog = NULL;
   bstring catalog_name = NULL;
   //struct ITEM_SPRITE* sprite = NULL;

   irc_detect_malformed( 6, "ITEM", line );

   serial = bgtoi( (bstring)vector_get( args, 1 ) );

   display_name = (bstring)vector_get( args, 2 );

   count = bgtoi( (bstring)vector_get( args, 3 ) );

   catalog_name = (bstring)vector_get( args, 4 );
   catalog = client_get_catalog( c, catalog_name );
   //scaffold_check_null_msg( catalog, "Catalog not found on client." );
   if( NULL == catalog ) {
      scaffold_print_debug(
         &module, "Client: Catalog not present in cache: %b\n", catalog_name
      );
      /*
      proto_client_request_spritesheet(
         c, catalog_name, CHUNKER_DATA_TYPE_ITEM_CATALOG
      );
      */
      client_request_file( c, DATAFILE_TYPE_ITEM_CATALOG, catalog_name );
   }

   sprite_id = bgtoi( (bstring)vector_get( args, 5 ) );

   /*
   sprite = item_spritesheet_get_sprite( catalog, sprite_id );
   scaffold_check_null_msg( sprite, "Sprite not found on client." );
   */

   item_new( e, display_name, count,(bstring)vector_get( args, 4 ), sprite_id, c );
   e->serial = serial;

   tilemap_drop_item_in_cache( last_item_cache, e );

   /*
   scaffold_assert( NULL != catalog->sprites_filename );
   client_request_file(
      c, CHUNKER_DATA_TYPE_ITEM_CATALOG_SPRITES, catalog->sprites_filename
   );
   */

   scaffold_print_debug(
      &module, "Client: Local instance of item updated: %b (%d, %b, %d)\n",
      e->display_name, e->serial, e->catalog_name, e->sprite_id
   );

cleanup:
   return;
}

static void irc_client_item_cache_end(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   last_item_cache = NULL;
}

static void irc_server_gamenewsprite(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
}

static void irc_server_mob(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   /* TODO: If the serial matches c's mob, update server. Otherwise send requested mob's details. */
}

#ifdef ENABLE_LOCAL_CLIENT

static void irc_client_gamenewsprite(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
}

static void irc_client_mob(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
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

   irc_detect_malformed( 8, "MOB", line );

   l = client_get_channel_by_name( c, (bstring)vector_get( args, 1 ) );
   scaffold_check_null( l );

   serial = bgtoi( (bstring)vector_get( args, 2 ) );

   mob_id = (bstring)vector_get( args, 3 );
   def_filename = (bstring)vector_get( args, 4 );
   nick = (bstring)vector_get( args, 5 );

   x = bgtoi( (bstring)vector_get( args, 6 ) );
   y = bgtoi( (bstring)vector_get( args, 7 ) );

   channel_set_mobile( l, serial, mob_id, def_filename, nick, x, y, c );
   scaffold_assert( 0 == scaffold_error );

   scaffold_print_debug(
      &module, "Client: Local instance of mobile updated: %s (%d)\n",
      bdata( nick ), serial
   );

cleanup:
   return;
}

static void irc_client_privmsg(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   bstring msg = NULL,
      nick = NULL;
   struct CHANNEL* l = NULL;
   struct MOBILE* o = NULL;
   struct CLIENT* c_sender = NULL;

   if( 3 > vector_count( args ) ) {
      goto cleanup;
   }

   msg = bmidstr( line, binchr( line, 2, &scaffold_colon_string ) + 1, blength( line ) );
   scaffold_check_null( msg );

#ifdef DEBUG_VERBOSE
   scaffold_print_debug( &module, "Message Incoming (%b): %b\n", args->entry[2], msg );
#endif /* DEBUG_VERBOSE */

   l = client_get_channel_by_name( c, (bstring)vector_get( args, 2 ) );
   scaffold_check_null( l );

   /* TODO: Authentication. */
   nick = /* Start at 1 to filter the : */
      bmidstr( line, 1, binchr( line, 0, &scaffold_exclamation_string ) - 1 );
/*
   c_sender = channel_get_client_by_name( l, nick );
   if( NULL != c_sender ) {
      o = c_sender->puppet;
      scaffold_check_null( o );

      mobile_speak( o, msg );
   } else {
*/
      backlog_speak( nick, msg );
/*   } */

cleanup:
   return;
}

#endif /* ENABLE_LOCAL_CLIENT */

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
IRC_COMMAND_ROW( "CREQ", irc_server_resource_request ),
#ifdef USE_CHUNKS
IRC_COMMAND_ROW( "GRF", irc_server_gamerequestfile ),
IRC_COMMAND_ROW( "GDA", irc_server_gamedataabort ),
IRC_COMMAND_ROW( "CREQ", irc_server_gamerequestfile ),
#endif /* USE_CHUNKS */
IRC_COMMAND_ROW( "GNS", irc_server_gamenewsprite ),
IRC_COMMAND_ROW( "MOB", irc_server_mob ),
#ifdef DEBUG_VM
IRC_COMMAND_ROW( "DEBUGVM", irc_server_debugvm ),
#endif /* DEBUGVM */
IRC_COMMAND_TABLE_END() };

#ifdef ENABLE_LOCAL_CLIENT

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
IRC_COMMAND_ROW( "IC_S", irc_client_item_cache_start ),
IRC_COMMAND_ROW( "ITEM", irc_client_item ),
IRC_COMMAND_ROW( "IC_E", irc_client_item_cache_end ),
IRC_COMMAND_ROW( "CRES", irc_client_resource_response ),
IRC_COMMAND_TABLE_END() };

#endif /* ENABLE_LOCAL_CLIENT */

static void irc_command_cleanup( const struct REF* ref ) {
   IRC_COMMAND* cmd = scaffold_container_of( ref, IRC_COMMAND, refcount );
   SCAFFOLD_SIZE i;

   /* Don't try to free the string or callback. */
   /* client_free( cmd->client ); */
   cmd->client = NULL;
   /* server_free( cmd->server ); */
   cmd->server = NULL;

   bdestroy( cmd->line );

   for( i = 0 ; vector_count( cmd->args ) > i ; i++ ) {
      bwriteallow( *((bstring)vector_get( cmd->args, i )) );
   }
   vector_remove_cb( cmd->args, callback_free_strings, NULL );
   vector_free( &(cmd->args) );

   mem_free( cmd );
}

void irc_command_free( IRC_COMMAND* cmd ) {
   refcount_dec( cmd, "command" );
}

IRC_COMMAND* irc_dispatch(
   const IRC_COMMAND* table, struct SERVER* s, struct CLIENT* c, const_bstring line
) {
   struct VECTOR* args = NULL;
   const IRC_COMMAND* command = NULL;
   SCAFFOLD_SIZE i;
   bstring cmd_test = NULL; /* Don't free this. */
   IRC_COMMAND* out = NULL;

   args = bgsplit( line, ' ' );
   scaffold_check_null( args );

   if( table == proto_table_server ) {
      scaffold_assert_server();
#ifdef ENABLE_LOCAL_CLIENT
   } else if( table == proto_table_client ) {
      scaffold_assert_client();
#endif /* ENABLE_LOCAL_CLIENT */
   }

   for( i = 0 ; vector_count( args ) > i ; i++ ) {
      bwriteprotect( *((bstring)vector_get( args, i )) );
   }

   for( i = 0 ; i < vector_count( args ) && IRC_LINE_CMD_SEARCH_RANGE > i ; i++ ) {
      cmd_test = (bstring)vector_get( args, i );
      for(
         command = &(table[0]);
         NULL != command->callback;
         command++
      ) {
         if( 0 == bstricmp( cmd_test, &(command->command) ) ) {
#ifdef DEBUG_VERBOSE
            if(
               0 != bstrncmp( cmd_test, &str_gdb, 3 ) &&
               0 != bstrncmp( cmd_test, &str_gu, 2 )
            ) {
               if( table == proto_table_server ) {
                  scaffold_print_debug(
                     &module, "Server Parse: %s\n", bdata( line ) );
               } else {
                  scaffold_print_debug(
                     &module, "Client Parse: %s\n", bdata( line ) );
               }
            }
#endif /* DEBUG_VERBOSE */

            out = mem_alloc( 1, IRC_COMMAND );
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
         &module, "Server: Parser unable to interpret: %b\n", line );
#ifdef ENABLE_LOCAL_CLIENT
   } else if( table == proto_table_client ) {
      scaffold_assert_client();
      scaffold_print_error(
         &module, "Client: Parser unable to interpret: %b\n", line );
#endif /* ENABLE_LOCAL_CLIENT */
   }


cleanup:
   if( NULL == out ) {
      for( i = 0 ; vector_count( args ) > i ; i++ ) {
         bwriteallow( *((bstring)vector_get( args, i )) );
      }
      vector_remove_cb( args, callback_free_strings, NULL );
      vector_free( &args );
   }
   return out;
}
