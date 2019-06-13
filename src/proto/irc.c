
#define PROTO_C
#include "../proto.h"

#include "../callback.h"
#include "../chunker.h"
#include "../datafile.h"
#include "../backlog.h"
#include "../channel.h"
#include "../ipc.h"
#include "../files.h"

struct IRC_CLIENT_DATA {
   int irc_mode;
};

typedef struct _IRC_COMMAND {
   struct REF refcount;
   const struct tagbstring command;
   void (*callback)( struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line );
   struct SERVER* server;
   struct CLIENT* client;
   struct VECTOR* args;
   bstring line;
} IRC_COMMAND;

#define IRC_COMMAND_TABLE_START( name ) \
   const IRC_COMMAND proto_table_ ## name []
#define IRC_COMMAND_ROW( command, callback ) \
   {REF_DISABLED, bsStatic( command ), callback, NULL, NULL, NULL, NULL}
#define IRC_COMMAND_TABLE_END() \
   {REF_DISABLED, bsStatic( "" ), NULL, NULL, NULL, NULL, NULL}

#ifndef IRC_C
extern IRC_COMMAND_TABLE_START( server );
extern IRC_COMMAND_TABLE_START( client );
#endif /* IRC_C */

#ifdef DEBUG
#define irc_print_args() \
   for( i = 0 ; args->qty > i ; i++ ) { \
      lg_debug( \
         __FILE__, "GDB %d: %s\n", i, bdata( args->entry[i] ) ); \
   }
#endif

#define IRC_LINE_CMD_SEARCH_RANGE 3

#ifdef USE_ITEMS
static struct TILEMAP_ITEM_CACHE* last_item_cache = NULL;
#endif /* USE_ITEMS */

static bstring irc_line_buffer = NULL;

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
   proto_printf( \
      c, ":%b %d %b %b", \
      server_get_remote( s ), enum_index, client_get_nick( c ), irc_reply_error_text[enum_index - 400] \
   );

#define irc_detect_malformed( args_qty, expression, line_var ) \
   if( args_qty != vector_count( args ) ) { \
      lg_error( \
         __FILE__, \
         "IRC: Malformed " expression " expression received: %b\n", \
         line_var \
      ); \
      goto cleanup; \
   }

static void proto_send( struct CLIENT* c, const bstring buffer ) {
   int bstr_retval;
   bstring buffer_copy = NULL;

   /* TODO: Make sure we're still connected. */

   buffer_copy = bstrcpy( buffer );
   lgc_null( buffer_copy );
   bstr_retval = bconchar( buffer_copy, '\r' );
   lgc_nonzero( bstr_retval );
   bstr_retval = bconchar( buffer_copy, '\n' );
   lgc_nonzero( bstr_retval );
   client_write( c, buffer_copy );

#ifdef DEBUG_NETWORK
   if( false != client_is_local( c ) ) {
      lg_debug( __FILE__, "Client sent to server: %b\n", buffer );
   } else {
      lg_debug( __FILE__, "Server sent to client: %b\n", buffer );
   }
#endif /* DEBUG_NETWORK */

cleanup:
   bdestroy( buffer_copy );
#ifdef ENABLE_LOCAL_CLIENT
   if( false != client_is_local( c ) ) {
      scaffold_assert_client();
   } else {
      scaffold_assert_server();
   }
#endif /* ENABLE_LOCAL_CLIENT */
   return;
}

static void proto_printf( struct CLIENT* c, const char* message, ... ) {
   bstring buffer = NULL;
   va_list varg;

#ifdef ENABLE_LOCAL_CLIENT
   if( client_is_local( c ) ) {
      scaffold_assert_client();
   } else {
      scaffold_assert_server();
   }
#endif /* ENABLE_LOCAL_CLIENT */

   buffer = bfromcstralloc( strlen( message ), "" );
   lgc_null( buffer );

   va_start( varg, message );
   lg_vsnprintf( buffer, message, varg );
   va_end( varg );

   if( 0 == lgc_error ) {
      proto_send( c, buffer );
   }

cleanup:
   bdestroy( buffer );
   return;
}

static void* proto_send_cb(
   size_t idx, void* iter, void* arg
) {
   struct CLIENT* c = (struct CLIENT*)iter;
   bstring buffer = (bstring)arg;
   proto_send( c, buffer );
   return NULL;
}

static void proto_channel_send(
   struct SERVER* s, struct CHANNEL* l, struct CLIENT* c_skip, bstring buffer
) {
   struct VECTOR* l_clients = NULL;
   bstring skip_nick = NULL;

   if( NULL != c_skip ) {
      skip_nick = client_get_nick( c_skip );
   }

   l_clients =
      hashmap_iterate_v( l->clients, callback_search_clients_r, skip_nick );
   lgc_null_msg( l_clients, "No clients found in channel." );

   vector_iterate( l_clients, proto_send_cb, buffer );

cleanup:
   if( NULL != l_clients ) {
      vector_remove_cb( l_clients, callback_v_free_clients, NULL );
      vector_cleanup( l_clients );
      mem_free( l_clients );
   }
   scaffold_assert_server();
}

static void proto_channel_printf( struct SERVER* s, struct CHANNEL* l, struct CLIENT* c_skip, const char* message, ... ) {
   bstring buffer = NULL;
   va_list varg;

   buffer = bfromcstralloc( strlen( message ), "" );
   lgc_null( buffer );

   va_start( varg, message );
   lg_vsnprintf( buffer, message, varg );
   va_end( varg );

   if( 0 == lgc_error ) {
      proto_channel_send( s, l, c_skip, buffer );
   }

   scaffold_assert_server();

cleanup:
   bdestroy( buffer );
   return;
}

void proto_register( struct CLIENT* c ) {
   proto_printf( c, "NICK %b", client_get_nick( c ) );
   proto_printf( c, "USER %b", client_get_realname( c ) );
}

