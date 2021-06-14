cmake_minimum_required(VERSION 3.10)

project(isekai)

add_subdirectory(libs/libvcol)
add_subdirectory(libs/camgraph)
add_subdirectory(libs/libtwin)
add_subdirectory(libs/camin)
add_subdirectory(libs/libgoki)

include_directories(libs/libvcol/src)
include_directories(libs/libvcol/src/bstrlib)
include_directories(libs/camgraph/src)
include_directories(libs/libtwin/src)
include_directories(libs/camin/src)
include_directories(libs/libgoki/src)

set(ISEKAI_C_FILES src/action.c src/animate.c src/audition.c src/b64.c src/backlog.c src/callback.c src/channel.c src/chunker.c src/client.c src/datafile.c src/datafile/xml.c src/ezxml.c src/files.c src/ipc/netipc.c src/item.c src/main.c src/mobile.c src/plugin.c src/plugins/isometer.c src/plugins/pov.c src/plugins/topdown.c src/proto/irc.c src/rng.c src/savestat.c src/savestat/s_bindmp.c src/savestat/s_sqlite.c src/scaffold.c src/server.c src/storage/sqlite.c src/tilemap.c src/tnacl.c src/twindow.c src/vm.c src/vm/duktapev.c src/hs/hsdecode.c src/hs/hsencode.c)

add_executable(isekai ${ISEKAI_C_FILES})

target_link_libraries(isekai PUBLIC libvcol camgraph libtwin camin libgoki)

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
   xDEBUG_TILES
   USE_SDL_IMAGE
   USE_COLORED_CONSOLE
   xDEBUG_PALETTE
   DEBUG_FPS
   ENABLE_LOCAL_CLIENT
   xUSE_DUKTAPE
   xDEBUG_VERBOSEGRAPHICS_COLOR
   USE_STATIC_MODE_TOPDOWN
   xUSE_STATIC_MODE_ISOMETRIC
   USE_STATIC_MODE_POV)

