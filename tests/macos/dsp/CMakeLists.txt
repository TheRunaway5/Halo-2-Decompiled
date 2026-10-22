cmake_minimum_required(VERSION 3.24)
get_filename_component(HALO2_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
include("${HALO2_ROOT}/cmake/macos/Defaults.cmake")
project(Halo2MacOSDspTests LANGUAGES C)
if(NOT APPLE)
    message(FATAL_ERROR "This fixture requires macOS")
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
include("${HALO2_ROOT}/cmake/TestStorage.cmake")
include("${HALO2_ROOT}/cmake/macos/Dsp.cmake")
include(CTest)
set(HALO2_DSP_CARGO "" CACHE FILEPATH "Rustup-managed Cargo")
set(HALO2_DSP_RUST_TOOLCHAIN "1.99.0" CACHE STRING "Pinned DSP toolchain")
halo2_macos_build_dsp_archive(archive SOURCE "${HALO2_ROOT}/third_party/dsp56300"
    DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/rust" CARGO "${HALO2_DSP_CARGO}"
    TOOLCHAIN "${HALO2_DSP_RUST_TOOLCHAIN}"
    ENVIRONMENT CARGO_PROFILE_RELEASE_OPT_LEVEL=0 CARGO_PROFILE_RELEASE_DEBUG=0
        CARGO_PROFILE_RELEASE_CODEGEN_UNITS=256)
add_custom_target(macos_dsp_archive DEPENDS "${archive}")
add_library(macos_dsp STATIC IMPORTED)
set_target_properties(macos_dsp PROPERTIES IMPORTED_LOCATION "${archive}")
add_dependencies(macos_dsp macos_dsp_archive)
add_executable(macos_dsp_tests dsp_tests.c "${HALO2_ROOT}/src/audio/dsp/sound_gp_cpu_tu15_ffi.c")
target_include_directories(macos_dsp_tests PRIVATE "${HALO2_ROOT}/src"
    "${HALO2_ROOT}/third_party/dsp56300/crates/emu-ffi/include")
target_link_libraries(macos_dsp_tests PRIVATE macos_dsp)
foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
    add_test(NAME macos_dsp_${arch} COMMAND /usr/bin/arch -${arch} $<TARGET_FILE:macos_dsp_tests>)
endforeach()
add_custom_target(macos_dsp_archive_structure_test)
add_test(NAME macos_dsp_archive_structure COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/archive_tests.py")
