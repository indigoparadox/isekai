#include "parser.h"

#include "server.h"
#include "heatshrink/heatshrink_decoder.h"
#include "heatshrink/heatshrink_encoder.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
   SERVER* s;
   CLIENT* c;
   CHANNEL* l;
} PARSER_TRIO;

/* This file contains our (possibly limited, slightly incompatible) version *
 * of the IRC protocol, as it interacts with our server and client objects. oopen game datapen game data*/

static void parser_server_reply_welcome( void* local, void* remote ) {
   CLIENT* c = (CLIENT*)remote;
   SERVER* s = (SERVER*)local;

   server_client_printf(
      s, c, ":%b 001 %b :Welcome to the Internet Relay Network %b!%b@%b",
      s->self.remote, c->nick, c->nick, c->username, c->remote
   );

   server_client_printf(
      s, c, ":%b 002 %b :Your host is %b, running version %b",
      s->self.remote, c->nick, s->servername, s->version
   );

   server_client_printf(
      s, c, ":%b 003 %b :This server was created 01/01/1970",
      s->self.remote, c->nick
   );

   server_client_printf(
      s, c, ":%b 004 %b :%b %b-%b abBcCFiIoqrRswx abehiIklmMnoOPqQrRstvVz",
      s->self.remote, c->nick, s->self.remote, s->servername, s->version
   );

   server_client_printf(
      s, c, ":%b 251 %b :There are %d users and 0 services on 1 servers",
      s->self.remote, c->nick, vector_count( &(s->clients) )
   );

   c->flags |= CLIENT_FLAGS_HAVE_WELCOME;
}

static void parser_server_reply_nick( void* local, void* remote,
                                      bstring oldnick ) {
   CLIENT* c = (CLIENT*)remote;
   SERVER* s = (SERVER*)local;

   if( !(c->flags & CLIENT_FLAGS_HAVE_WELCOME) ) {
      goto cleanup;
   }

   if( NULL == oldnick ) {
      oldnick = c->username;
   }

   server_client_printf(
      s, c, ":%b %b :%b!%b@%b NICK %b",
      s->self.remote, c->nick, oldnick, c->username, c->remote, c->nick
   );

cleanup:
   return;
}

void parser_server_reply_motd( void* local, void* remote ) {
   CLIENT* c = (CLIENT*)remote;
   SERVER* s = (SERVER*)local;

   if( 1 > blength( c->nick ) ) {
      goto cleanup;
   }

   if( c->flags & CLIENT_FLAGS_HAVE_MOTD ) {
      goto cleanup;
   }

   server_client_printf(
      s, c, ":%b 433 %b :MOTD File is missing",
      s->self.remote, c->nick
   );

   c->flags |= CLIENT_FLAGS_HAVE_MOTD;

cleanup:
   return;
}

static void parser_server_user( void* local, void* remote,
                                struct bstrList* args ) {
   CLIENT* c = (CLIENT*)remote;
   SERVER* s = (SERVER*)local;
   int i,
       consumed = 0;
   char* c_mode = NULL;
   int bstr_result = 0;

   /* TODO: Error on already registered. */

   /* Start at 1, skipping the command, itself. */
   for( i = 1 ; args->qty > i ; i++ ) {
      if( 0 == consumed ) {
         /* First arg: User */
         bstr_result = bassign( c->username, args->entry[i] );
         scaffold_check_nonzero( bstr_result );
      } else if( 1 == consumed && scaffold_is_numeric( args->entry[i] ) ) {
         /* Second arg: Mode */
         c_mode = bdata( args->entry[i] );
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
         bconchar( c->realname, ' ' );
         bconcat( c->realname, args->entry[i] );
      }
      consumed++;
   }

   /* Try to set the nick. */
   if(
      0 == bstrcmp( &scaffold_empty_string, c->nick ) &&
      0 != server_set_client_nick( s, c, c->username )
   ) {
      server_client_printf(
         s, c, ":%b 433 %b :Nickname is already in use",
         s->self.remote, c->username
      );
   }

   //scaffold_print_debug( "User: %s, Real: %s, Remote: %s\n", bdata( c->username ), bdata( c->realname ), bdata( c->remote ) );

   c->flags |= CLIENT_FLAGS_HAVE_USER;

   if( ( c->flags & CLIENT_FLAGS_HAVE_NICK ) ) {
      parser_server_reply_welcome( local, remote );
   }
   //parser_server_reply_nick( local, remote, NULL );

cleanup:
   return;
}