void proto_client_join( struct CLIENT* c, const bstring name, const bstring mode ) {
   bstring buffer = NULL;
   int bstr_retval;
   /* We won't record the channel in our list until the server confirms it. */

   assert( NULL != name );

   scaffold_set_client();

   buffer = bfromcstr( "JOIN " );
   lgc_null( buffer );
   bstr_retval = bconcat( buffer, name );
   lgc_nonzero( bstr_retval );

   proto_send( c, buffer );

   client_set_flag( c, CLIENT_FLAGS_SENT_CHANNEL_JOIN );

   bstr_retval = bassigncstr( buffer, "GMODE " );
   lgc_nonzero( bstr_retval );
   bstr_retval = bconcat( buffer, name );
   lgc_nonzero( bstr_retval );
   bstr_retval = bconchar( buffer, ' ' );
   lgc_nonzero( bstr_retval );
   bstr_retval = bconcat( buffer, mode );
   lgc_nonzero( bstr_retval );

   proto_send( c, buffer );

cleanup:
   bdestroy( buffer );
}

void proto_client_leave( struct CLIENT* c, const bstring lname ) {
   int bstr_retval;
   bstring buffer = NULL;

   /* We won't record the channel in our list until the server confirms it. */

   scaffold_assert_client();

   buffer = bfromcstr( "PART " );
   lgc_null( buffer );
   bstr_retval = bconcat( buffer, lname );
   lgc_nonzero( bstr_retval );
   proto_send( c, buffer );

   /* TODO: Add callback from parser and only delete channel on confirm. */
   client_remove_channel( c, lname );

cleanup:
   bdestroy( buffer );
   return;
}

#ifdef USE_CHUNKS

void proto_send_chunk(
   struct CLIENT* c, struct CHUNKER* h, SCAFFOLD_SIZE start_pos,
   const bstring filename, const bstring data
) {
   DATAFILE_TYPE type = DATAFILE_TYPE_INVALID;
   SCAFFOLD_SIZE chunk_len = 0,
      raw_len = 0;
   bstring data_sent = NULL,
      parms_block = NULL;

   scaffold_assert_server();

   /* Note the starting point and progress for the client. */

   if( NULL != h ) {
      type = h->type;
      chunk_len = h->tx_chunk_length;
      raw_len = h->raw_length;
      data_sent = bstrcpy( data );
      parms_block = chunker_encode_block( h->parms );
   } else {
      /* Make things easy on the arg count validator. */
      data_sent = bfromcstr( "x" );
   }

   proto_printf(
      c, ":server GDB %b TILEMAP %b %b %d %d %d %d : %b",
      client_get_nick( c ), filename, parms_block, type, start_pos,
      chunk_len, raw_len, data_sent
   );

/* cleanup: */
   bdestroy( data_sent );
   bdestroy( parms_block );
   return;
}

void proto_abort_chunker( struct CLIENT* c, struct CHUNKER* h ) {
   scaffold_assert_client();
   lg_debug(
      __FILE__,
      "Client: Aborting transfer of %s from server due to cached copy.\n",
      bdata( h->filename )
   );
   proto_printf( c, "GDA %b", h->filename );
}

/** \brief This should ONLY be called by client_request_file(). All requests for
 *         files should go through that function.
 * \param c          Local client object.
 * \param filename   Name/path of the file (relative to the server root).
 * \param type       Type of file requested. Used in return processing.
 */
void proto_request_file( struct CLIENT* c, const bstring filename, DATAFILE_TYPE type ) {
   bstring req_parms = NULL;
   scaffold_assert_client();
   assert( 0 < blength( filename ) );
   /*
   lg_debug(
      __FILE__, "Client: Requesting %b file: %s\n",
      chunker_type_names[type].data, bdata( filename )
   );
   */
   req_parms = chunker_encode_block( NULL );
   proto_printf( c, "GRF %d %b %b", type, req_parms, filename );
   bdestroy( req_parms );
}

#endif /* USE_CHUNKS */

void proto_send_mob( struct CLIENT* c, struct MOBILE* o ) {
   bstring owner_nick = NULL;
   struct CHANNEL* l = NULL;
   bstring channel_name = NULL;

   scaffold_assert_server();
   assert( NULL != o );
   //assert( NULL != o->mob_id );
   //assert( NULL != o->def_filename );

   l = mobile_get_channel( o );
   assert( NULL != l );
   channel_name = channel_get_name( l );
   assert( NULL != channel_name );

   if( NULL != mobile_get_owner( o ) ) {
      assert( NULL != client_get_nick( mobile_get_owner( o ) ) );
      owner_nick = client_get_nick( mobile_get_owner( o ) );
   } else {
      owner_nick = &scaffold_null;
   }

   proto_printf(
      c, "MOB %b %d %b %b %b %d %d",
      channel_name, mobile_get_serial( o ), mobile_get_id( o ),
      mobile_get_def_filename( o ),
      owner_nick, mobile_get_x( o ), mobile_get_y( o )
   );
}

#ifdef USE_ITEMS
static void* proto_send_item_cb(
   size_t idx, void* iter, void* arg
) {
   struct ITEM* e = (struct ITEM*)iter;
   struct CLIENT* c = (struct CLIENT*)arg;

   scaffold_assert_server();

   assert( 0 < blength( e->catalog_name ) );

   proto_printf(
      c, "ITEM %d %b %d %b %d",
      e->serial, e->display_name, e->count, e->catalog_name, e->sprite_id
   );

   return NULL;
}

void proto_send_container( struct CLIENT* c, struct ITEM* e ) {
   if( false != item_is_container( e ) ) {

      proto_printf(
         c, "CONTAINER_START %d",
         e->serial
      );

      vector_iterate( e->content.container, proto_send_item_cb, c );

      proto_printf( c, "CONTAINER_END" );
   }
}

