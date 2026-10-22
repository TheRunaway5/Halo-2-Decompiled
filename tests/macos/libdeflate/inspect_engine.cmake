# Run with -DCMAKE_PROJECT_INCLUDE=<this file> in a disposable full-engine
# configure. Inspect the actual final targets without building the deferred
# numerical executor or substituting an engine target.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    function(halo2_inspect_engine_decompression)
        get_target_property(definitions halo2_cache COMPILE_DEFINITIONS)
        get_target_property(links halo2_cache LINK_LIBRARIES)
        get_target_property(library PkgConfig::LIBDEFLATE IMPORTED_LOCATION)
        get_target_property(dependencies PkgConfig::LIBDEFLATE MANUALLY_ADDED_DEPENDENCIES)
        set(expected "${CMAKE_BINARY_DIR}/macos-dependencies/libdeflate/lib/libdeflate.0.dylib")
        set(notice "Libdeflate=${CMAKE_BINARY_DIR}/macos-dependencies/libdeflate/share/licenses/Libdeflate.txt")
        if(NOT APPLE OR NOT HALO2_USE_LIBDEFLATE OR NOT HALO2_MACOS_BUILD_LIBDEFLATE
                OR NOT "HALO2_HAVE_LIBDEFLATE=1" IN_LIST definitions
                OR NOT "PkgConfig::LIBDEFLATE" IN_LIST links OR NOT library STREQUAL expected
                OR NOT "halo2_macos_libdeflate_sdk" IN_LIST dependencies
                OR NOT notice IN_LIST HALO2_MACOS_ENGINE_LICENSES)
            message(FATAL_ERROR "Production MCC accelerator target, SDK dependency or source notice differs")
        endif()
        file(SHA256 "${CMAKE_SOURCE_DIR}/CMakeLists.txt" source_sha)
        set(record "{}")
        string(JSON record SET "${record}" schema "\"H2_MACOS_LIBDEFLATE_ENGINE_SELECTION_V1\"")
        string(JSON record SET "${record}" library "\"${library}\"")
        string(JSON record SET "${record}" notice "\"${notice}\"")
        string(JSON record SET "${record}" root_cmake_sha256 "\"${source_sha}\"")
        string(JSON record SET "${record}" requested_architectures "\"${CMAKE_OSX_ARCHITECTURES}\"")
        string(JSON record SET "${record}" accelerator_defined true)
        string(JSON record SET "${record}" sdk_build_dependency true)
        string(JSON record SET "${record}" license_registered true)
        string(JSON record SET "${record}" full_engine_built false)
        string(JSON record SET "${record}" passed true)
        file(WRITE "${CMAKE_BINARY_DIR}/libdeflate-engine-selection.json" "${record}\n")
    endfunction()
    cmake_language(DEFER CALL halo2_inspect_engine_decompression)
endif()
