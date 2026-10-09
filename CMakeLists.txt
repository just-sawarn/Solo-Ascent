cmake_minimum_required(VERSION 3.20)
project(solo_ascent C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON) # gnu11: required by EM_ASM and raylib

option(SA_SANITIZE "Enable ASan/UBSan (debug)" OFF)
option(SA_NATIVE_TESTS "Build native unit tests" ON)

set(SA_WARN -Wall -Wextra)
if(SA_SANITIZE)
  set(SA_SAN_FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

include(FetchContent)
# cJSON: fetched but built here (its own CMake predates CMake 4 policy rules); SOURCE_SUBDIR skips its CMakeLists.
FetchContent_Declare(cjson GIT_REPOSITORY https://github.com/DaveGamble/cJSON.git GIT_TAG v1.7.18 GIT_SHALLOW TRUE SOURCE_SUBDIR sa_no_cmake)
FetchContent_MakeAvailable(cjson)
add_library(cjson STATIC ${cjson_SOURCE_DIR}/cJSON.c)
target_include_directories(cjson PUBLIC ${cjson_SOURCE_DIR})
target_compile_options(cjson PRIVATE -w)

# ---- Engine-agnostic game logic (no raylib, no browser calls; natively testable) ----
add_library(sa_core STATIC
  src/core/ecs.c
  src/core/physics.c
  src/core/player.c
  src/core/camrig.c
  src/core/data.c
  src/core/combat.c
  src/core/enemy_ai.c
  src/core/items.c
  src/core/inventory.c
  src/core/story.c
  src/core/party.c
  src/core/companion_ai.c
  src/core/gameloop.c
  src/core/vec.c
  src/core/log.c)
target_include_directories(sa_core PUBLIC src)
target_link_libraries(sa_core PUBLIC cjson)
target_compile_definitions(sa_core PUBLIC SA_DATA_DIR="${CMAKE_SOURCE_DIR}/data")
target_compile_options(sa_core PRIVATE ${SA_WARN} ${SA_SAN_FLAGS})
target_link_options(sa_core PUBLIC ${SA_SAN_FLAGS})
if(NOT EMSCRIPTEN)
  target_link_libraries(sa_core PUBLIC m)
endif()

# ---- Native tests ----
if(SA_NATIVE_TESTS AND NOT EMSCRIPTEN)
  enable_testing()
  foreach(t ecs gameloop physics player camrig data combat enemy_ai party items inventory story story_content)
    add_executable(test_${t} tests/test_${t}.c)
    target_link_libraries(test_${t} PRIVATE sa_core)
    target_compile_options(test_${t} PRIVATE ${SA_WARN} ${SA_SAN_FLAGS})
    add_test(NAME ${t} COMMAND test_${t})
  endforeach()
endif()

# ---- Game (raylib) : web build, or native desktop for quick iteration ----
if(EMSCRIPTEN OR SA_BUILD_NATIVE_GAME)
  include(FetchContent)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
  if(EMSCRIPTEN)
    set(PLATFORM Web CACHE STRING "" FORCE)
    set(GRAPHICS GRAPHICS_API_OPENGL_ES3 CACHE STRING "" FORCE)
  endif()
  FetchContent_Declare(raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(raylib)

  target_compile_options(raylib PRIVATE -w) # third-party warnings are not ours to fix

  add_executable(solo_ascent
    src/main.c
    src/ui/debug_overlay.c
    src/ui/input.c
    src/ui/hud.c
    src/ui/menu.c
    src/ui/story_hud.c
    src/ui/dialogue.c
    src/render/actors.c
    src/render/renderer.c
    src/render/shaders.c
    src/render/procmodels.c
    src/render/character.c
    src/render/anim.c
    src/render/heroes.c
    src/world/test_scene.c
    src/world/npc_world.c
    src/platform/web/platform.c)
  target_link_libraries(solo_ascent PRIVATE sa_core raylib)
  target_compile_options(solo_ascent PRIVATE ${SA_WARN} ${SA_SAN_FLAGS})

  if(EMSCRIPTEN)
    set_target_properties(solo_ascent PROPERTIES SUFFIX ".html")
    target_link_options(solo_ascent PRIVATE
      -sUSE_GLFW=3 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sFULL_ES3=1
      -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=64MB -sSTACK_SIZE=1MB
      -sASSERTIONS=$<IF:$<CONFIG:Debug>,1,0>
      -sEXPORTED_RUNTIME_METHODS=ccall,cwrap
      -sEXPORTED_FUNCTIONS=_main,_sa_on_visibility,_sa_on_context_lost,_sa_on_context_restored
      "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/assets/models/weapons@/assets/weapons"
      "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/assets/models/characters@/assets/characters"
      "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/assets/models/animations@/assets/animations"
      "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/data@/data"
      --shell-file ${CMAKE_SOURCE_DIR}/web/shell.html)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
      target_link_options(solo_ascent PRIVATE -gsource-map)
    else()
      target_compile_options(solo_ascent PRIVATE -O3 -flto)
      target_link_options(solo_ascent PRIVATE -O3 -flto --closure=1 "--closure-args=--externs ${CMAKE_SOURCE_DIR}/web/externs.js")
    endif()
  endif()
endif()

# ---- Web glue files copied next to the generated page ----
if(EMSCRIPTEN)
  set(SA_WEB_FILES glue.js game.css sw.js manifest.webmanifest icon-192.png icon-512.png)
  foreach(f ${SA_WEB_FILES})
    add_custom_command(TARGET solo_ascent POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/web/${f} $<TARGET_FILE_DIR:solo_ascent>/${f})
  endforeach()
  set_target_properties(solo_ascent PROPERTIES OUTPUT_NAME index)
endif()
