#include "server.h"

#include <stdlib.h>

#include "parser.h"

void server_init( SERVER* s, const bstring myhost ) {
   client_init( &(s->self) );
   vector_init( &(s->clients) );
   s->self.remote = bstrcpy( myhost );
   s->servername =  blk2bstr( bsStaticBlkParms( "ProCIRCd" ) );
   s->version = blk2bstr(  bsStaticBlkParms( "0.1" ) );
   s->self.sentinal = SERVER_SENTINAL;
}

inline void server_stop( SERVER* s ) {
   s->self.running = FALSE;
}

void server_cleanup( SERVER* s ) {
   int i;
   CLIENT* c;

   /* Remove clients. */
   for( i = 0 ; vector_count( &(s->clients) ) > i ; i++ ) {
      c = (CLIENT*)vector_get( &(s->clients), i );
      server_cleanup_client_channels( s, c );
      client_cleanup( c );
   }
   vector_free( &(s->clients) );

   /* Don't remove channels, since those will be removed as clients are. */

   client_cleanup( &(s->self) );
}

void server_add_client( SERVER* s, CLIENT* n ) {
   server_lock_clients( s, TRUE );
   vector_add( &(s->clients), n );
   server_lock_clients( s, FALSE );
}

CLIENT* server_get_client( SERVER* s, int index ) {
   CLIENT* c;
   server_lock_clients( s, TRUE );
   c = vector_get( &(s->clients), index );
   server_lock_clients( s, FALSE );
   return c;
}

static int server_get_client_index_by_socket( SERVER* s, int socket,
      BOOL lock ) {
   CLIENT* c = NULL;
   int i;

   if( lock ) {
      server_lock_clients( s, TRUE );
   }
   for( i = 0 ; vector_count( &(s->clients) ) > i ; i++ ) {
      c = vector_get( &(s->clients), i );
      if( socket == c->link.socket ) {
         /* Skip the reset below. */
         goto cleanup;
      }
   }
   i = -1;

cleanup:

   if( lock ) {
      server_lock_clients( s, FALSE );
   }

   return i;
}

CLIENT* server_get_client_by_nick( SERVER* s, const bstring nick, BOOL lock ) {
   CLIENT* c = NULL;
   int i;

   if( lock ) {
      server_lock_clients( s, TRUE );
   }

   for( i = 0 ; vector_count( &(s->clients) ) > i ; i++ ) {
      c = vector_get( &(s->clients), i );
      if( 0 == bstrcmp( c->nick, nick ) ) {
         /* Skip the reset below. */
         goto cleanup;
      }
   }

   c = NULL;

cleanup:

   if( lock ) {
      server_lock_clients( s, FALSE );
   }

   return c;
}

void server_cleanup_client_channels( SERVER* s, CLIENT* c ) {
   int i, j;
   CHANNEL* l_iter;

   for( i = 0 ; vector_count( &(c->channels) ) > i ; i++ ) {
      l_iter = vector_get( &(c->channels ), i );
      /* Cleanup channel if it has no other clients. */
      if( 1 >= vector_count( &(l_iter->clients) ) ) {
         scaffold_print_debug(
            "Channel %s has no clients. Deleting.\n", bdata( l_iter->name )
         );

         if( NULL != s ) {
            for( j = 0 ; vector_count( &(s->self.channels) ) > j ; j++ ) {
               if( 0 == bstrcmp( (((CHANNEL*)vector_get( &(s->self.channels), j ))->name),
                                 l_iter->name ) ) {
                  vector_delete( &(s->self.channels), j );
               }
            }
         }

         channel_cleanup( l_iter );
      }
   }
}

void server_drop_client( SERVER* s, int socket ) {
   CLIENT* c;
   int index;

   server_lock_clients( s, TRUE );
   index = server_get_client_index_by_socket( s, socket, FALSE );
   c = vector_get( &(s->clients), index );
   server_cleanup_client_channels( s, c );
   client_cleanup( c );
   vector_delete( &(s->clients), index );
   server_lock_clients( s, FALSE );
}

void server_listen( SERVER* s, int port ) {
   s->self.link.arg = s;
   connection_listen( &(s->self.link), port );
   if( SCAFFOLD_ERROR_NEGATIVE == scaffold_error ) {
      scaffold_print_error( "Unable to bind to specified port. Exiting.\n" );
   }
}

void server_service_clients( SERVER* s ) {
   ssize_t last_read_count = 0;
   CLIENT* c = NULL;
   CONNECTION* n_client = NULL;
   int i = 0;

   /* Check for new clients. */
   n_client = connection_register_incoming( &(s->self.link) );
   if( NULL != n_client ) {
      client_new( c );
      memcpy( &(c->link), n_client, sizeof( CONNECTION ) );
      free( n_client ); /* Don't clean up, because data is still valid. */

      /* Add some details to c before stowing it. */
      connection_assign_remote_name( &(c->link), c->remote );

      server_add_client( s, c );
   }

   /* Check for commands from existing clients. */
   for( i = 0 ; vector_count( &(s->clients) ) > i ; i++ ) {
      c = vector_get( &(s->clients), i );

      btrunc( s->self.buffer, 0 );
      last_read_count = connection_read_line( &(c->link), s->self.buffer );
      btrimws( s->self.buffer );

      if( 0 >= last_read_count ) {
         /* TODO: Handle error reading. */
         continue;
      }

      scaffold_print_debug(
         "Server: Line received from %d: %s\n",
         c->link.socket, bdata( s->self.buffer )
      );

      parser_dispatch( s, c, s->self.buffer );
   }

cleanup:

   return;
}

/* Returns 0 if successful or IRC numeric error otherwise. */
int server_set_client_nick( SERVER* s, CLIENT* c, const bstring nick ) {
   int retval = 0;
   int bstr_result = 0;

   if( NULL == nick ) {
      retval = ERR_NONICKNAMEGIVEN;
      goto cleanup;
   }

   if( NULL != server_get_client_by_nick( s, nick, TRUE ) ) {
      retval = ERR_NICKNAMEINUSE;
      goto cleanup;
   }

   bstr_result = bassign( c->nick, nick );
   scaffold_check_nonzero( bstr_result );

cleanup:

   return retval;
}

void server_lock_clients( SERVER* s, BOOL locked ) {
}
