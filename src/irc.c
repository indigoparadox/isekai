
#define IRC_C
#include "irc.h"

#include "server.h"
#include "callbacks.h"
#include "chunker.h"
#include "datafile.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* This file contains our (possibly limited, slightly incompatible) version *
 * of the IRC protocol, as it interacts with our server and client objects. oopen game datapen game data*/

static void irc_server_reply_welcome( CLIENT* c, SERVER* s ) {

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
      s->self.remote, c->nick, hashmap_count( &(s->clients) )
   );

   c->flags |= CLIENT_FLAGS_HAVE_WELCOME;
}

static void irc_server_reply_nick( CLIENT* c, SERVER* s, bstring oldnick ) {
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

void irc_server_reply_motd( CLIENT* c, SERVER* s ) {
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

static void irc_server_reply_gdb_tilemap( CLIENT* c, SERVER* s, CHANNEL* l ) {
   client_send_file( c, l->name, l->gamedata.tmap.serialize_filename );
   /* tilesets > tileset > image */
   //vector_lock( )
   //client_send_file( c, l->gamedata.tmap.tilesets );

cleanup:
   return;
}

static void irc_server_user( CLIENT* c, SERVER* s, struct bstrList* args ) {
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

#if 0
// FIXME
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
#endif
   //scaffold_print_debug( "User: %s, Real: %s, Remote: %s\n", bdata( c->username ), bdata( c->realname ), bdata( c->remote ) );

   c->flags |= CLIENT_FLAGS_HAVE_USER;

   if( ( c->flags & CLIENT_FLAGS_HAVE_NICK ) ) {
      // FIXME: irc_server_reply_welcome( local, remote );
   }

cleanup:
   return;
}

static void irc_server_nick( CLIENT* c, SERVER* s, struct bstrList* args ) {
   bstring oldnick = NULL;
   bstring newnick = NULL;
   int bstr_result = 0;

   if( 2 >= args->qty ) {
      newnick = args->entry[1];
   }

   server_set_client_nick( s, c, newnick );
   if( SCAFFOLD_ERROR_NULLPO == scaffold_error ) {
      server_client_printf(
         s, c, ":%b 431 %b :No nickname given",
         s->self.remote, c->nick
      );
      goto cleanup;
   }

   if( SCAFFOLD_ERROR_NOT_NULLPO == scaffold_error ) {
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
      irc_server_reply_welcome( c, s );
   }

   irc_server_reply_nick( c, s, oldnick );

cleanup:

   bdestroy( oldnick );

   return;
}

static void irc_server_quit( CLIENT* c, SERVER* s, struct bstrList* args ) {
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

static void irc_server_ison( CLIENT* c, SERVER* s, struct bstrList* args ) {
   VECTOR* ison = NULL;
   bstring response = NULL;
   CLIENT* c_iter = NULL;
   int i;

   response = bfromcstralloc( 80, "" );
   scaffold_check_null( response );

   /* TODO: Root the command stuff out of args. */
   ison = hashmap_iterate_v( &(s->clients), callback_search_clients_l, args );
   scaffold_check_null( ison );
   vector_lock( ison, TRUE );
   for( i = 0 ; vector_count( ison ) > i ; i++ ) {
      c_iter = (CLIENT*)vector_get( ison, i );
      /* 1 for the main list + 1 for the vector. */
      assert( 2 <= c_iter->link.refcount.count );
      bconcat( response, c_iter->nick );
      bconchar( response, ' ' );
   }
   vector_lock( ison, FALSE );
   scaffold_check_null( response );

   server_client_printf( s, c, ":%b 303 %b :%b", s->self.remote, c->nick, response );

cleanup:
   if( NULL != ison ) {
      vector_remove_cb( ison, callback_free_clients, NULL );
      vector_free( ison );
      free( ison );
   }
   bdestroy( response );
   return;
}

static void irc_server_join( CLIENT* c, SERVER* s, struct bstrList* args ) {
   CHANNEL* l = NULL;
   bstring namehunt = NULL;
   int8_t bstr_result = 0;
   bstring names = NULL;
   struct bstrList* cat_names = NULL;

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

   cat_names = bstrListCreate();
   scaffold_check_null( cat_names );
   hashmap_iterate( &(l->clients), callback_concat_clients, cat_names );
   names = bjoinblk( cat_names, " ", 1 );

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

   assert( hashmap_count( &(c->channels) ) > 0 );
   assert( hashmap_count( &(s->self.channels) ) > 0 );

cleanup:
   if( NULL != cat_names ) {
      bstrListDestroy( cat_names );
   }
   bdestroy( names );
   bdestroy( namehunt );
   return;
}

static void irc_server_part( CLIENT* c, SERVER* s, struct bstrList* args ) {
}

static void irc_server_privmsg( CLIENT* c, SERVER* s, struct bstrList* args ) {
   CLIENT* c_dest = NULL;
   CHANNEL* l_dest = NULL;
   bstring msg = NULL;

   //bdestroy( scaffold_pop_string( args ) );
   msg = bjoin( args, &scaffold_space_string );

   c_dest = server_get_client( s, args->entry[1] );
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

/*
static void* irc_prn_who( VECTOR* v, size_t idx, void* iter, void* arg ) {
   PARSER_TRIO* trio = (PARSER_TRIO*)arg;
   CLIENT* c_iter = (CLIENT*)iter;
   server_client_printf(
      trio->s, trio->c, ":%b RPL_WHOREPLY %b %b",
      trio->s->self.remote, c_iter->nick, trio->l->name
   );
   return NULL;
}
*/

static void irc_server_who( CLIENT* c, SERVER* s, struct bstrList* args ) {
   CHANNEL* l = NULL;
   struct bstrList* search_targets = NULL;
   VECTOR* ison = NULL;
   CLIENT* c_iter = NULL;
   size_t i;
   bstring response = NULL;

   if( 2 > args->qty ) {
      scaffold_print_error( "Server: Malformed WHO expression received.\n" );
      scaffold_error = SCAFFOLD_ERROR_MISC;
      goto cleanup;
   }

   /* TODO: Handle non-channels. */
   if( '#' != args->entry[1]->data[0] ) {
      scaffold_print_error( "Non-channel WHO not yet implemented.\n" );
      goto cleanup;
   }

   l = client_get_channel_by_name( &(s->self), args->entry[1] );
   scaffold_check_null( l );

   response = bfromcstralloc( 80, "" );
   scaffold_check_null( response );

   search_targets = bstrListCreate();
   scaffold_check_null( search_targets );

   ison = hashmap_iterate_v( &(l->clients), callback_search_clients, NULL );
   scaffold_check_null( ison );
   vector_lock( ison, TRUE );
   for( i = 0 ; vector_count( ison ) > i ; i++ ) {
      c_iter = (CLIENT*)vector_get( ison, i );
      /* 1 for the main list + 1 for the vector. */
      assert( 2 <= c_iter->link.refcount.count );
      //bconcat( response, c_iter->nick );
      //bconchar( response, ' ' );

      server_client_printf(
         s, c, ":%b RPL_WHOREPLY %b %b",
         s->self.remote, c->nick, l->name
      );
   }
   vector_lock( ison, FALSE );
   scaffold_check_null( response );

   //vector_iterate( &(l->clients), irc_prn_who, &trio );
   //server_client_printf( s, c, ":%b 303 %b :%b", s->self.remote, c->nick, response );


   /* Also send the tilemap data if we haven't yet, since we know the client  *
    * is ready, now.                                                          */
   /* TODO: Detect if client doesn't support our extensions. */
   if( !(c->flags & CLIENT_FLAGS_HAVE_TILEMAP) ) {
      irc_server_reply_gdb_tilemap( c, s, l );
      c->flags |= CLIENT_FLAGS_HAVE_TILEMAP;
   }

cleanup:
   if( NULL != ison ) {
      vector_remove_cb( ison, callback_free_clients, NULL );
      vector_free( ison );
      free( ison );
   }
   bdestroy( response );
   bstrListDestroy( search_targets );
   return;
}

static void irc_server_ping( CLIENT* c, SERVER* s, struct bstrList* args ) {

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
static void irc_server_gu( CLIENT* c, SERVER* s, struct bstrList* args ) {
   CHANNEL* l = NULL;
   bstring reply_c = NULL;
   bstring reply_l = NULL;
   struct bstrList gu_args;

   /* Strip off the command "header". */
   memcpy( &gu_args, args, sizeof( struct bstrList ) );
   // FIXME
   //scaffold_pop_string( &gu_args ); /* Source */
   //scaffold_pop_string( &gu_args ); /* Channel */

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

static void irc_client_gu( CLIENT* c, SERVER* s, struct bstrList* args ) {
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
//   gamedata_react_client( d, c, &gu_args, &reply );

   if( NULL != reply ) {
      client_printf( c, "%b", reply );
   }

   bdestroy( reply );
   return;
}

static void irc_client_join( CLIENT* c, SERVER* s, struct bstrList* args ) {
   CHANNEL* l = NULL;
   bstring l_name = NULL;

   assert( SCAFFOLD_TRACE_CLIENT == scaffold_trace_path );

   scaffold_check_bounds( 3, args->qty );
   l_name = args->entry[3];

   /* Get the channel, or create it if it does not exist. */
   l = client_get_channel_by_name( c, l_name );
   if( NULL == l ) {
      channel_new( l, l_name );
      gamedata_init_client( &(l->gamedata) );
      client_add_channel( c, l );
      scaffold_print_info( "Client created local channel mirror: %s\n",
                           bdata( l_name ) );
   }

   scaffold_print_info( "Client joined channel: %s\n", bdata( l_name ) );

   assert( hashmap_count( &(c->channels) ) > 0 );

   client_printf( c, "WHO %b", l->name );

cleanup:
   return;
}

static const struct tagbstring str_closing = bsStatic( ":Closing" );

static void irc_client_error( CLIENT* c, SERVER* s, struct bstrList* args ) {
   if(
      2 <= args->qty &&
      0 == bstrcmp( &str_closing, args->entry[1] )
   ) {
      c->running = FALSE;
   }
}

static void irc_client_gdb( CLIENT* c, SERVER* s, struct bstrList* args ) {
   CHANNEL* l = NULL;
   GAMEDATA* d = NULL;
   CHUNKER* h = NULL;
   size_t progress = 0;
   size_t total = 0;
   size_t length = 0;
   bstring data = NULL;
   bstring filename = NULL;
   const char* progress_c,
      * total_c,
      * length_c;

   if( 11 != args->qty ) {
      scaffold_print_error( "Server: Malformed GDB expression received.\n" );
      scaffold_error = SCAFFOLD_ERROR_MISC;
      goto cleanup;
   }

   l = client_get_channel_by_name( c, args->entry[3] );
   scaffold_check_null( l );

   d = &(l->gamedata);
   scaffold_check_null( d );

   scaffold_check_null( args->entry[6] );
   progress_c = bdata( args->entry[6] );
   scaffold_check_null( progress_c );

   scaffold_check_null( args->entry[7] );
   length_c = bdata( args->entry[7] );
   scaffold_check_null( length_c );

   scaffold_check_null( args->entry[8] );
   total_c = bdata( args->entry[8] );
   scaffold_check_null( total_c );

   filename = args->entry[5];
   scaffold_check_null( filename );

   progress = atoi( progress_c );
   length = atoi( length_c );
   total = atoi( total_c );

   data = args->entry[10];
   scaffold_check_null( data );

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

   h = hashmap_get( &(d->incoming_chunkers), filename );
   if( NULL == h ) {
      h = (CHUNKER*)calloc( 1, sizeof( CHUNKER ) );
      chunker_unchunk_start( h, l->name, CHUNKER_DATA_TYPE_TILEMAP, total );
      hashmap_put( &(d->incoming_chunkers), filename, h );
      scaffold_check_nonzero( scaffold_error );
   }

   chunker_unchunk_pass( h, data, progress, length );

   if( chunker_unchunk_finished( h ) ) {
      datafile_parse_tilemap( &(d->tmap), filename, (BYTE*)h->raw_ptr, h->raw_length );
      scaffold_print_info( "Tilemap for %s successfully loaded into cache.\n", bdata( l->name ) );
   }

cleanup:
   return;
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
IRC_COMMAND_ROW( "GU", irc_server_gu ),
IRC_COMMAND_TABLE_END() };

IRC_COMMAND_TABLE_START( client ) = {
IRC_COMMAND_ROW( "GU", irc_client_gu  ),
IRC_COMMAND_ROW( "366", irc_client_join ),
IRC_COMMAND_ROW( "ERROR", irc_client_error  ),
IRC_COMMAND_ROW( "GDB", irc_client_gdb  ),
IRC_COMMAND_TABLE_END() };

//IRC_COMMAND* irc_dispatch( void* local, void* arg2, const_bstring line ) {

static void irc_command_cleanup( const struct _REF* ref ) {
   IRC_COMMAND* cmd = scaffold_container_of( ref, IRC_COMMAND, refcount );
   int i;

   /* Don't try to free the string or callback. */
   client_free( cmd->client );
   cmd->client = NULL;
   server_free( cmd->server );
   cmd->server = NULL;

   for( i = 0 ; cmd->args->qty > i ; i++ ) {
      bwriteallow( *(cmd->args->entry[i]) );
   }
   bstrListDestroy( cmd->args );
   cmd->args = NULL;
}

void irc_command_free( IRC_COMMAND* cmd ) {
   ref_dec( &(cmd->refcount) );
}

IRC_COMMAND* irc_dispatch(
   const IRC_COMMAND* table, SERVER* s, CLIENT* c, const_bstring line
) {
   struct bstrList* args = NULL;
   const IRC_COMMAND* command = NULL;
   size_t i;
   bstring cmd_test = NULL; /* Don't free this. */
   IRC_COMMAND* out = NULL;

   //irc_cmd_cb* foo = irc_client_gdb;

   args = bsplit( line, ' ' );
   scaffold_check_null( args );

   if( table == irc_table_server ) {
      assert( SCAFFOLD_TRACE_SERVER == scaffold_trace_path );
   } else if( table == irc_table_client ) {
      assert( SCAFFOLD_TRACE_CLIENT == scaffold_trace_path );
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
            if( 0 != bstrncmp( cmd_test, &(irc_table_client[3].command), 3 ) ) {
               if( table == irc_table_server ) {
                  scaffold_print_debug( "Server Parse: %s\n", bdata( line ) );
               } else {
                  scaffold_print_debug( "Client Parse: %s\n", bdata( line ) );
               }
            }
#endif /* DEBUG */

            out = (IRC_COMMAND*)calloc( 1, sizeof( IRC_COMMAND ) );
            scaffold_check_null( out );
            memcpy( out, command, sizeof( IRC_COMMAND ) );
            if( NULL != s ) {
               out->server = s;
               ref_inc( &(s->self.link.refcount) );
            }
            out->client = c;
            ref_inc( &(c->link.refcount) );
            out->args = args;
            ref_init( &(out->refcount), irc_command_cleanup );

            assert( 0 == bstrcmp( &(out->command), &(command->command) ) );

            /* Found a command, so short-circuit. */
            goto cleanup;
         }
      }
   }

   if( table == irc_table_server ) {
      assert( SCAFFOLD_TRACE_SERVER == scaffold_trace_path );
      scaffold_print_error( "Server: Parser unable to interpret: %s\n", bdata( line ) );
   } else if( table == irc_table_client ) {
      assert( SCAFFOLD_TRACE_CLIENT == scaffold_trace_path );
      scaffold_print_error( "Client: Parser unable to interpret: %s\n", bdata( line ) );
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
