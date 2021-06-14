cmake_minimum_required(VERSION 3.10)

project(isekai)

if(NOT DEFINED GFX_API)
    set(GFX_API SDL)
endif()

if(NOT DEFINED IPC_API)
   set(IPC_API NET)
endif()

set(ISEKAI_LIBS)
set(ISEKAI_INCLUDES)

set(ISEKAI_DEFINES
   DEBUG
   USE_COLORED_CONSOLE
   ENABLE_LOCAL_CLIENT)

set(ISEKAI_C_FILES src/animate.c src/backlog.c src/bstrglue.c src/bstrlib/bstraux.c src/bstrlib/bstrlib.c src/bstrlib/buniutil.c src/bstrlib/utf8util.c src/callback.c src/channel.c src/client.c src/datafile.c src/datafile/xml.c src/ezxml.c src/files.c src/graphics.c src/hashmap.c src/htmlrend.c src/htmtree.c src/item.c src/main.c src/mem.c src/mobile.c src/mode.c src/mode/local2d.c src/mode/localiso.c src/mode/localray.c src/rng.c src/savestat.c src/savestat/s_bindmp.c src/savestat/s_sqlite.c src/scaffold.c src/server.c src/tilemap.c src/twindow.c src/vector.c)

if(IPC_API MATCHES "NET")
   list(APPEND ISEKAI_DEFINES USE_NETWORK USE_CONNECT_DIALOG USE_RANDOM_PORT)
   list(APPEND ISEKAI_C_FILES src/ipc/netipc.c src/proto/irc.c)
else()
   list(APPEND ISEKAI_C_FILES src/ipc/shmipc.c src/proto/shmp.c)
endif()

if(GFX_API MATCHES "SDL")
   list(APPEND ISEKAI_DEFINES USE_SDL USE_SDL_IMAGE)
   list(APPEND ISEKAI_C_FILES src/graphics/sdlg.c src/input/sdli.c src/ui/graphicu.c)

   find_package(SDL REQUIRED)
   find_package(SDL_image REQUIRED)

   list(APPEND ISEKAI_LIBS ${SDL_LIBRARY} ${SDL_IMAGE_LIBRARY})

   list(APPEND ISEKAI_INCLUDES ${SDL_INCLUDE_DIRS} ${SDL_IMAGE_INCLUDE_DIR})
else()
   list(APPEND ISEKAI_C_FILES src/graphics/nullg.c src/input/nulli.c src/ui/nullu.c)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "GNU")
   list(APPEND ISEKAI_DEFINES USE_STDINT USE_REF_INLINE USE_FUNCTION USE_SNPRINTF)
   list(APPEND ISEKAI_LIBS m)
elseif(CMAKE_C_COMPILER_ID MATCHES "Bruce")
   add_compile_options(-ansi -3)
endif()

add_executable(isekai ${ISEKAI_C_FILES})

set_property(TARGET isekai PROPERTY C_STANDARD 90)

target_link_libraries(isekai ${ISEKAI_INCLUDES})

include_directories(${ISEKAI_INCLUDES})

target_compile_definitions(isekai PUBLIC ${ISEKAI_DEFINES})

