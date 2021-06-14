cmake_minimum_required(VERSION 3.10)

project(isekai)

set(ISEKAI_C_FILES src/animate.c src/b64.c src/backlog.c src/bstrglue.c src/bstrlib/bstraux.c src/bstrlib/bstrlib.c src/bstrlib/buniutil.c src/bstrlib/utf8util.c src/callback.c src/channel.c src/chunker.c src/client.c src/datafile.c src/datafile/xml.c src/duktape/duktape.c src/ezxml.c src/files.c src/graphics.c src/graphics/sdlg.c src/hashmap.c src/hs/hsdecode.c src/hs/hsencode.c src/htmlrend.c src/htmtree.c src/input/sdli.c src/ipc/netipc.c src/item.c src/main.c src/mem.c src/mobile.c src/mode.c src/mode/local2d.c src/mode/localiso.c src/mode/localray.c src/proto/irc.c src/rng.c src/savestat.c src/savestat/s_bindmp.c src/savestat/s_sqlite.c src/scaffold.c src/server.c src/tilemap.c src/tinypy/tinypy.c src/tnacl.c src/twindow.c src/ui/graphicu.c src/vector.c src/vm.c src/vm/duktapev.c)

add_executable(isekai ${ISEKAI_C_FILES})

target_compile_definitions(isekai PUBLIC
   USE_SDL
   DEBUG
   USE_NETWORK
   xDEBUG_NETWORK
   USE_FILE_CACHE
   USE_CHUNKS
   USE_RANDOM_PORT
   xDEBUG_REF
   USE_CONNECT_DIALOG
   DEBUG_TILES
   USE_SDL_IMAGE
   USE_COLORED_CONSOLE
   xDEBUG_PALETTE
   DEBUG_FPS
   ENABLE_LOCAL_CLIENT
   USE_DUKTAPE
   xDEBUG_VERBOSEGRAPHICS_COLOR
   USE_SDL
   DEBUG
   USE_NETWORK
   xDEBUG_NETWORK
   USE_FILE_CACHE
   USE_CHUNKS
   USE_RANDOM_PORT
   xDEBUG_REF
   USE_CONNECT_DIALOG
   DEBUG_TILES
   USE_SDL_IMAGE
   USE_COLORED_CONSOLE
   xDEBUG_PALETTE
   DEBUG_FPS
   ENABLE_LOCAL_CLIENT
   USE_DUKTAPE
   xDEBUG_VERBOSEGRAPHICS_COLOR
   )

