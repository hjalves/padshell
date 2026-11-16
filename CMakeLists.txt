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
  src/gemini/url.cpp
  src/gemini/gemtext.cpp
  src/gemini/client.cpp
  src/skin.cpp
  src/skins/ameno.cpp
  src/apps/editor.cpp
  src/apps/browser.cpp
)
target_compile_options(padshell PRIVATE -Wall -Wextra)

if(NINTENDO_SWITCH)
  # The IMPORTED_TARGET doesn't carry the --static extras; link them directly.
  target_include_directories(padshell PRIVATE ${SDL_STATIC_INCLUDE_DIRS})
  target_compile_options(padshell PRIVATE ${SDL_STATIC_CFLAGS_OTHER})
  target_link_directories(padshell PRIVATE ${SDL_STATIC_LIBRARY_DIRS})
  target_link_libraries(padshell PRIVATE mbedtls mbedx509 mbedcrypto ${SDL_STATIC_LIBRARIES})

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
  # The Switch ships mbedtls 2.28; build the same version for desktop so the
  # TLS code compiles unchanged (distros now ship 3.x/4.x with another API).
  include(FetchContent)
  set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
  set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "" FORCE)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
  FetchContent_Declare(mbedtls
    URL https://github.com/Mbed-TLS/mbedtls/archive/refs/tags/v2.28.10.tar.gz
    URL_HASH SHA256=0f2e0525903a89ae1d39ce439d858be66933bda54c5b6102b72a29ed8fe7c088)
  FetchContent_MakeAvailable(mbedtls)
  find_package(Threads REQUIRED)

  target_link_libraries(padshell PRIVATE PkgConfig::SDL m mbedtls Threads::Threads)

  # Command-line fetcher for testing the Gemini client: gemfetch <url>
  add_executable(gemfetch tools/gemfetch.cpp src/gemini/url.cpp src/gemini/client.cpp)
  target_link_libraries(gemfetch PRIVATE mbedtls Threads::Threads)
  # Desktop reads assets from <build>/romfs, mirroring romfs:/ on the Switch.
  add_custom_command(TARGET padshell POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/romfs $<TARGET_FILE_DIR:padshell>/romfs)

  enable_testing()
  add_executable(padshell_tests tests/tests.cpp src/textbuffer.cpp src/multitap.cpp src/skin.cpp src/gfx.cpp
    src/gemini/url.cpp src/gemini/gemtext.cpp)
  target_link_libraries(padshell_tests PRIVATE PkgConfig::SDL m)
  target_compile_options(padshell_tests PRIVATE -Wall -Wextra)
  add_test(NAME padshell_tests COMMAND padshell_tests)
endif()