void proto_send_tile_cache(
   struct CLIENT* c, struct TILEMAP_ITEM_CACHE* cache
) {
   struct CHANNEL* l = NULL;

   scaffold_assert_server();

   l = scaffold_container_of( cache->tilemap, struct CHANNEL, tilemap );
   lgc_equal( l->sentinal, CHANNEL_SENTINAL );

   proto_printf(
      c, "IC_S %b %d %d",
      l->name, cache->position.x, cache->position.y
   );

   vector_iterate( &(cache->items), proto_send_item_cb, c );

   proto_printf( c, "IC_E" );

cleanup:
   return;
}

static void* proto_send_tile_cache_channel_cb(
   bstring idx, void* iter, void* arg
) {
   struct CLIENT* c = (struct CLIENT*)iter;
   struct TILEMAP_ITEM_CACHE* cache = (struct TILEMAP_ITEM_CACHE*)arg;

   proto_send_tile_cache( c, cache );
}

void proto_send_tile_cache_channel(
   struct CHANNEL* l, struct TILEMAP_ITEM_CACHE* cache
) {
   hashmap_iterate(
      l->clients,
      proto_send_tile_cache_channel_cb,
      cache
   );
}

#endif /* USE_ITEMS */

void proto_client_send_update( struct CLIENT* c, struct ACTION_PACKET* update ) {
   SCAFFOLD_SIZE serial = 0;
   scaffold_assert_client();
   struct MOBILE* o = NULL;
   struct MOBILE* target = NULL;
   enum ACTION_OP op = ACTION_OP_NONE;
   TILEMAP_COORD_TILE x, y;
   struct CHANNEL* l = NULL;

   lgc_null( update );

   o = action_packet_get_mobile( update );
   target = action_packet_get_target( update );
   op = action_packet_get_op( update );
   x = action_packet_get_tile_x( update );
   y = action_packet_get_tile_y( update );
   l = action_packet_get_channel( update );

   if( NULL != target ) {
      serial = mobile_get_serial( target );
   }
   proto_printf(
      c, "GU %b %d %d %d %d %d",
      l->name, mobile_get_serial( o ), op, x, y, serial
   );

cleanup:
   return;
}

void proto_server_send_update( struct CLIENT* c, struct ACTION_PACKET* update ) {
   SCAFFOLD_SIZE serial = 0;
   struct MOBILE* o = NULL;
   struct MOBILE* target = NULL;
   enum ACTION_OP op = ACTION_OP_NONE;
   TILEMAP_COORD_TILE x, y;
   struct CHANNEL* l = NULL;

   lgc_null( update );

   o = action_packet_get_mobile( update );
   target = action_packet_get_target( update );
   op = action_packet_get_op( update );
   x = action_packet_get_tile_x( update );
   y = action_packet_get_tile_y( update );
   l = action_packet_get_channel( update );

   scaffold_assert_server();

   if( NULL != target ) {
      serial = mobile_get_serial( target );
   }
   lgc_null( l );
   lgc_null( l->name );
   lgc_null( o );

   proto_printf(
      c, "GU %b %d %d %d %d %d",
      l->name, mobile_get_serial( o ), op, x, y, serial
   );

cleanup:
   return;
}

void proto_send_msg_channel( struct CLIENT* c, struct CHANNEL* ld, bstring msg ) {
   scaffold_assert_client();
   proto_send_msg( c, ld->name, msg );
}

void proto_send_msg_client( struct CLIENT* c, struct CLIENT* cd, bstring msg ) {
   scaffold_assert_client();
   proto_send_msg( c, client_get_nick( cd ), msg );
}

void proto_send_msg( struct CLIENT* c, bstring dest, bstring msg ) {
   scaffold_assert_client();
   proto_printf( c, "PRIVMSG %b :%b", dest, msg );
}

void proto_server_send_msg_channel(
   struct SERVER* s, struct CHANNEL* l, const bstring nick, const bstring msg
) {
   scaffold_assert_server();
   lgc_null_msg( l, "Invalid channel to send message." );
   proto_channel_printf(
      s, l, NULL, ":%b!%b@%b PRIVMSG %b :%b", nick, nick, server_get_remote( s ), l->name, msg
   );
cleanup:
   return;
}

void proto_client_stop( struct CLIENT* c ) {
   scaffold_assert_client();
   proto_printf( c, "QUIT" );
}

void proto_client_request_mobs( struct CLIENT* c, struct CHANNEL* l ) {
   proto_printf( c, "WHO %b", l->name );
}

#ifdef DEBUG_VM

void proto_client_debug_vm(
   struct CLIENT* c, struct CHANNEL* l, const bstring code
) {
   proto_printf( c, "DEBUGVM %b %b", l->name, code );
}

#endif /* DEBUG_VM */

/* This file contains our (possibly limited, slightly incompatible) version *
 * of the IRC protocol, as it interacts with our server and client objects. oopen game datapen game data*/

static void irc_server_reply_welcome( struct CLIENT* c, struct SERVER* s ) {

   proto_printf(
      c, ":%b 001 %b :Welcome to the Internet Relay Network %b!%b@%b",
      server_get_remote( s ), client_get_nick( c ), client_get_nick( c ), client_get_username( c ), client_get_remote( c )
   );

   proto_printf(
      c, ":%b 002 %b :Your host is isekai, running version 0.1",
      server_get_remote( s ), client_get_nick( c )
   );

   proto_printf(
      c, ":%b 003 %b :This server was created 01/01/1970",
      server_get_remote( s ), client_get_nick( c )
   );

   proto_printf(
      c, ":%b 004 %b :%b isekai-0.1 abBcCFiIoqrRswx abehiIklmMnoOPqQrRstvVz",
      server_get_remote( s ), client_get_nick( c ), server_get_remote( s )
   );

   proto_printf(
      c, ":%b 251 %b :There are %d users and 0 services on 1 servers",
      server_get_remote( s ), client_get_nick( c ), server_get_client_count( s )
   );

   client_set_flag( c, CLIENT_FLAGS_HAVE_WELCOME );
}

