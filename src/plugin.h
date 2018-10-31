
#ifndef PLUGIN_H
#define PLUGIN_H

typedef enum {
   PLUGIN_SUCCESS,
   PLUGIN_FAILURE,
   PLUGIN_PARTIAL
} PLUGIN_RESULT;

typedef enum {
   PLUGIN_INIT,
   PLUGIN_DRAW,
   PLUGIN_UPDATE,
   PLUGIN_POLL_INPUT,
   PLUGIN_FREE
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

#endif /* PLUGIN_H */