static void parser_server_nick( void* local, void* remote,
                                struct bstrList* args ) {
   CLIENT* c = (CLIENT*)remote;
   SERVER* s = (SERVER*)local;
   bstring oldnick = NULL;
   bstring newnick = NULL;
   int nick_return;
   int bstr_result = 0;

   if( 2 >= args->qty ) {
      newnick = args->entry[1];
   }

   nick_return = server_set_client_nick( s, c, newnick );
   if( ERR_NONICKNAMEGIVEN == nick_return ) {
      server_client_printf(
         s, c, ":%b 431 %b :No nickname given",
         s->self.remote, c->nick
      );
      goto cleanup;
   }

   if( ERR_NICKNAMEINUSE == nick_return ) {
      server_client_printf(
         s, c, ":%b 433 %b :Nickname is already in use",
         s->self.remote, c->nick
      );
      goto cleanup;
   }

   if( 0 < blength( c->nick ) ) {
      oldnick = bstrcpy( c->nick );
      scaffold_check_null( oldnick );
   }

   bstr_result = bassign( c->nick, newnick );
   scaffold_check_nonzero( bstr_result );

   c->flags |= CLIENT_FLAGS_HAVE_NICK;

   /* Don't reply yet if there's been no USER statement yet. */
   if( !(c->flags & CLIENT_FLAGS_HAVE_USER) ) {
      goto cleanup;
   }

   if( !(c->flags & CLIENT_FLAGS_HAVE_WELCOME) ) {
      parser_server_reply_welcome( local, remote );
   }

   parser_server_reply_nick( local, remote, oldnick );

cleanup:

   bdestroy( oldnick );

   return;
}

static void parser_server_quit( void* local, void* remote,
                                struct bstrList* args ) {
   SERVER* s = (SERVER*)local;
   CLIENT* c = (CLIENT*)remote;
   bstring message;
   bstring space;

   /* TODO: Send everyone parting message. */
   space = bfromcstr( " " );
   message = bjoin( args, space );

   server_client_printf(
      s, c, "ERROR :Closing Link: %b (Client Quit)",
      c->nick
   );

   server_drop_client( s, c->nick );

   bdestroy( message );
   bdestroy( space );
}

typedef struct {
   bstring clients;
   struct bstrList* args;
} PARSER_ISON;

static void* parser_cmp_ison( VECTOR* v, size_t idx, void* iter, void* arg ) {
   CLIENT* c;
   PARSER_ISON* ison = (PARSER_ISON*)arg;
   size_t i;

   for( i = 0 ; ison->args->qty > i ; i++ ) {
      c = client_cmp_nick( v, idx, iter, ison->args->entry[i] );
      if( NULL != c ) {
         bconcat( ison->clients, c->nick );
         bconchar( ison->clients, ' ' );
      }
   }

   return NULL;
}

static void parser_server_ison( void* local, void* remote,
                                struct bstrList* args ) {
   SERVER* s = (SERVER*)local;
   CLIENT* c = (CLIENT*)remote;
   PARSER_ISON ison = { 0 };

   ison.clients = bfromcstralloc( 128, "" );
   ison.args = args;
   scaffold_check_null( ison.clients );
   scaffold_check_null( ison.args );

   vector_iterate( &(s->clients), parser_cmp_ison, &ison );
   server_client_printf( s, c, ":%b 303 %b :%b", s->self.remote, c->nick, ison.clients );

cleanup:

   bdestroy( ison.clients );

   return;
}

static void* parser_cat_names( VECTOR* v, size_t idx, void* iter, void* arg ) {
   CLIENT* c = (CLIENT*)iter;
   bstring names = (bstring)arg;
   bconcat( names, c->nick );
   if( vector_count( v ) - 1 != idx ) {
      bconchar( names, ' ' );
   };
   return NULL;
}

