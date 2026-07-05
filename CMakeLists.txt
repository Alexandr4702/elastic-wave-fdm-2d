cmake_minimum_required(VERSION 3.20)
project(elastic_wave_fdm_2d VERSION 0.2.0 LANGUAGES CXX)

option(ELASTIC_WAVE_BUILD_TESTS "Build unit tests" ON)
option(ELASTIC_WAVE_NATIVE_SIMD "Use SIMD instructions available on the build CPU" ON)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/nlohmann_json/CMakeLists.txt")
    message(FATAL_ERROR
        "nlohmann/json submodule is missing; run: git submodule update --init --recursive")
endif()
add_subdirectory(external/nlohmann_json EXCLUDE_FROM_ALL)

find_package(SDL2 2.0.20 CONFIG REQUIRED)
if(TARGET SDL2::SDL2)
    set(ELASTIC_WAVE_SDL_TARGET SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
    set(ELASTIC_WAVE_SDL_TARGET SDL2::SDL2-static)
else()
    message(FATAL_ERROR "The installed SDL2 package does not provide a supported CMake target")
endif()

add_library(elastic_wave src/configuration.cpp src/simulation.cpp)
add_library(elastic_wave::elastic_wave ALIAS elastic_wave)
target_include_directories(elastic_wave PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_features(elastic_wave PUBLIC cxx_std_17)
target_link_libraries(elastic_wave PRIVATE nlohmann_json::nlohmann_json)

if(MSVC)
    target_compile_options(elastic_wave PRIVATE /W4 /permissive-)
    if(ELASTIC_WAVE_NATIVE_SIMD)
        target_compile_options(elastic_wave PRIVATE $<$<CONFIG:Release>:/O2> $<$<CONFIG:Release>:/arch:AVX2>)
    endif()
else()
    target_compile_options(elastic_wave PRIVATE -Wall -Wextra -Wpedantic)
    if(ELASTIC_WAVE_NATIVE_SIMD)
        target_compile_options(elastic_wave PRIVATE $<$<CONFIG:Release>:-O3> $<$<CONFIG:Release>:-march=native>)
    endif()
endif()

add_executable(elastic-wave src/main.cpp)
target_link_libraries(elastic-wave PRIVATE elastic_wave::elastic_wave)

configure_file(environment.json environment.json COPYONLY)

add_executable(elastic-wave-visualizer src/visualizer_sdl.cpp)
target_link_libraries(elastic-wave-visualizer PRIVATE
    elastic_wave::elastic_wave
    ${ELASTIC_WAVE_SDL_TARGET})

if(ELASTIC_WAVE_BUILD_TESTS)
    enable_testing()
    add_executable(elastic-wave-tests tests/simulation_test.cpp)
    target_link_libraries(elastic-wave-tests PRIVATE elastic_wave::elastic_wave)
    add_test(NAME simulation COMMAND elastic-wave-tests)
endif()
