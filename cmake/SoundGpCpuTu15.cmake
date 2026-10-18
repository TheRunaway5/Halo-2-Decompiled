# Explicit native GP execution target. It is not part of the ordinary build
# until a real platform consumer links it; no approximate fallback is selected.
enable_language(C)
find_program(HALO2_DSP_CARGO cargo DOC "Cargo with a Rust 1.99 toolchain for the pinned MIT DSP core")
set(HALO2_DSP_RUST_TOOLCHAIN "1.99.0" CACHE STRING "Rustup toolchain for the pinned DSP core")
set(HALO2_DSP_RUSTUP_HOME "" CACHE PATH "Optional private Rustup home; does not change the global toolchain")
set(HALO2_DSP_PREBUILT_LIBRARY "" CACHE FILEPATH "Optional separately built pinned dsp56300-emu-ffi static library")
set(_halo2_gp_source "${CMAKE_CURRENT_SOURCE_DIR}/third_party/dsp56300")
set(_halo2_gp_target "${CMAKE_CURRENT_BINARY_DIR}/tu15-dsp-rust")
if(WIN32)
    set(_halo2_gp_archive "${_halo2_gp_target}/release/dsp56300_emu_ffi.lib")
else()
    set(_halo2_gp_archive "${_halo2_gp_target}/release/libdsp56300_emu_ffi.a")
endif()
set(_halo2_gp_env CARGO_PROFILE_RELEASE_OPT_LEVEL=0 CARGO_PROFILE_RELEASE_DEBUG=0 CARGO_PROFILE_RELEASE_CODEGEN_UNITS=256)
if(HALO2_DSP_RUSTUP_HOME)
    list(APPEND _halo2_gp_env "RUSTUP_HOME=${HALO2_DSP_RUSTUP_HOME}")
endif()
file(GLOB_RECURSE _halo2_gp_inputs CONFIGURE_DEPENDS "${_halo2_gp_source}/*.rs" "${_halo2_gp_source}/*.toml" "${_halo2_gp_source}/Cargo.lock")
if(HALO2_DSP_PREBUILT_LIBRARY)
    if(NOT EXISTS "${HALO2_DSP_PREBUILT_LIBRARY}")
        message(FATAL_ERROR "HALO2_DSP_PREBUILT_LIBRARY does not exist")
    endif()
    set(_halo2_gp_archive "${HALO2_DSP_PREBUILT_LIBRARY}")
else()
    add_custom_command(OUTPUT "${_halo2_gp_archive}"
    COMMAND ${CMAKE_COMMAND} -E env ${_halo2_gp_env}
        "${HALO2_DSP_CARGO}" "+${HALO2_DSP_RUST_TOOLCHAIN}" build
        --manifest-path "${_halo2_gp_source}/Cargo.toml" --locked --release
        -p dsp56300-emu-ffi --no-default-features -j 1 --target-dir "${_halo2_gp_target}"
        --config profile.release.package.dsp56300-core.opt-level=2
        --config profile.release.package.dsp56300-emu.opt-level=2
        --config profile.release.package.dsp56300-emu-ffi.opt-level=2
    DEPENDS ${_halo2_gp_inputs}
    COMMENT "Building pinned MIT DSP56300 instruction core (serial, explicit target)"
        VERBATIM)
    add_custom_target(halo2_dsp56300_rust DEPENDS "${_halo2_gp_archive}")
endif()
add_library(halo2_dsp56300 STATIC IMPORTED)
set_target_properties(halo2_dsp56300 PROPERTIES IMPORTED_LOCATION "${_halo2_gp_archive}")
if(NOT HALO2_DSP_PREBUILT_LIBRARY)
    add_dependencies(halo2_dsp56300 halo2_dsp56300_rust)
endif()
add_library(halo2_sound_gp_cpu_tu15 STATIC EXCLUDE_FROM_ALL src/sound_gp_cpu_tu15.cpp src/sound_gp_cpu_tu15_ffi.c)
add_library(halo2_sound_gp_dma_tu15 STATIC EXCLUDE_FROM_ALL src/sound_gp_dma_tu15.cpp)
target_include_directories(halo2_sound_gp_dma_tu15 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(halo2_sound_gp_dma_tu15 PUBLIC cxx_std_20)
option(HALO2_BUILD_GP_DMA_TESTS "Build the explicit native GP scratch DMA owner tests" OFF)
if(BUILD_TESTING AND HALO2_BUILD_GP_DMA_TESTS)
    add_executable(sound_gp_dma_tu15_tests tests/sound_gp_dma_tu15_tests.cpp)
    target_link_libraries(sound_gp_dma_tu15_tests PRIVATE halo2_sound_gp_dma_tu15 Threads::Threads)
    add_test(NAME native_sound_gp_dma_owner_tu15 COMMAND sound_gp_dma_tu15_tests)
endif()
target_include_directories(halo2_sound_gp_cpu_tu15 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${_halo2_gp_source}/crates/emu-ffi/include")
target_link_libraries(halo2_sound_gp_cpu_tu15 PUBLIC halo2_dsp56300 OpenSSL::Crypto Threads::Threads ${CMAKE_DL_LIBS})
if(UNIX)
    target_link_libraries(halo2_sound_gp_cpu_tu15 PUBLIC m)
endif()
option(HALO2_BUILD_GP_CPU_TESTS "Build the explicit native GP CPU owner tests (requires pinned Rust toolchain)" OFF)
if(BUILD_TESTING AND HALO2_BUILD_GP_CPU_TESTS)
    add_executable(sound_gp_cpu_tu15_tests tests/sound_gp_cpu_tu15_tests.cpp)
    target_link_libraries(sound_gp_cpu_tu15_tests PRIVATE halo2_sound_gp_cpu_tu15)
    add_test(NAME native_sound_gp_cpu_owner_tu15 COMMAND sound_gp_cpu_tu15_tests)
    add_test(NAME original_sound_gp_block_owner_tu15 COMMAND sound_gp_cpu_tu15_tests
        --original-gp "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_gp_image_tu15/gp-upload.bin")
    set_tests_properties(original_sound_gp_block_owner_tu15 PROPERTIES SKIP_RETURN_CODE 77)
endif()