#if 0
static void parser_tmap_chunk_cb( CHUNKER* h, ssize_t socket ) {
   PARSER_TRIO* trio = (PARSER_TRIO*)h->cb_arg;

   /* TODO: How to tell if the client doesn't exist without reffing it? */
   //assert( 0 != trio->c->sentinal );
   if( FALSE == mailbox_is_alive( h->mailqueue, socket ) ) {
      scaffold_print_debug(
         "Client with socket %d no longer connected. Stopping chunk job.\n",
         socket
      );
      h->status = CHUNKER_STATUS_DELETE;
      goto cleanup;
   }

// XXX
   server_client_printf(
      trio->s, trio->c, ":%b GDB %b %b TILEMAP %b %d %d : %b",
      trio->s->self.remote, trio->c->nick, trio->l->name, h->filename,
      h->progress, h->src_len, h->dest_buffer
   );

   btrunc( h->dest_buffer, 0 );

   if( CHUNKER_STATUS_FINISHED == h->status || CHUNKER_STATUS_ERROR == h->status ) {
      free( trio );
      scaffold_print_debug( "Parser map chunker transmission complete.\n" );
      h->status = CHUNKER_STATUS_DELETE;
   }

cleanup:
   return;
}
#endif

static void parser_server_join( void* local, void* remote,
                                struct bstrList* args ) {
   SERVER* s = (SERVER*)local;
   CLIENT* c = (CLIENT*)remote;
   CHANNEL* l = NULL;
   bstring namehunt = NULL;
   int8_t bstr_result = 0;
   bstring names = NULL;
   //bstring map_serial = NULL;
   //struct bstrList* map_serial_list = NULL;
   PARSER_TRIO* chunker_trio = NULL;
   //CHUNKER* h = NULL;
   heatshrink_encoder* h = NULL;
   HSE_sink_res hse_res;
   HSE_poll_res hsp_res;

   if( 2 > args->qty ) {
      server_client_printf(
         s, c, ":%b 461 %b %b :Not enough parameters",
         s->self.remote, c->username, args->entry[0]
      );
      goto cleanup;
   }

   namehunt = bstrcpy( args->entry[1] );
   bstr_result = btrimws( namehunt );
   scaffold_check_nonzero( bstr_result );

   if( TRUE != scaffold_string_is_printable( namehunt ) ) {
      server_client_printf(
         s, c, ":%b 403 %b %b :No such channel",
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
   server_client_printf(
      s, c, ":%b!%b@%b JOIN %b",
      c->nick, c->username, c->remote, l->name
   );

   names = bfromcstr( "" );
   scaffold_check_null( names );
   vector_iterate( &(l->clients), parser_cat_names, names );

   server_client_printf(
      s, c, ":%b 332 %b %b :%b",
      s->self.remote, c->nick, l->name, l->topic
   );

   server_client_printf(
      s, c, ":%b 353 %b = %b :%b",
      s->self.remote, c->nick, l->name, names
   );

   server_client_printf(
      s, c, ":%b 366 %b %b :End of NAMES list",
      s->self.remote, c->nick, l->name
   );

   scaffold_check_null( l->gamedata.tmap.serialize_buffer );

   chunker_trio = (PARSER_TRIO*)calloc( 1, sizeof( PARSER_TRIO ) );
   chunker_trio->c = c;
   chunker_trio->l = l;
   chunker_trio->s = s;

   assert( NULL != l->gamedata.tmap.serialize_buffer );
   assert( 0 < blength( l->gamedata.tmap.serialize_buffer ) );
   //assert( 0 < c->jobs_socket );

#if 0
   chunker_new( h, -1, -1 );
   chunker_set_cb( h, parser_tmap_chunk_cb, s->self.jobs, chunker_trio );
   chunker_chunk(
      h,
      c->jobs_socket,
      l->gamedata.tmap.serialize_filename,
      (BYTE*)bdata( l->gamedata.tmap.serialize_buffer ),
      blength( l->gamedata.tmap.serialize_buffer )
   );

#endif

   if( NULL != c->chunker.encoder ) {
      /* TODO: What if the client has a chunker already? */
   }
   h = heatshrink_encoder_alloc(
      PARSER_HS_WINDOW_SIZE,
      PARSER_HS_LOOKAHEAD_SIZE
   );
   scaffold_check_null( h );
   c->chunker.encoder = h;
   c->chunker.pos = 0;
   c->chunker.foreign_buffer = l->gamedata.tmap.serialize_buffer;
   c->chunker.filename = l->gamedata.tmap.serialize_filename;

   assert( vector_count( &(c->channels) ) > 0 );
   assert( vector_count( &(s->self.channels) ) > 0 );

cleanup:
   //bstrListDestroy( map_serial_list );
   //bdestroy( map_serial );
   bdestroy( names );
   bdestroy( namehunt );
   return;
}

static void parser_server_part( void* local, void* remote,
                                struct bstrList* args ) {
}

static void parser_server_privmsg( void* local, void* remote,
                                   struct bstrList* args ) {
   SERVER* s = (SERVER*)local;
   CLIENT* c = (CLIENT*)remote;
   CLIENT* c_dest = NULL;
   CHANNEL* l_dest = NULL;
   bstring msg = NULL;

   //bdestroy( scaffold_pop_string( args ) );
   msg = bjoin( args, &scaffold_space_string );

   c_dest = server_get_client_by_nick( s, args->entry[1] );
   if( NULL != c_dest ) {
      server_client_printf(
         s, c_dest, ":%b!%b@%b %b", c->nick, c->username, c->remote, msg
      );
      goto cleanup;
   }

   /* Maybe it's for a channel, instead? */
   l_dest = client_get_channel_by_name( &(s->self), args->entry[1] );
   if( NULL != l_dest ) {
      server_channel_printf(
         s, l_dest, c, ":%b!%b@%b %b", c->nick, c->username, c->remote, msg
      );
      goto cleanup;
   }

   /* TODO: Handle error? */

cleanup:
   bdestroy( msg );
   return;
}

static void* parser_prn_who( VECTOR* v, size_t idx, void* iter, void* arg ) {
   PARSER_TRIO* trio = (PARSER_TRIO*)arg;
   CLIENT* c_iter = (CLIENT*)iter;
   server_client_printf(
      trio->s, trio->c, ":%b RPL_WHOREPLY %b %b",
      trio->s->self.remote, c_iter->nick, trio->l->name
   );
   return NULL;
}

static void parser_server_who( void* local, void* remote,
                               struct bstrList* args ) {
   SERVER* s = (SERVER*)local;
   CLIENT* c = (CLIENT*)remote;
   CHANNEL* l = NULL;
   PARSER_TRIO trio = { 0 };

   l = client_get_channel_by_name( &(s->self), args->entry[1] );
   scaffold_check_null( l );

   trio.c = c;
   trio.l = l;
   trio.s = s;

   vector_iterate( &(l->clients), parser_prn_who, &trio );

cleanup:
   return;
}

static void parser_server_ping( void* local, void* remote,
                               struct bstrList* args ) {
   SERVER* s = (SERVER*)local;
   CLIENT* c = (CLIENT*)remote;

   if( 2 > args->qty ) {
      goto cleanup;
   }

   server_client_printf(
      s, c, ":%b PONG %b :%b", s->self.remote, s->self.remote, c->remote
   );

cleanup:
   return;
}

/* GU #channel px+1y+2z+3 */
static void parser_server_gu( void* local, void* remote,
                              struct bstrList* args ) {
   SERVER* s = (SERVER*)local;
   CLIENT* c = (CLIENT*)remote;
   CHANNEL* l = NULL;
   bstring reply_c = NULL;
   bstring reply_l = NULL;
   struct bstrList gu_args;

   /* Strip off the command "header". */
   memcpy( &gu_args, args, sizeof( struct bstrList ) );
   scaffold_pop_string( &gu_args ); /* Source */
   scaffold_pop_string( &gu_args ); /* Channel */

   /* Find out if this command affects a certain channel. */
   if( 2 <= args->qty && '#' == bdata( args->entry[1] )[0] ) {
      l = client_get_channel_by_name( &(s->self), args->entry[1] );
      scaffold_check_null( l );
   }

   gamedata_update_server( &(l->gamedata), c, &gu_args, &reply_c, &reply_l );

   /* TODO: Make sure client is actually in the channel requested. */
   c = channel_get_client_by_name( l, c->nick );
   scaffold_check_null( c );

   if( NULL != reply_c ) {
      server_client_printf( s, c, "%b", reply_c );
   }

   if( NULL != reply_l ) {
      server_channel_printf( s, l, c, "%b", reply_l );
   }

cleanup:
   bdestroy( reply_l );
   bdestroy( reply_c );
   return;
}

static void parser_client_gu( void* local, void* gamedata,
                              struct bstrList* args ) {
   CLIENT* c = (CLIENT*)local;
   GAMEDATA* d = (GAMEDATA*)gamedata;
   bstring reply = NULL;
   struct bstrList gu_args;

   memcpy( &gu_args, args, sizeof( struct bstrList ) );

#if 0
   /* Strip off the command "header". */
   scaffold_pop_string( &gu_args ); /* Source */
   scaffold_pop_string( &gu_args ); /* Channel */

   /* Find out if this command affects a certain channel. */
   if( 2 <= args->qty && '#' == bdata( args->entry[1] )[0] ) {
      l = server_get_channel_by_name( s, args->entry[1] );
      scaffold_check_null( l );
   }
#endif

   /* TODO: Modify gamedata based on new information. */
   gamedata_react_client( d, c, &gu_args, &reply );

   if( NULL != reply ) {
      client_printf( c, "%b", reply );
   }

   bdestroy( reply );
   return;
}

static void parser_client_join( void* local, void* gamedata,
                                struct bstrList* args ) {
   CLIENT* c = (CLIENT*)local;
   CHANNEL* l = NULL;

   scaffold_check_bounds( 3, args->mlen );

   /* Get the channel, or create it if it does not exist. */
   l = client_get_channel_by_name( c, args->entry[2] );
   if( NULL == l ) {
      channel_new( l, args->entry[2] );
      client_add_channel( c, l );
      scaffold_print_info( "Client created local channel mirror: %s\n",
                           bdata( args->entry[2] ) );
   }

   scaffold_print_info( "Client joined channel: %s\n", bdata( args->entry[2] ) );

   assert( vector_count( &(c->channels) ) > 0 );

cleanup:
   return;
}

static const struct tagbstring str_closing = bsStatic( ":Closing" );

static void parser_client_error( void* local, void* gamedata,
                                struct bstrList* args ) {
   CLIENT* c = (CLIENT*)local;

   if(
      2 <= args->qty &&
      0 == bstrcmp( &str_closing, args->entry[1] )
   ) {
      c->running = FALSE;
   }
}

static void parser_client_gdb( void* local, void* gamedata,
                                struct bstrList* args ) {
   GAMEDATA* d = (GAMEDATA*)gamedata;
   //CHUNKER* h = NULL;
   heatshrink_decoder* h = NULL;
   int hash_ret;
   size_t progress = 0;
   size_t total = 0;
   size_t consumed = 0;
   HSD_sink_res hsd_res;
   HSD_poll_res hsp_res;
   bstring data = NULL;
   bstring filename = NULL;
   uint8_t outbuffer[PARSER_FILE_XMIT_BUFFER] = { 0 };

   assert( 9 == args->qty );

   filename = args->entry[4];
   progress = atoi( bdata( args->entry[5] ) );
   total = atoi( bdata( args->entry[6] ) );
   data = args->entry[8];

   if( progress > total ) {
      scaffold_print_error( "Invalid progress for %s.\n", bdata( filename ) );
      scaffold_error = SCAFFOLD_ERROR_MISC;
      goto cleanup;
   }

   if( 0 < d->incoming_buffer_len && d->incoming_buffer_len != total ) {
      scaffold_print_error( "Invalid total for %s.\n", bdata( filename ) );
      scaffold_error = SCAFFOLD_ERROR_MISC;
      goto cleanup;
   }

   hashmap_get( d->incoming_chunkers, filename, (void**)(&h) );
   if( NULL == h ) {
      //chunker_new( h, total, (total - progress) );
      h = heatshrink_decoder_alloc(
         PARSER_FILE_XMIT_BUFFER,
         PARSER_HS_WINDOW_SIZE,
         PARSER_HS_LOOKAHEAD_SIZE
      );
      hash_ret = hashmap_put( d->incoming_chunkers, filename, h );
      scaffold_check_nonzero( hash_ret );
      d->incoming_buffer_len = total;
   }

   hsd_res = heatshrink_decoder_sink(
      h, (uint8_t*)bdata( data ), (size_t)blength( data ), &consumed
   );
   assert( HSDR_SINK_ERROR_NULL != hsd_res );

   while( HSDR_POLL_MORE == (hsp_res = heatshrink_decoder_poll(
      h, outbuffer, PARSER_FILE_XMIT_BUFFER, &consumed
   )) ) {
      memcpy( &(d->incoming_buffer[progress]), outbuffer, consumed );
      progress += consumed;
   }
   //chunker_unchunk( h, filename, data, progress );
   scaffold_print_debug( "%d out of %d\n", progress, total );


#if 0
   if( progress < total ) {

   } else {
      /* Download complete, so deserialize. */
   }
#endif

   //scaffold_print_debug( "INCOMING DATA %s OF %s: %s\n", bdata( progress ), bdata( total ), bdata( data ) );
cleanup:
   return;
}

const parser_entry parser_table_server[] = {
   {bsStatic( "USER" ), parser_server_user},
   {bsStatic( "NICK" ), parser_server_nick},
   {bsStatic( "QUIT" ), parser_server_quit},
   {bsStatic( "ISON" ), parser_server_ison},
   {bsStatic( "JOIN" ), parser_server_join},
   {bsStatic( "PART" ), parser_server_part},
   {bsStatic( "PRIVMSG" ), parser_server_privmsg},
   {bsStatic( "WHO" ), parser_server_who},
   {bsStatic( "PING" ), parser_server_ping},
   {bsStatic( "GU" ), parser_server_gu },
   {bsStatic( "" ), NULL}
};

const parser_entry parser_table_client[] = {
   {bsStatic( "GU" ), parser_client_gu },
   {bsStatic( "JOIN" ), parser_client_join },
   {bsStatic( "ERROR" ), parser_client_error },
   {bsStatic( "GDB" ), parser_client_gdb },
   {bsStatic( "" ), NULL}
};

void parser_dispatch( void* local, void* arg2, const_bstring line ) {
   SERVER* s_local = (SERVER*)local;
   const parser_entry* parser_table = NULL;
   struct bstrList* args = NULL;
   const parser_entry* command = NULL;
   size_t i;
   bstring cmd_test = NULL; /* Don't free this. */

   if( SERVER_SENTINAL == s_local->self.sentinal ) {
      parser_table = parser_table_server;
      //arg_command_index = 0;
   } else {
      parser_table = parser_table_client;
      //arg_command_index = 1; /* First index is just the server name. */
   }

   args = bsplit( line, ' ' );
   scaffold_check_null( args );
   //scaffold_check_bounds( (arg_command_index + 1), args->mlen );

   for( i = 0 ; i < args->qty && PARSER_CMD_SEARCH_RANGE > i ; i++ ) {
      cmd_test = args->entry[i];
      for(
         command = &(parser_table[0]);
         NULL != command->callback;
         command++
      ) {
         if( 0 == bstrncmp(
            cmd_test, &(command->command), blength( &(command->command) )
         ) ) {
#ifdef DEBUG
            if( 0 != bstrncmp( cmd_test, &(parser_table_client[3].command), 3 ) ) {
               scaffold_print_debug( "Parse: %s\n", bdata( line ) );
            }
#endif /* DEBUG */
            command->callback( local, arg2, args );

            /* Found a command, so short-circuit. */
            goto cleanup;
         }
      }
   }

   scaffold_print_error( "Parser unable to interpret: %s\n", bdata( line ) );

cleanup:

   bstrListDestroy( args );
}
