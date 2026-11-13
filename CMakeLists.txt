cmake_minimum_required(VERSION 3.16)
project(retro_wm VERSION 0.1.0 LANGUAGES C)

option(RETRO_WM_BUILD_DEMO "Build the SDL2 host" ON)
option(RETRO_WM_SANITIZE "Enable address and undefined-behavior sanitizers" OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
include(CTest)

add_library(retro_build_options INTERFACE)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(retro_build_options INTERFACE -Wall -Wextra -Wpedantic -Werror)
    if(RETRO_WM_SANITIZE)
        target_compile_options(retro_build_options INTERFACE
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(retro_build_options INTERFACE -fsanitize=address,undefined)
    endif()
endif()

add_library(retro_wm STATIC
    src/retro_wm.c src/retro_menu.c src/retro_ui.c src/retro_theme.c)
target_include_directories(retro_wm PUBLIC include)
target_link_libraries(retro_wm PUBLIC retro_build_options)

if(RETRO_WM_BUILD_DEMO OR BUILD_TESTING)
    add_library(retro_demo_app STATIC examples/demo_app.c)
    target_include_directories(retro_demo_app PUBLIC examples)
    target_link_libraries(retro_demo_app PUBLIC retro_wm)
endif()

if(RETRO_WM_BUILD_DEMO)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    add_library(retro_sdl_host STATIC platform/sdl/sdl_host.c)
    target_include_directories(retro_sdl_host PUBLIC platform/sdl include)
    target_link_libraries(retro_sdl_host PRIVATE retro_build_options PkgConfig::SDL2)

    add_executable(retro_wm_demo examples/sdl_demo.c examples/demo_smoke.c)
    target_link_libraries(retro_wm_demo PRIVATE
        retro_demo_app retro_sdl_host PkgConfig::SDL2)
endif()

if(BUILD_TESTING)
    add_executable(retro_wm_tests tests/test_wm.c)
    target_link_libraries(retro_wm_tests PRIVATE retro_wm)
    add_test(NAME wm_behavior COMMAND retro_wm_tests)
    if(RETRO_WM_BUILD_DEMO)
        add_test(NAME sdl_smoke COMMAND retro_wm_demo --smoke-test)
        set_tests_properties(sdl_smoke PROPERTIES
            ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software"
            TIMEOUT 15)
    endif()
endif()