struct IRC_WHO_REPLY {
   struct CLIENT* c;
   struct CHANNEL* l;
   struct SERVER* s;
};

static void* irc_callback_reply_who( bstring idx, void* iter, void* arg ) {
   struct IRC_WHO_REPLY* who = (struct IRC_WHO_REPLY*)arg;
   struct CLIENT* c_iter = (struct CLIENT*)iter;
   proto_printf(
      who->c,
      ":%b 352 %b %b %b %b server %b H :0 %b",
      server_get_remote( who->s ),
      client_get_nick( who->c ),
      who->l->name,
      client_get_nick( c_iter ),
      client_get_remote( c_iter ),
      client_get_username( c_iter ),
      client_get_realname( c_iter )
   );
   return NULL;
}

static void irc_server_user(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   SCAFFOLD_SIZE i,
       consumed = 0;
   int bstr_result = 0;
   bstring realnamebuffer = NULL;

   /* Start at 1, skipping the command, itself. */
   for( i = 1 ; vector_count( args ) > i ; i++ ) {
      if( 0 == consumed ) {
         /* First arg: User */
         //bstr_result = bassign( c->username, (bstring)vector_get( args, i ) );
         bstr_result =
            client_set_names( c, NULL, (bstring)vector_get( args, i ), NULL );
         lgc_nonzero( bstr_result );
      } else if( 1 == consumed && scaffold_is_numeric( (bstring)vector_get( args, i ) ) ) {
         /* Second arg: Mode */
      } else if( 1 == consumed || 2 == consumed ) {
         /* Second or Third arg: * */
         if( 1 == consumed ) {
            consumed++;
         }
      } else if( 3 == consumed && ':' != ((bstring)vector_get( args, i ))->data[0] ) {
         /* Third or Fourth arg: Remote Host */
         //bstr_result = bassign( c->remote, (bstring)vector_get( args, i ) );
         bstr_result = client_set_remote( c, (bstring)vector_get( args, i ) );
         lgc_nonzero( bstr_result );
      } else if( 3 == consumed || 4 == consumed ) {
         /* Fourth or Fifth arg: Real Name */
         //bstr_result = bassign( c->realname, (bstring)vector_get( args, i ) );
         bstr_result =
            client_set_names( c, NULL, NULL, (bstring)vector_get( args, i ) );
         lgc_nonzero( bstr_result );
         if( 3 == consumed ) {
            consumed++; /* Extra bump for missing host. */
         }
      } else if( 4 < consumed ) {
         /* More real name. */
         realnamebuffer = bstrcpy( client_get_realname( c ) );
         bstr_result = bconchar( realnamebuffer, ' ' );
         lgc_nonzero( bstr_result );
         bstr_result =
            bconcat( realnamebuffer, (bstring)vector_get( args, i ) );
         lgc_nonzero( bstr_result );
         client_set_names( c, NULL, NULL, realnamebuffer );
      }
      consumed++;
   }

   client_set_flag( c, CLIENT_FLAGS_HAVE_USER );

   if( client_test_flags( c, CLIENT_FLAGS_HAVE_NICK ) ) {
      /* FIXME: irc_server_reply_welcome( local, remote ); */
   }

cleanup:
   bdestroy( realnamebuffer );
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
      proto_printf(
         c, ":%b 431 %b :No nickname given",
         server_get_remote( s ), client_get_nick( c )
      );
      goto cleanup;
   }

   server_set_client_nick( s, c, newnick );
   if( LGC_ERROR_NULLPO == lgc_error ) {
      proto_printf(
         c, ":%b 431 %b :No nickname given",
         server_get_remote( s ), client_get_nick( c )
      );
      goto cleanup;
   }

   if( LGC_ERROR_NOT_NULLPO == lgc_error ) {
      proto_printf(
         c, ":%b 433 %b :Nickname is already in use",
         server_get_remote( s ), client_get_nick( c )
      );
      goto cleanup;
   }

   if( 0 < blength( client_get_nick( c ) ) ) {
      oldnick = bstrcpy( client_get_nick( c ) );
      lgc_null( oldnick );
   } else {
      oldnick = bstrcpy( client_get_username( c ) );
      lgc_null( oldnick );
   }

   bstr_result = client_set_names( c, newnick, NULL, NULL );
   lgc_nonzero( bstr_result );

   client_set_flag( c, CLIENT_FLAGS_HAVE_NICK );

   /* Don't reply yet if there's been no USER statement yet. */
   if( !client_test_flags( c, CLIENT_FLAGS_HAVE_USER ) ) {
      lg_debug(
         __FILE__, "Client %p quietly changed nick from: %b To: %b\n",
         c, oldnick, client_get_nick( c )
      );
      goto cleanup;
   }

   if( !client_test_flags( c, CLIENT_FLAGS_HAVE_WELCOME ) ) {
      irc_server_reply_welcome( c, s );
   }

   proto_printf(
      c, ":%b %b :%b!%b@%b NICK %b",
      server_get_remote( s ), client_get_nick( c ), oldnick,
      client_get_username( c ), client_get_remote( c ), client_get_nick( c )
   );

   lg_debug(
      __FILE__, "Client %p changed nick from: %b To: %b\n",
      c, oldnick, client_get_nick( c )
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

   proto_printf(
      c, "ERROR :Closing Link: %b (Client Quit)",
      client_get_nick( c )
   );

   server_drop_client( s, client_get_nick( c ) );

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
   lgc_null( response );

   /* TODO: Root the command stuff out of args. */
   ison = server_get_clients_online( s, args );
   lgc_null( ison );
   vector_lock( ison, true );
   for( i = 0 ; vector_count( ison ) > i ; i++ ) {
      c_iter = (struct CLIENT*)vector_get( ison, i );
      /* 1 for the main list + 1 for the vector. */
      //assert( 2 <= c_iter->refcount.count );
      bstr_result = bconcat( response, client_get_nick( c_iter ) );
      lgc_nonzero( bstr_result );
      bstr_result = bconchar( response, ' ' );
      lgc_nonzero( bstr_result );
   }
   vector_lock( ison, false );
   lgc_null( response );

   proto_printf( c, ":%b 303 %b :%b", server_get_remote( s ), client_get_nick( c ), response );

cleanup:
   if( NULL != ison ) {
      vector_remove_cb( ison, callback_v_free_clients, NULL );
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
      proto_printf(
         c, ":%b 461 %b %b :Not enough parameters",
         server_get_remote( s ), client_get_username( c ), (bstring)vector_get( args, 0 )
      );
      goto cleanup;
   }

   /* Add the channel to the server if it does not exist. */
   namehunt = bstrcpy( (bstring)vector_get( args, 1 ) );
   bstr_result = btrimws( namehunt );
   lgc_nonzero( bstr_result );

   if( true != scaffold_string_is_printable( namehunt ) ) {
      proto_printf(
         c, ":%b 403 %b %b :No such channel",
         server_get_remote( s ), client_get_username( c ), namehunt
      );
      goto cleanup;
   }

   l = server_add_channel( s, namehunt, c );
   lgc_null( l );

   assert( NULL != c );
   assert( NULL != s );
   assert( NULL != client_get_nick( c ) );
   assert( NULL != client_get_username( c ) );
   assert( NULL != client_get_remote( c ) );
   assert( NULL != l->name );

   /* Announce the new join. */
   proto_channel_printf(
      s, l, c, ":%b!%b@%b JOIN %b", client_get_nick( c ), client_get_username( c ), client_get_remote( c ), l->name
   );

   /* Now tell the joining client. */
   proto_printf(
      c, ":%b!%b@%b JOIN %b",
      client_get_nick( c ), client_get_username( c ), client_get_remote( c ), l->name
   );

   cat_names = vector_new();
   hashmap_iterate( l->clients, callback_concat_clients, cat_names );
   names = bfromcstr( "" );
   lgc_null( names );
   vector_iterate( cat_names, callback_concat_strings, names );

   proto_printf(
      c, ":%b 332 %b %b :%b",
      server_get_remote( s ), client_get_nick( c ), l->name, l->topic
   );

   proto_printf(
      c, ":%b 353 %b = %b :%b",
      server_get_remote( s ), client_get_nick( c ), l->name, names
   );

   proto_printf(
      c, ":%b 366 %b %b :End of NAMES list",
      server_get_remote( s ), client_get_nick( c ), l->name
   );

   assert( client_get_channels_count( c ) > 0 );
   assert( server_get_channels_count( s ) > 0 );

cleanup:
   if( NULL != cat_names ) {
      vector_remove_cb( cat_names, callback_v_free_strings, NULL );
      vector_free( &cat_names );
   }
   bdestroy( names );
   bdestroy( namehunt );
   return;
}

