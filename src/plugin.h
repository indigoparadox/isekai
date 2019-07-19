
#ifndef PLUGIN_H
#define PLUGIN_H

struct CLIENT;
struct MOBILE;
struct CHANNEL;
struct ACTION_PACKET;
struct INPUT;

typedef enum {
   PLUGIN_SUCCESS,
   PLUGIN_FAILURE,
   PLUGIN_PARTIAL
} PLUGIN_RESULT;

typedef enum PLUGIN_CALL {
   PLUGIN_INIT,
   PLUGIN_DRAW,
   PLUGIN_UPDATE,
   PLUGIN_POLL_INPUT,
   PLUGIN_FREE,
   PLUGIN_MOBILE_ACTION_SERVER,
   PLUGIN_MOBILE_ACTION_CLIENT,
   PLUGIN_MOBILE_INIT,
   PLUGIN_CLIENT_INIT,
   PLUGIN_MOBILE_FREE,
   PLUGIN_CLIENT_FREE
} PLUGIN_CALL;

typedef enum {
   PLUGIN_MODE
} PLUGIN_TYPE;

PLUGIN_RESULT plugin_load_all( PLUGIN_TYPE ptype );
PLUGIN_RESULT plugin_load( PLUGIN_TYPE ptype, bstring plugin_name );
PLUGIN_RESULT plugin_call_all( PLUGIN_TYPE ptype, PLUGIN_CALL hook, ... );
PLUGIN_RESULT plugin_unload_all( PLUGIN_TYPE ptype );
PLUGIN_RESULT plugin_unload( PLUGIN_TYPE ptype, bstring plugin_name );
PLUGIN_RESULT plugin_call(
   PLUGIN_TYPE ptype, const bstring plug, PLUGIN_CALL hook, ... );
bstring plugin_get_mode_short( int mode );
bstring plugin_get_mode_name( int mode );
int plugin_count();
struct VECTOR* plugin_get_mode_name_list();

#ifdef PLUGIN_C

#ifndef USE_DYNAMIC_PLUGINS
PLUGIN_RESULT mode_topdown_init();
PLUGIN_RESULT mode_topdown_draw(
   struct CLIENT* c,
   struct CHANNEL* l
);
PLUGIN_RESULT mode_topdown_update(
   struct CLIENT* c,
   struct CHANNEL* l
);
PLUGIN_RESULT mode_topdown_poll_input(
   struct CLIENT* c, struct CHANNEL* l, struct INPUT* p
);
PLUGIN_RESULT mode_topdown_free( struct CLIENT* c );
PLUGIN_RESULT mode_topdown_mobile_action_client( struct ACTION_PACKET* update );
PLUGIN_RESULT mode_topdown_mobile_action_server( struct ACTION_PACKET* update );
PLUGIN_RESULT mode_topdown_mobile_init( struct MOBILE* o, struct CHANNEL* l );
PLUGIN_RESULT mode_topdown_mobile_free( struct MOBILE* o );
PLUGIN_RESULT mode_topdown_client_init( struct CLIENT* c, struct CHANNEL* l );
PLUGIN_RESULT mode_topdown_client_free( struct CLIENT* c );

/* --- */

PLUGIN_RESULT mode_pov_init();
PLUGIN_RESULT mode_pov_draw(
   struct CLIENT* c,
   struct CHANNEL* l
);
PLUGIN_RESULT mode_pov_update(
   struct CLIENT* c,
   struct CHANNEL* l
);
PLUGIN_RESULT mode_pov_poll_input(
   struct CLIENT* c, struct CHANNEL* l, struct INPUT* p
);
PLUGIN_RESULT mode_pov_free( struct CLIENT* c );
PLUGIN_RESULT mode_pov_mobile_action_client( struct ACTION_PACKET* update );
PLUGIN_RESULT mode_pov_mobile_action_server( struct ACTION_PACKET* update );
PLUGIN_RESULT mode_pov_mobile_init( struct MOBILE* o, struct CHANNEL* l );
PLUGIN_RESULT mode_pov_mobile_free( struct MOBILE* o );
PLUGIN_RESULT mode_pov_client_init( struct CLIENT* c, struct CHANNEL* l );
PLUGIN_RESULT mode_pov_client_free( struct CLIENT* c );

#endif /* !USE_DYNAMIC_PLUGINS */

struct VECTOR* mode_list_pretty;
struct VECTOR* mode_list_short;

static struct HASHMAP* plugin_list_mode;
#endif

#endif /* PLUGIN_H */
