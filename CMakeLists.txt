cmake_minimum_required(VERSION 3.10)

project(isekai)

find_package(SDL REQUIRED)
find_package(SDL_image REQUIRED)

include_directories(${SDL_INCLUDE_DIRS})
include_directories(${SDL_IMAGE_INCLUDE_DIR})

set(ISEKAI_C_FILES src/animate.c src/backlog.c src/bstrglue.c src/bstrlib/bstraux.c src/bstrlib/bstrlib.c src/bstrlib/buniutil.c src/bstrlib/utf8util.c src/callback.c src/channel.c src/client.c src/datafile.c src/datafile/xml.c src/ezxml.c src/files.c src/graphics.c src/graphics/sdlg.c src/hashmap.c src/htmlrend.c src/htmtree.c src/input/sdli.c src/ipc/netipc.c src/item.c src/main.c src/mem.c src/mobile.c src/mode.c src/mode/local2d.c src/mode/localiso.c src/mode/localray.c src/proto/irc.c src/rng.c src/savestat.c src/savestat/s_bindmp.c src/savestat/s_sqlite.c src/scaffold.c src/server.c src/tilemap.c src/tnacl.c src/twindow.c src/ui/graphicu.c src/vector.c)

add_executable(isekai ${ISEKAI_C_FILES})

target_link_libraries(isekai ${SDL_LIBRARY} ${SDL_IMAGE_LIBRARY} m)

target_compile_definitions(isekai PUBLIC
   USE_SDL
   DEBUG
   USE_NETWORK
   USE_RANDOM_PORT
   USE_CONNECT_DIALOG
   USE_SDL_IMAGE
   USE_COLORED_CONSOLE
   DEBUG_FPS
   ENABLE_LOCAL_CLIENT
   )