static void irc_server_gmode(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   struct CHANNEL* l = NULL;
   bstring mode = NULL;
   bstring name = NULL;

   irc_detect_malformed( 3, "GMODE", line )

   name = (bstring)vector_get( args, 1 );
   lgc_null( name );
   l = client_get_channel_by_name( c, name );
   lgc_null( l );

   mode = (bstring)vector_get( args, 2 );

   //channel_set_mode( l, mode );
   proto_channel_printf( s, l, NULL, "GMODE %b %b", l->name, mode );
   lg_info( __FILE__, "Isekai mode for %b set to %b.", name, mode );

cleanup:
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

   msg_list = vector_new();
   for( i = 1 ; vector_count( args ) > i ; i++ ) {
      vector_add( msg_list, bstrcpy( (bstring)vector_get( args, i ) ) );
   }

   lg_debug( __FILE__, "%s\n", ((bstring)vector_get( msg_list, 0 ))->data );

   msg = bfromcstr( "" );
   lgc_null( msg );
   vector_iterate( msg_list, callback_concat_strings, msg );

   c_dest = server_get_client( s, (bstring)vector_get( args, 1 ) );
   if( NULL != c_dest ) {
      proto_printf(
         c_dest, ":%b!%b@%b %b", client_get_nick( c ),
         client_get_username( c ), client_get_remote( c ), line
      );
      goto cleanup;
   }

   /* Maybe it's for a channel, instead? */
   l_dest = server_get_channel_by_name( s, (bstring)vector_get( args, 1 ) );
   if( NULL != l_dest ) {
      proto_channel_printf(
         s, l_dest, c, ":%b!%b@%b %b", client_get_nick( c ),
         client_get_username( c ), client_get_remote( c ), line
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
      lg_error( __FILE__, "Non-channel WHO not yet implemented.\n" );
      goto cleanup;
   }

   l = server_get_channel_by_name( s, (bstring)vector_get( args, 1 ) );
   lgc_null( l );

   who.c = c;
   who.l = l;
   who.s = s;
   hashmap_iterate( l->clients, irc_callback_reply_who, &who );
   proto_printf(
      c, ":server 315 %b :End of /WHO list.",
      client_get_nick( c ), l->name
   );

   hashmap_iterate( l->clients, callback_send_mobs_to_channel, l );

cleanup:
   return;
}

static void irc_server_ping(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {

   if( 2 > vector_count( args ) ) {
      goto cleanup;
   }

   proto_printf(
      c, ":%b PONG %b :%b", server_get_remote( s ), server_get_remote( s ), client_get_remote( c )
   );

cleanup:
   return;
}

#ifdef USE_CHUNKS

static void irc_server_gamerequestfile(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   DATAFILE_TYPE type;
   bstring file_path_found = NULL,
      filename = NULL;
   struct CHUNKER_PARMS* parms = NULL;

   irc_detect_malformed( 4, "GRF", line );

   type = (DATAFILE_TYPE)bgtoi( (bstring)vector_get( args, 1 ) );

   filename = (bstring)vector_get( args, 3 );

   file_path_found = files_search( filename );
   if( NULL == file_path_found ) {
      lg_debug( __FILE__, "Sending cancel for: %b\n", filename );
      proto_send_chunk( c, NULL, 0, filename, NULL );
      goto cleanup;
   }

   /* Decode the parameter block. */
   parms = chunker_decode_block( (bstring)vector_get( args, 2 ) );
   lgc_null( parms );

   client_send_file( c, type, files_root( NULL ), file_path_found, parms );

cleanup:
   bdestroy( file_path_found );
   return;
}

#endif /* USE_CHUNKS */

static void irc_server_gameupdate(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   SCAFFOLD_SIZE serial,
      target_serial;
   struct ACTION_PACKET* update = NULL;
   struct MOBILE* o = NULL;
   struct CHANNEL* l = NULL;
   enum ACTION_OP op = ACTION_OP_NONE;

   irc_detect_malformed( 7, "GU", line )

   l = client_get_channel_by_name( c, (bstring)vector_get( args, 1 ) );
   lgc_null( l );

   serial = bgtoi( (bstring)vector_get( args, 2 ) );
   o = (struct MOBILE*)vector_get( l->mobiles, serial );
   lgc_null( o );

   op = (enum ACTION_OP)bgtoi( (bstring)vector_get( args, 3 ) );

   target_serial = bgtoi( (bstring)vector_get( args, 6 ) );

   if( c != mobile_get_owner( o ) ) {
      lg_error(
         __FILE__,
         "Client %s attempted to modify mobile %d to %d without permission.\n",
         bdata( client_get_nick( c ) ), mobile_get_serial( o ), op
      );
      goto cleanup;
   }

   update = action_packet_new(
      l,
      o,
      op,
      bgtoi( (bstring)vector_get( args, 4 ) ),
      bgtoi( (bstring)vector_get( args, 5 ) ),
      (struct MOBILE*)vector_get( l->mobiles, target_serial )
   );
   lgc_null( update );

   /* Everything checks out! */
   action_enqueue( update, ACTION_QUEUE_SERVER );

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
      lgc_nonzero( bstr_ret );
      bstr_ret = bconcat( code, (bstring)vector_get( args, i ) );
      lgc_nonzero( bstr_ret );
   }

   l = server_get_channel_by_name( s, lname );
   lgc_null( l );

   /* FIXME: channel_vm_start( l, code ); */

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
   struct ACTION_PACKET* update = NULL;
   struct CHANNEL* l = NULL;
   struct MOBILE* o = NULL;
   struct MOBILE* target = NULL;

   l = client_get_channel_by_name( c, (bstring)vector_get( args, 1 ) );
   lgc_null( l );

   serial = bgtoi( (bstring)vector_get( args, 2 ) );
   o = (struct MOBILE*)vector_get( l->mobiles, serial );
   lgc_null( o );

   target_serial = bgtoi( (bstring)vector_get( args, 6 ) );
   target =
      (struct MOBILE*)vector_get( l->mobiles, target_serial );
   /* No NULL check. If it's NULL, it's NULL. */

   update = action_packet_new(
      l,
      o,
      (enum ACTION_OP)bgtoi( (bstring)vector_get( args, 3 ) ),
      bgtoi( (bstring)vector_get( args, 4 ) ),
      bgtoi( (bstring)vector_get( args, 5 ) ),
      target
   );
   lgc_null( update );

   action_enqueue( update, ACTION_QUEUE_CLIENT );

cleanup:
   return;
}

static void irc_client_gmode(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   struct CHANNEL* l = NULL;
   bstring mode = NULL;
   bstring name = NULL;

   irc_detect_malformed( 3, "GMODE", line )

   name = (bstring)vector_get( args, 1 );
   lgc_null( name );
   l = client_get_channel_by_name( c, name );
   lgc_null( l );

   mode = (bstring)vector_get( args, 2 );

   channel_set_mode( l, mode );
   lg_info( __FILE__, "Isekai mode for %b set to %b.", name, mode );

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

   lgc_bounds( 3, vector_count( args ) );
   l_name = (bstring)vector_get( args, 3 );

   /* Get the channel, or create it if it does not exist. */
   /* TODO: Move this to client module. */
   l = client_get_channel_by_name( c, l_name );
   if( NULL == l ) {
      /* Create a new client-side channel mirror. */
      channel_new( l, l_name, false, c, NULL );
      client_add_channel( c, l );
      channel_add_client( l, c, false );
      lg_debug(
         __FILE__, "Client created local channel mirror: %s\n", bdata( l_name )
      );

      /* Strip off the #. */
      l_filename = bmidstr( l->name, 1, blength( l->name ) - 1 );
      bstr_res = bcatcstr( l_filename, ".tmx" );
      lgc_nonzero( bstr_res );
      lgc_null( l_filename );

      client_request_file_later( c, DATAFILE_TYPE_TILEMAP, l_filename );
   }

   lg_info(
      __FILE__, "Client joined channel: %s\n", bdata( l_name )
   );

   assert( client_get_channels_count( c ) > 0 );

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
      lg_debug( __FILE__, "IRC Error received from server.\n" );
      client_stop( c );
   }
}

