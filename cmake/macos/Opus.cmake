include_guard(GLOBAL)
function(halo2_macos_opus_dependency)
    if(TARGET PkgConfig::OPUS)
        return()
    endif()
    if(NOT APPLE OR NOT CMAKE_OSX_ARCHITECTURES)
        message(FATAL_ERROR "The macOS Opus dependency needs explicit Mac architectures")
    endif()
    get_filename_component(root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../.." ABSOLUTE)
    set(prefix "${CMAKE_BINARY_DIR}/macos-dependencies/opus")
    set(work "${CMAKE_BINARY_DIR}/macos-dependencies/opus-work")
    set(library "${prefix}/lib/libopus.0.dylib")
    set(stamp "${prefix}/built.stamp")
    set(HALO2_MACOS_DEPENDENCY_JOBS 2 CACHE STRING "Jobs within serial macOS dependency builds")
    set(arguments)
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        list(APPEND arguments --arch "${arch}")
    endforeach()
    if(CMAKE_OSX_SYSROOT)
        list(APPEND arguments --sysroot "${CMAKE_OSX_SYSROOT}")
    endif()
    file(MAKE_DIRECTORY "${prefix}/include/opus")
    add_custom_command(OUTPUT "${stamp}"
        COMMAND ${Python3_EXECUTABLE} -B "${root}/tools/macos/build_opus.py"
            --work "${work}" --prefix "${prefix}" ${arguments}
            --deployment-target "${CMAKE_OSX_DEPLOYMENT_TARGET}" --jobs "${HALO2_MACOS_DEPENDENCY_JOBS}"
        BYPRODUCTS "${library}" "${prefix}/lib/libopus.dylib"
            "${prefix}/include/opus/opus.h" "${prefix}/manifest.json"
            "${prefix}/share/licenses/Opus.txt" "${prefix}/lib/pkgconfig/opus.pc"
        DEPENDS "${root}/tools/macos/build_opus.py" "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
            "${root}/cmake/macos/DependencyToolchain.cmake"
        COMMENT "Building pinned Opus for the requested macOS architectures" VERBATIM)
    add_custom_target(halo2_macos_opus_sdk DEPENDS "${stamp}")
    add_library(PkgConfig::OPUS SHARED IMPORTED GLOBAL)
    set_target_properties(PkgConfig::OPUS PROPERTIES IMPORTED_LOCATION "${library}"
        IMPORTED_SONAME "@rpath/libopus.0.dylib"
        INTERFACE_INCLUDE_DIRECTORIES "${prefix}/include;${prefix}/include/opus")
    add_dependencies(PkgConfig::OPUS halo2_macos_opus_sdk)
    set(OPUS_VERSION "1.6.1" PARENT_SCOPE)
    set(OPUS_INCLUDE_DIRS "${prefix}/include;${prefix}/include/opus" PARENT_SCOPE)
    set(HALO2_MACOS_ENGINE_LICENSES ${HALO2_MACOS_ENGINE_LICENSES}
        "Opus=${prefix}/share/licenses/Opus.txt" PARENT_SCOPE)
endfunction()
