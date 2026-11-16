cmake_minimum_required(VERSION 3.22)
project(padshell VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(PADSHELL_AUTHOR "padshell" CACHE STRING "Author shown in the Switch homebrew menu")

find_package(PkgConfig REQUIRED)
if(NINTENDO_SWITCH)
  # Switch portlibs are static: pull in SDL's transitive deps too.
  set(PKG_CONFIG_ARGN --static)
endif()
pkg_check_modules(SDL REQUIRED IMPORTED_TARGET sdl2 SDL2_ttf)

add_executable(padshell
  src/main.cpp
  src/gfx.cpp
  src/multitap.cpp
  src/platform.cpp
  src/shell.cpp
  src/textbuffer.cpp
  src/skin.cpp
  src/skins/ameno.cpp
  src/apps/editor.cpp
)
target_compile_options(padshell PRIVATE -Wall -Wextra)

if(NINTENDO_SWITCH)
  # The IMPORTED_TARGET doesn't carry the --static extras; link them directly.
  target_include_directories(padshell PRIVATE ${SDL_STATIC_INCLUDE_DIRS})
  target_compile_options(padshell PRIVATE ${SDL_STATIC_CFLAGS_OTHER})
  target_link_directories(padshell PRIVATE ${SDL_STATIC_LIBRARY_DIRS})
  target_link_libraries(padshell PRIVATE ${SDL_STATIC_LIBRARIES})

  nx_generate_nacp(padshell.nacp
    NAME "padshell"
    AUTHOR "${PADSHELL_AUTHOR}"
    VERSION "${PROJECT_VERSION}"
  )
  nx_create_nro(padshell
    NACP padshell.nacp
    ROMFS ${CMAKE_CURRENT_SOURCE_DIR}/romfs
  )
else()
  target_link_libraries(padshell PRIVATE PkgConfig::SDL m)
  # Desktop reads assets from <build>/romfs, mirroring romfs:/ on the Switch.
  add_custom_command(TARGET padshell POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/romfs $<TARGET_FILE_DIR:padshell>/romfs)

  enable_testing()
  add_executable(padshell_tests tests/tests.cpp src/textbuffer.cpp src/multitap.cpp)
  target_compile_options(padshell_tests PRIVATE -Wall -Wextra)
  add_test(NAME padshell_tests COMMAND padshell_tests)
endif()