#endif /* ENABLE_LOCAL_CLIENT */

void proto_empty_buffer( struct CLIENT* c ) {
   bstring buffer = NULL;
   buffer = bfromcstr( "" );
   while( 0 < client_read( c, buffer ) );
/* cleanup: */
   bdestroy( buffer );
   return;
}

#ifdef USE_CHUNKS

#ifdef ENABLE_LOCAL_CLIENT

static void irc_client_gamedatablock(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   struct CHUNKER_PROGRESS progress;

   irc_detect_malformed( 12, "GDB", line );

   progress.parms = chunker_decode_block( (bstring)vector_get( args, 5 ) );
   lg_debug( __FILE__, "Chunk parms: %b\n", (bstring)vector_get( args, 5 ) );
   progress.type = bgtoi( (bstring)vector_get( args, 6 ) );
   progress.current = bgtoi( (bstring)vector_get( args, 7 ) );
   progress.chunk_size = bgtoi( (bstring)vector_get( args, 8 ) );
   progress.total = bgtoi( (bstring)vector_get( args, 9 ) );
   progress.filename = (bstring)vector_get( args, 4 );
   progress.data = (bstring)vector_get( args, 11 );

   client_process_chunk( c, &progress );

cleanup:
   return;
}

#endif /* ENABLE_LOCAL_CLIENT */

