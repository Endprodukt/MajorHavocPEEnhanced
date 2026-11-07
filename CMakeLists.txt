cmake_minimum_required(VERSION 3.21)

project(mhavocpe_enhanced VERSION 0.3.1 LANGUAGES C CXX)

# Builds: see BUILD.md.

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(MH_BUILD_GAME "Build the game itself (needs the VectorPie SDK and SDL2)" ON)

# --- third party ------------------------------------------------------------

# miniz (MIT): reads the ROM zip.
add_library(miniz STATIC third_party/miniz/miniz.c)
target_include_directories(miniz PUBLIC third_party/miniz)
target_compile_options(miniz PRIVATE -w)

# --- the board: no SDK, no SDL, testable on its own ---------------------------

add_library(mhcore STATIC
    src/core/machine.c
    src/core/emu.c
    src/core/options.c
    src/core/pokeyio.c
    src/cpu/m6502.c
    src/avg/avg.c
    src/avg/avgsm.c
    src/rom/romset.c
    src/sound/pokey.c
    src/sound/oki6295.c
    src/sound/sound.c
    src/render/svg.c
    src/render/layout.c
    src/render/config.c
    src/render/smooth.c
    src/render/frames.c
    src/render/enhance.c
)
target_include_directories(mhcore PUBLIC src)
target_link_libraries(mhcore PUBLIC miniz)
target_compile_options(mhcore PRIVATE -Wall -Wextra)
if(UNIX)
    target_link_libraries(mhcore PUBLIC m)
endif()

# --- tools ---------------------------------------------------------------------

# Runs the game without a screen and writes its pictures (SVG).
add_executable(mhshot tools/mhshot/main.c)
target_link_libraries(mhshot PRIVATE mhcore)
target_compile_options(mhshot PRIVATE -Wall -Wextra)

# --- the game: on the VectorPie SDK (a window on a PC, the vector board on the cabinet) ---

if(MH_BUILD_GAME)
    set(MH_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/VectorPie/sdk"
        CACHE PATH "VectorPie SDK source directory")
    if(NOT EXISTS "${MH_SDK_DIR}/CMakeLists.txt")
        message(FATAL_ERROR "VectorPie SDK not found in ${MH_SDK_DIR}: see BUILD.md "
                            "(or configure with -DMH_BUILD_GAME=OFF for the tools only)")
    endif()
    if(MH_PI_BUILD)
        # cmake/toolchain-pi-clang.cmake (the Pi build from Windows, without
        # WSL): the cabinet's own SDL2, by its file.
        add_library(SDL2::SDL2 SHARED IMPORTED GLOBAL)
        set_target_properties(SDL2::SDL2 PROPERTIES
            IMPORTED_LOCATION "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu/libSDL2-2.0.so.0"
            IMPORTED_SONAME "libSDL2-2.0.so.0"
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SYSROOT}/usr/include/SDL2")
        set(THREADS_PREFER_PTHREAD_FLAG ON)
    endif()
    set(VP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(VP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory("${MH_SDK_DIR}" vectorpie EXCLUDE_FROM_ALL)

    add_library(mhout STATIC
        src/render/draw.c
        src/platform/app.c
        src/platform/audio.c
        src/platform/controls.c
        src/platform/menu.c
    )
    target_link_libraries(mhout PUBLIC mhcore vectorpie)
    target_compile_options(mhout PRIVATE -Wall -Wextra)

    add_executable(mhavocpe src/main.c)
    target_link_libraries(mhavocpe PRIVATE mhout)
    vp_program(mhavocpe)

    # package files next to the program (vp_asset_path looks there)
    configure_file(package/mhavocpe.ini ${CMAKE_CURRENT_BINARY_DIR}/mhavocpe.ini COPYONLY)
    configure_file(package/game.ini ${CMAKE_CURRENT_BINARY_DIR}/game.ini COPYONLY)
    configure_file(package/marquee.png ${CMAKE_CURRENT_BINARY_DIR}/marquee.png COPYONLY)
    # (the ROMs do not come with the game: the owner's roms/ next to the
    # program, or the cabinet's ROM folders; README)
endif()
