include_guard(GLOBAL)
find_program(HALO2_MACOS_LIPO lipo REQUIRED)

function(halo2_macos_verify_dsp_archive archive)
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        execute_process(COMMAND "${HALO2_MACOS_LIPO}" "${archive}"
            -verify_arch "${arch}" RESULT_VARIABLE result ERROR_VARIABLE error)
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "DSP archive lacks the requested macOS architectures (${arch}): ${error}")
        endif()
    endforeach()
endfunction()

function(halo2_macos_build_dsp_archive output)
    cmake_parse_arguments(DSP "" "SOURCE;DIRECTORY;CARGO;TOOLCHAIN" "ENVIRONMENT" ${ARGN})
    if(NOT CMAKE_OSX_ARCHITECTURES)
        message(FATAL_ERROR "Set explicit macOS architectures for the Rust DSP archive")
    endif()
    set(work "${DSP_DIRECTORY}/source")
    set(archive "${DSP_DIRECTORY}/universal/libdsp56300_emu_ffi.a")
    file(GLOB_RECURSE inputs CONFIGURE_DEPENDS
        "${DSP_SOURCE}/crates/*.rs" "${DSP_SOURCE}/crates/*.toml"
        "${DSP_SOURCE}/crates/*.c" "${DSP_SOURCE}/crates/*.h")
    list(APPEND inputs "${DSP_SOURCE}/Cargo.toml" "${DSP_SOURCE}/Cargo.lock")
    # Upstream build.rs generates a C header in its source directory. Keep that
    # write in this build's private copy, including for ordinary engine builds.
    set(commands
        COMMAND ${CMAKE_COMMAND} -E make_directory "${work}" "${DSP_DIRECTORY}/universal"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DSP_SOURCE}/Cargo.toml" "${DSP_SOURCE}/Cargo.lock" "${work}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${DSP_SOURCE}/crates" "${work}/crates")
    set(slices)
    set(verify_commands)
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        if(arch STREQUAL "arm64")
            set(triple aarch64-apple-darwin)
        elseif(arch STREQUAL "x86_64")
            set(triple x86_64-apple-darwin)
        else()
            message(FATAL_ERROR "Unsupported macOS Rust DSP architecture: ${arch}")
        endif()
        list(APPEND commands COMMAND ${CMAKE_COMMAND} -E env ${DSP_ENVIRONMENT}
            "MACOSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}"
            "${DSP_CARGO}" "+${DSP_TOOLCHAIN}" build
            --manifest-path "${work}/Cargo.toml" --locked --release
            -p dsp56300-emu-ffi --no-default-features -j 1
            --target "${triple}" --target-dir "${DSP_DIRECTORY}/targets"
            --config profile.release.package.dsp56300-core.opt-level=2
            --config profile.release.package.dsp56300-emu.opt-level=2
            --config profile.release.package.dsp56300-emu-ffi.opt-level=2)
        list(APPEND slices "${DSP_DIRECTORY}/targets/${triple}/release/libdsp56300_emu_ffi.a")
        list(APPEND verify_commands COMMAND "${HALO2_MACOS_LIPO}" "${archive}" -verify_arch "${arch}")
    endforeach()
    # One command serializes both Cargo builds and generated-header writes.
    add_custom_command(OUTPUT "${archive}" ${commands}
        COMMAND ${CMAKE_COMMAND} -E compare_files
            "${work}/crates/emu-ffi/include/dsp56300.h" "${DSP_SOURCE}/crates/emu-ffi/include/dsp56300.h"
        COMMAND "${HALO2_MACOS_LIPO}" -create ${slices} -output "${archive}"
        ${verify_commands}
        DEPENDS ${inputs}
        COMMENT "Building and verifying macOS Rust DSP architecture slices" VERBATIM)
    set(${output} "${archive}" PARENT_SCOPE)
endfunction()
