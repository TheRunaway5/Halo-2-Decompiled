include_guard(GLOBAL)
function(halo2_macos_ffmpeg_dependency)
    if(TARGET PkgConfig::AVCODEC)
        return()
    endif()
    if(NOT APPLE OR NOT CMAKE_OSX_ARCHITECTURES)
        message(FATAL_ERROR "The macOS playback dependency needs explicit Mac architectures")
    endif()
    get_filename_component(root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../.." ABSOLUTE)
    set(prefix "${CMAKE_BINARY_DIR}/macos-dependencies/ffmpeg")
    set(work "${CMAKE_BINARY_DIR}/macos-dependencies/ffmpeg-work")
    set(stamp "${prefix}/built.stamp")
    set(HALO2_MACOS_DEPENDENCY_JOBS 2 CACHE STRING "Jobs within serial macOS dependency builds")
    set(arguments)
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        list(APPEND arguments --arch "${arch}")
    endforeach()
    if(CMAKE_OSX_SYSROOT)
        list(APPEND arguments --sysroot "${CMAKE_OSX_SYSROOT}")
    endif()
    file(MAKE_DIRECTORY "${prefix}/include")
    set(libraries libavcodec.63.dylib libavformat.63.dylib libavutil.61.dylib
        libswresample.7.dylib libswscale.10.dylib libdav1d.7.dylib)
    set(outputs)
    foreach(library IN LISTS libraries)
        list(APPEND outputs "${prefix}/lib/${library}")
    endforeach()
    add_custom_command(OUTPUT "${stamp}"
        COMMAND ${Python3_EXECUTABLE} -B "${root}/tools/macos/build_ffmpeg.py"
            --work "${work}" --prefix "${prefix}" ${arguments}
            --deployment-target "${CMAKE_OSX_DEPLOYMENT_TARGET}" --jobs "${HALO2_MACOS_DEPENDENCY_JOBS}"
        BYPRODUCTS ${outputs} "${prefix}/manifest.json"
            "${prefix}/share/licenses/FFmpeg-COPYING.LGPLv2.1"
            "${prefix}/share/licenses/FFmpeg-COPYING.LGPLv3"
            "${prefix}/share/licenses/FFmpeg-LICENSE.md" "${prefix}/share/licenses/Dav1d.txt"
        DEPENDS "${root}/tools/macos/build_ffmpeg.py" "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
        COMMENT "Building pinned FFmpeg/dav1d for the requested macOS architectures" VERBATIM)
    add_custom_target(halo2_macos_ffmpeg_sdk DEPENDS "${stamp}")
    set(targets)
    foreach(name avutil swresample avcodec avformat swscale)
        if(name STREQUAL "avutil")
            set(major 61)
        elseif(name STREQUAL "swresample")
            set(major 7)
        elseif(name STREQUAL "swscale")
            set(major 10)
        else()
            set(major 63)
        endif()
        add_library(halo2_macos_${name} SHARED IMPORTED GLOBAL)
        set_target_properties(halo2_macos_${name} PROPERTIES
            IMPORTED_LOCATION "${prefix}/lib/lib${name}.${major}.dylib"
            IMPORTED_SONAME "@rpath/lib${name}.${major}.dylib"
            INTERFACE_INCLUDE_DIRECTORIES "${prefix}/include")
        add_dependencies(halo2_macos_${name} halo2_macos_ffmpeg_sdk)
        list(APPEND targets halo2_macos_${name})
    endforeach()
    add_library(PkgConfig::AVCODEC INTERFACE IMPORTED GLOBAL)
    set_target_properties(PkgConfig::AVCODEC PROPERTIES INTERFACE_LINK_LIBRARIES "${targets}")
    add_dependencies(PkgConfig::AVCODEC halo2_macos_ffmpeg_sdk)
    set(AVCODEC_INCLUDE_DIRS "${prefix}/include" PARENT_SCOPE)
    set(HALO2_MACOS_ENGINE_LICENSES ${HALO2_MACOS_ENGINE_LICENSES}
        "FFmpeg-LGPL-2.1=${prefix}/share/licenses/FFmpeg-COPYING.LGPLv2.1"
        "FFmpeg-LGPL-3=${prefix}/share/licenses/FFmpeg-COPYING.LGPLv3"
        "FFmpeg-notices=${prefix}/share/licenses/FFmpeg-LICENSE.md"
        "Dav1d=${prefix}/share/licenses/Dav1d.txt" PARENT_SCOPE)
endfunction()
