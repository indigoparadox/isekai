
struct CLIENT {
   struct REF refcount; /*!< Parent "class". The "root" class is REF. */

   struct CONNECTION* link;

   /* Items shared between server and client. */
   VBOOL running;
   bstring nick;
   bstring username;
   bstring realname;
   bstring remote;
   bstring away;
   bstring mobile_sprite;
   void* protocol_data;
   struct HASHMAP* mode_data; /* Mode > Channel */
   uint16_t flags;
   struct UI* ui;
   struct HASHMAP* channels; /*!< All channels the client is in now, or all
                             *   channels available if this is a server.
                             */

   struct VECTOR* delayed_files; /*!< Requests for files to be executed "later"
                                 *   as a way of getting around locks on
                                 *   resource holders due to dependencies
                                 *   (e.g. tileset needed by tilemap).
                                 *   NOT to be confused with chunkers!
                                 */
   struct MOBILE* puppet;
   struct HASHMAP* sprites; /*!< Contains sprites for all mobiles/items this
                            *   client encounters on client-side. Not used
                            *   server-side.
                            */
   struct HASHMAP* tilesets;
   SCAFFOLD_SIZE tilesets_loaded;
#ifdef USE_ITEMS
   struct HASHMAP item_catalogs;
   struct VECTOR unique_items;
#endif /* USE_ITEMS */
   struct TWINDOW* local_window;

#ifdef USE_CHUNKS
   struct HASHMAP* chunkers;
#endif /* USE_CHUNKS */

   VBOOL local_client;

   int sentinal;     /*!< Used in release version to distinguish from server. */
};

