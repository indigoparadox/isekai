cmake_minimum_required(VERSION 3.10)

project(isekai)

set(ISEKAI_DEFINES
   DEBUG
   USE_NETWORK
   USE_RANDOM_PORT
   USE_CONNECT_DIALOG
   USE_COLORED_CONSOLE
   ENABLE_LOCAL_CLIENT)

if(NOT DEFINED GFX_API)
    set(GFX_API SDL)
endif()

if(GFX_API MATCHES "SDL")
   list(APPEND ISEKAI_DEFINES USE_SDL USE_SDL_IMAGE)

   find_package(SDL REQUIRED)
   find_package(SDL_image REQUIRED)

   include_directories(${SDL_INCLUDE_DIRS})
   include_directories(${SDL_IMAGE_INCLUDE_DIR})
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU")
   list(APPEND ISEKAI_DEFINES USE_STDINT USE_REF_INLINE USE_FUNCTION)
elseif(CMAKE_C_COMPILER_ID MATCHES "Bruce")
   add_compile_options(-ansi -3)
endif()

set(ISEKAI_C_FILES src/animate.c src/backlog.c src/bstrglue.c src/bstrlib/bstraux.c src/bstrlib/bstrlib.c src/bstrlib/buniutil.c src/bstrlib/utf8util.c src/callback.c src/channel.c src/client.c src/datafile.c src/datafile/xml.c src/ezxml.c src/files.c src/graphics.c src/graphics/sdlg.c src/hashmap.c src/htmlrend.c src/htmtree.c src/input/sdli.c src/ipc/netipc.c src/item.c src/main.c src/mem.c src/mobile.c src/mode.c src/mode/local2d.c src/mode/localiso.c src/mode/localray.c src/proto/irc.c src/rng.c src/savestat.c src/savestat/s_bindmp.c src/savestat/s_sqlite.c src/scaffold.c src/server.c src/tilemap.c src/tnacl.c src/twindow.c src/ui/graphicu.c src/vector.c)

add_executable(isekai ${ISEKAI_C_FILES})

set_property(TARGET isekai PROPERTY C_STANDARD 90)

target_link_libraries(isekai ${SDL_LIBRARY} ${SDL_IMAGE_LIBRARY} m)

target_compile_definitions(isekai PUBLIC ${ISEKAI_DEFINES})

