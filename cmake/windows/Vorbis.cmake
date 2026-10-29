include_guard(GLOBAL)

function(halo2_windows_loading_vorbis_dependency)
    if(TARGET halo2_windows_loading_vorbis)
        return()
    endif()
    if(NOT WIN32)
        message(FATAL_ERROR "The Windows loading Vorbis dependency requires a Windows target")
    endif()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(HALO2_WINDOWS_LOADING_OGG REQUIRED IMPORTED_TARGET GLOBAL ogg)

    get_filename_component(root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../.." ABSOLUTE)
    set(prefix "${CMAKE_BINARY_DIR}/windows-dependencies/loading-vorbis")
    set(source "${prefix}/source")
    set(stamp "${prefix}/prepared.stamp")
    set(sources)
    foreach(name mdct smallft block envelope window lsp lpc analysis synthesis psy info
            floor1 floor0 res0 mapping0 registry codebook sharedbook lookup bitrate
            canonical_vorbis_kernels canonical_vorbis_kernels_mdct)
        list(APPEND sources "${source}/lib/${name}.c")
    endforeach()
    set(headers "${source}/lib/canonical_vorbis_kernels.h")
    foreach(name envelope lpc lsp codebook misc psy masking os mdct smallft highlevel
            registry scales window lookup lookup_data codec_internal backends bitrate)
        list(APPEND headers "${source}/lib/${name}.h")
    endforeach()
    foreach(name codec vorbisenc vorbisfile)
        list(APPEND headers "${source}/include/vorbis/${name}.h")
    endforeach()
    add_custom_command(OUTPUT "${stamp}"
        COMMAND "${Python3_EXECUTABLE}" -B "${root}/tools/windows/prepare_loading_vorbis.py"
            --output "${source}"
        COMMAND "${CMAKE_COMMAND}" -E touch "${stamp}"
        BYPRODUCTS ${sources} ${headers} "${source}/COPYING" "${source}/manifest.json"
        DEPENDS "${root}/tools/windows/prepare_loading_vorbis.py"
            "${root}/tools/windows/canonical_vorbis_kernels.c"
            "${root}/tools/windows/canonical_vorbis_kernels_mdct.c"
            "${root}/tools/windows/canonical_vorbis_kernels.h"
            "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
        COMMENT "Preparing pinned Vorbis with canonical loading-audio arithmetic" VERBATIM)
    add_custom_target(halo2_windows_loading_vorbis_sources DEPENDS "${stamp}")
    set_source_files_properties(${sources} ${headers} PROPERTIES GENERATED TRUE)
    add_library(halo2_windows_loading_vorbis STATIC ${sources} ${headers})
    add_dependencies(halo2_windows_loading_vorbis halo2_windows_loading_vorbis_sources)
    set_target_properties(halo2_windows_loading_vorbis PROPERTIES
        C_STANDARD 99 C_STANDARD_REQUIRED YES)
    target_include_directories(halo2_windows_loading_vorbis
        PUBLIC "${source}/include" PRIVATE "${source}/lib")
    target_link_libraries(halo2_windows_loading_vorbis PUBLIC PkgConfig::HALO2_WINDOWS_LOADING_OGG)
    if(MSVC)
        target_compile_options(halo2_windows_loading_vorbis PRIVATE /fp:strict)
    else()
        # Canonical kernels spell out each rounded float operation and fmaf.
        # Keep the surrounding decoder strict, with the compiler's baseline ISA.
        target_compile_options(halo2_windows_loading_vorbis PRIVATE -fno-fast-math -ffp-contract=off)
        target_link_libraries(halo2_windows_loading_vorbis PRIVATE m)
    endif()

    set(HALO2_WINDOWS_LOADING_VORBIS_SOURCE_DIR "${source}" PARENT_SCOPE)
    set(HALO2_WINDOWS_LOADING_VORBIS_LICENSE "${source}/COPYING" PARENT_SCOPE)
    set(HALO2_WINDOWS_LOADING_VORBIS_MANIFEST "${source}/manifest.json" PARENT_SCOPE)
endfunction()
