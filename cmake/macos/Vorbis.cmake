include_guard(GLOBAL)
function(halo2_macos_vorbis_dependency)
    if(TARGET PkgConfig::VORBISFILE)
        return()
    endif()
    if(NOT APPLE OR NOT CMAKE_OSX_ARCHITECTURES)
        message(FATAL_ERROR "The macOS Vorbis dependency needs explicit Mac architectures")
    endif()
    get_filename_component(root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../.." ABSOLUTE)
    set(prefix "${CMAKE_BINARY_DIR}/macos-dependencies/vorbis")
    set(work "${CMAKE_BINARY_DIR}/macos-dependencies/vorbis-work")
    set(stamp "${prefix}/built.stamp")
    set(HALO2_MACOS_DEPENDENCY_JOBS 2 CACHE STRING "Jobs within serial macOS dependency builds")
    set(arguments)
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        list(APPEND arguments --arch "${arch}")
    endforeach()
    if(CMAKE_OSX_SYSROOT)
        list(APPEND arguments --sysroot "${CMAKE_OSX_SYSROOT}")
    endif()
    file(MAKE_DIRECTORY "${prefix}/include/ogg" "${prefix}/include/vorbis")
    add_custom_command(OUTPUT "${stamp}"
        COMMAND ${Python3_EXECUTABLE} -B "${root}/tools/macos/build_vorbis.py"
            --work "${work}" --prefix "${prefix}" ${arguments}
            --deployment-target "${CMAKE_OSX_DEPLOYMENT_TARGET}" --jobs "${HALO2_MACOS_DEPENDENCY_JOBS}"
        BYPRODUCTS "${prefix}/lib/libogg.0.dylib" "${prefix}/lib/libvorbis.0.dylib"
            "${prefix}/lib/libvorbisenc.2.dylib" "${prefix}/lib/libvorbisfile.3.dylib"
            "${prefix}/include/ogg/ogg.h" "${prefix}/include/vorbis/codec.h"
            "${prefix}/manifest.json" "${prefix}/share/licenses/Ogg.txt"
            "${prefix}/share/licenses/Vorbis.txt"
        DEPENDS "${root}/tools/macos/build_vorbis.py" "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
            "${root}/cmake/macos/DependencyToolchain.cmake"
        COMMENT "Building pinned Ogg/Vorbis for the requested macOS architectures" VERBATIM)
    add_custom_target(halo2_macos_vorbis_sdk DEPENDS "${stamp}")
    foreach(spec "ogg:0" "vorbis:0" "vorbisenc:2" "vorbisfile:3")
        string(REPLACE ":" ";" pair "${spec}")
        list(GET pair 0 name)
        list(GET pair 1 abi)
        add_library(halo2_macos_${name} SHARED IMPORTED GLOBAL)
        set_target_properties(halo2_macos_${name} PROPERTIES
            IMPORTED_LOCATION "${prefix}/lib/lib${name}.${abi}.dylib"
            IMPORTED_SONAME "@rpath/lib${name}.${abi}.dylib"
            INTERFACE_INCLUDE_DIRECTORIES "${prefix}/include")
        add_dependencies(halo2_macos_${name} halo2_macos_vorbis_sdk)
    endforeach()
    set_property(TARGET halo2_macos_vorbis PROPERTY INTERFACE_LINK_LIBRARIES halo2_macos_ogg)
    foreach(name vorbisenc vorbisfile)
        set_property(TARGET halo2_macos_${name} PROPERTY INTERFACE_LINK_LIBRARIES halo2_macos_vorbis)
    endforeach()
    # The existing converter uses the packet/synthesis API (vorbis + ogg).
    add_library(PkgConfig::VORBISFILE INTERFACE IMPORTED GLOBAL)
    set_property(TARGET PkgConfig::VORBISFILE PROPERTY INTERFACE_LINK_LIBRARIES halo2_macos_vorbis)
    set(HALO2_MACOS_ENGINE_LICENSES ${HALO2_MACOS_ENGINE_LICENSES}
        "Ogg=${prefix}/share/licenses/Ogg.txt" "Vorbis=${prefix}/share/licenses/Vorbis.txt" PARENT_SCOPE)
endfunction()