static void irc_server_gamedataabort(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   irc_detect_malformed( 2, "GDA", line );

   lg_debug(
      __FILE__,
      "Server: Terminating transfer of %s at request of client: %p\n",
      bdata( (bstring)vector_get( args, 1 ) ), c
   );

   client_remove_chunkers( c, (bstring)vector_get( args, 1 ) );

cleanup:
   return;
}

#endif /* USE_CHUNKS */

#ifdef USE_ITEMS

static void irc_client_item_cache_start(
   struct CLIENT* c, struct SERVER* s, struct VECTOR* args, bstring line
) {
   struct CHANNEL* l = NULL;
   TILEMAP_COORD_TILE x;
   TILEMAP_COORD_TILE y;
   struct TILEMAP_ITEM_CACHE* cache = NULL;

   irc_detect_malformed( 4, "IC_S", line );

   l = client_get_channel_by_name( c, (bstring)vector_get( args, 1 ) );
   lgc_null_msg( l, "Channel not found." );

   x = bgtoi( (bstring)vector_get( args, 2 ) );
   y = bgtoi( (bstring)vector_get( args, 3 ) );

   if( NULL != cache ) {
      lg_error(
         __FILE__, "New item cache opened without closing previous: %b (%d, %d)",
         l->name, x, y
      );
   }

   cache = tilemap_get_item_cache( l->tilemap, x, y, true );

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
   bstring display_name;
   struct ITEM_SPRITESHEET* catalog = NULL;
   bstring catalog_name = NULL;

   irc_detect_malformed( 6, "ITEM", line );

   serial = bgtoi( (bstring)vector_get( args, 1 ) );

   display_name = (bstring)vector_get( args, 2 );

   count = bgtoi( (bstring)vector_get( args, 3 ) );

   catalog_name = (bstring)vector_get( args, 4 );
   catalog = client_get_catalog( c, catalog_name );
   if( NULL == catalog ) {
      lg_debug(
         __FILE__, "Client: Catalog not present in cache: %b\n", catalog_name
      );
      client_request_file( c, DATAFILE_TYPE_ITEM_CATALOG, catalog_name );
   }

   sprite_id = bgtoi( (bstring)vector_get( args, 5 ) );
   item_new( e, display_name, count,(bstring)vector_get( args, 4 ), sprite_id, c );
   e->serial = serial;

   tilemap_drop_item_in_cache( last_item_cache, e );

   lg_debug(
      __FILE__, "Client: Local instance of item updated: %b (%d, %b, %d)\n",
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

#endif /* USE_ITEMS */

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
   uint8_t serial = 0;
   bstring def_filename = NULL,
      mob_id = NULL,
      nick = NULL;
   struct CHANNEL* l = NULL;
   SCAFFOLD_SIZE x = 0,
      y = 0;

   irc_detect_malformed( 8, "MOB", line );

   l = client_get_channel_by_name( c, (bstring)vector_get( args, 1 ) );
   lgc_null( l );

   serial = bgtoi( (bstring)vector_get( args, 2 ) );

   mob_id = (bstring)vector_get( args, 3 );
   def_filename = (bstring)vector_get( args, 4 );
   nick = (bstring)vector_get( args, 5 );

   x = bgtoi( (bstring)vector_get( args, 6 ) );
   y = bgtoi( (bstring)vector_get( args, 7 ) );

   channel_set_mobile( l, serial, mob_id, def_filename, nick, x, y );
   lg_debug( __FILE__, "scaffold_error: %d\n", lgc_error );
   lgc_nonzero( lgc_error );

   lg_debug(
      __FILE__, "Client: Local instance of mobile updated: %s (%d)\n",
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

   if( 3 > vector_count( args ) ) {
      goto cleanup;
   }

   msg = bmidstr( line, binchr( line, 2, &scaffold_colon_string ) + 1, blength( line ) );
   lgc_null( msg );

#ifdef DEBUG_VERBOSE
   lg_debug(
      __FILE__, "Message Incoming (%b): %b\n",
      (bstring)vector_get( args, 2 ), msg
   );
#endif /* DEBUG_VERBOSE */

   l = client_get_channel_by_name( c, (bstring)vector_get( args, 2 ) );
   lgc_null( l );

   /* TODO: Authentication. */
   nick = /* Start at 1 to filter the : */
      bmidstr( line, 1, binchr( line, 0, &scaffold_exclamation_string ) - 1 );

#ifndef DISABLE_BACKLOG
   backlog_speak( nick, msg );
#endif /* DISABLE_BACKLOG */

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
IRC_COMMAND_ROW( "GMODE", irc_server_gmode ),
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
IRC_COMMAND_ROW( "MOB", irc_server_mob ),
#ifdef DEBUG_VM
IRC_COMMAND_ROW( "DEBUGVM", irc_server_debugvm ),
#endif /* DEBUGVM */
IRC_COMMAND_TABLE_END() };

#ifdef ENABLE_LOCAL_CLIENT

IRC_COMMAND_TABLE_START( client ) = {
IRC_COMMAND_ROW( "GU", irc_client_gu  ),
IRC_COMMAND_ROW( "366", irc_client_join ),
IRC_COMMAND_ROW( "GMODE", irc_client_gmode ),
IRC_COMMAND_ROW( "ERROR", irc_client_error  ),
#ifdef USE_CHUNKS
IRC_COMMAND_ROW( "GDB", irc_client_gamedatablock ),
#endif /* USE_CHUNKS */
IRC_COMMAND_ROW( "GNS", irc_client_gamenewsprite ),
IRC_COMMAND_ROW( "MOB", irc_client_mob ),
IRC_COMMAND_ROW( "PRIVMSG", irc_client_privmsg ),
#ifdef USE_ITEMS
IRC_COMMAND_ROW( "IC_S", irc_client_item_cache_start ),
IRC_COMMAND_ROW( "ITEM", irc_client_item ),
IRC_COMMAND_ROW( "IC_E", irc_client_item_cache_end ),
#endif // USE_ITEMS
IRC_COMMAND_TABLE_END() };

#endif /* ENABLE_LOCAL_CLIENT */

bool proto_dispatch( struct CLIENT* c, struct SERVER* s ) {
   bool keep_going = true;
   SCAFFOLD_SIZE last_read_count = 0;
   const IRC_COMMAND* table = proto_table_server;
   int bstr_result;
   struct VECTOR* args = NULL;
   SCAFFOLD_SIZE args_count = 0;
   bstring cmd_test = NULL; /* Don't free this. */
   SCAFFOLD_SIZE i = 0, j = 0;

#ifdef ENABLE_LOCAL_CLIENT
   /* Figure out if we're being called from a client or server. */
   if( client_is_local( c ) ) {
      table = proto_table_client;
   } else {
#endif /* ENABLE_LOCAL_CLIENT */
      assert( NULL != s );
#ifdef ENABLE_LOCAL_CLIENT
   }
#endif /* ENABLE_LOCAL_CLIENT */

   /* Make sure a buffer is present. */
   bwriteallow( (*irc_line_buffer) ); /* Unprotect the buffer. */
   bstr_result = btrunc( irc_line_buffer, 0 );
   lgc_nonzero( bstr_result );

   /* Get the next line and clean it up. */
   last_read_count = client_read( c, irc_line_buffer );

   //if( !ipc_connected( c->link ) ) {
   if( !client_is_connected( c ) ) {
      /* Return an empty command to force abortion of the iteration. */
      keep_going = false;
      goto cleanup;
   }

   /* Everything is fine, so tidy up the buffer. */
   btrimws( irc_line_buffer );
   bwriteprotect( (*irc_line_buffer) ); /* Protect the buffer until next read. */

   /* The -1 is not bubbled up; the scaffold_error should suffice. */
   if( 0 >= last_read_count ) {
      goto cleanup;
   }

#ifdef DEBUG_NETWORK
   lg_debug(
      __FILE__, "Server: Line received: %b\n", irc_line_buffer
   );
#endif /* DEBUG_NETWORK */

   args = bgsplit( irc_line_buffer, ' ' );
   lgc_null( args );

   if( table == proto_table_server ) {
      scaffold_assert_server();
#ifdef ENABLE_LOCAL_CLIENT
   } else if( table == proto_table_client ) {
      scaffold_assert_client();
#endif /* ENABLE_LOCAL_CLIENT */
   }

   args_count = vector_count( args );
   for( i = 0 ; args_count > i ; i++ ) {
      bwriteprotect( *((bstring)vector_get( args, i )) );
   }

   for( i = 0 ; i < args_count && IRC_LINE_CMD_SEARCH_RANGE > i ; i++ ) {
      cmd_test = (bstring)vector_get( args, i );
      for(
         /*command = &(table[0]);
         NULL != command->callback;
         command++*/
         j = 0 ; NULL != table[j].callback ; j++
      ) {
         if( 0 == bstricmp( cmd_test, &(table[j].command) ) ) {
            table[j].callback( c, s, args, irc_line_buffer );

            /* Found a command, so short-circuit. */
            goto cleanup;
         }
      }
   }

   if( table == proto_table_server ) {
      scaffold_assert_server();
      lg_error(
         __FILE__,
         "Server: Parser unable to interpret: %b\n", irc_line_buffer );
#ifdef ENABLE_LOCAL_CLIENT
   } else if( table == proto_table_client ) {
      scaffold_assert_client();
      lg_error(
         __FILE__,
         "Client: Parser unable to interpret: %b\n", irc_line_buffer );
#endif /* ENABLE_LOCAL_CLIENT */
   }

cleanup:
   /*
   if( NULL == out ) {
      for( i = 0 ; args_count > i ; i++ ) {
         bwriteallow( *((bstring)vector_get( args, i )) );
      }
      vector_remove_cb( args, callback_v_free_strings, NULL );
      vector_free( &args );
   }
   mem_free( cmd ); */

   for( i = 0 ; args_count > i ; i++ ) {
      bwriteallow( *((bstring)vector_get( args, i )) );
   }
   vector_remove_cb( args, callback_v_free_strings, NULL );
   vector_free( &args );
   return keep_going;
}

void proto_setup() {
   irc_line_buffer = bfromcstralloc( IPC_BUFFER_LEN, "" );
}

void proto_shutdown() {
   bwriteallow( *irc_line_buffer );
   bdestroy( irc_line_buffer );
}
