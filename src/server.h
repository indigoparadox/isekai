#ifndef SERVER_H
#define SERVER_H

#include "libvcol.h"
#include "client.h"

struct CLIENT;
struct CHANNEL;
struct SERVER;

#define SERVER_SENTINAL 164641

#define SERVER_RANDOM_NICK_LEN 10

void server_free_clients( struct SERVER* s );
struct SERVER* server_new();
void server_init( struct SERVER* s, const bstring myhost );
void server_stop_clients( struct SERVER* s );
#ifdef USE_INLINES
inline
#endif /* USE_INLINES */
void server_stop( struct SERVER* s );
bool server_free( struct SERVER* s );
void server_channel_printf( struct SERVER* s, struct CHANNEL* l, struct CLIENT* c_skip, const char* message, ... );
void server_channel_send( struct SERVER* s, struct CHANNEL* l, struct CLIENT* c_skip, bstring buffer );
short server_add_client( struct SERVER* s, struct CLIENT* n );
struct CHANNEL* server_add_channel( struct SERVER* s, bstring l_name, struct CLIENT* c_first );
void server_add_connection( struct SERVER* s, struct CLIENT* n );
struct CHANNEL* server_get_channel_by_name( struct SERVER* s, bstring nick );
void server_channel_add_client( struct CHANNEL* l, struct CLIENT* c );
uint16_t server_get_port( struct SERVER* s );
struct CLIENT* server_get_client( struct SERVER* s, const bstring nick );
struct CLIENT* server_get_client_by_ptr( struct SERVER* s, struct CLIENT* c );
void server_cleanup_client_channels( struct SERVER* s, struct CLIENT* c );
void server_drop_client( struct SERVER* s, bstring nick );
bool server_listen( struct SERVER* s, int port );
bool server_poll_new_clients( struct SERVER* s );
bool server_service_clients( struct SERVER* s );
void server_set_client_nick( struct SERVER* s, struct CLIENT* c, const bstring nick );
bstring server_file_search( bstring search_filename );
bool server_is_running( struct SERVER* s );
bool server_is_listening( struct SERVER* s );
bstring server_get_remote( struct SERVER* s );
size_t server_get_client_count( struct SERVER* s );
struct VECTOR* server_get_clients_online( struct SERVER* s, struct VECTOR* filter );
size_t server_get_channels_count( struct SERVER* s );

#ifndef SERVER_C
extern struct tagbstring str_server_data_path;
#endif /* !SERVER_C */

#endif /* SERVER_H */
