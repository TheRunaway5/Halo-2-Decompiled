include_guard(GLOBAL)
function(halo2_macos_sdl_dependency)
    if(TARGET SDL2::SDL2)
        message(FATAL_ERROR "Select the owned SDL SDK before importing another SDL")
    endif()
    if(NOT APPLE OR NOT CMAKE_OSX_ARCHITECTURES)
        message(FATAL_ERROR "The macOS SDL dependency needs explicit Mac architectures")
    endif()
    get_filename_component(root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../.." ABSOLUTE)
    set(prefix "${CMAKE_BINARY_DIR}/macos-dependencies/sdl")
    set(work "${CMAKE_BINARY_DIR}/macos-dependencies/sdl-work")
    set(library "${prefix}/lib/libSDL2-2.0.0.dylib")
    set(stamp "${prefix}/built.stamp")
    set(HALO2_MACOS_DEPENDENCY_JOBS 2 CACHE STRING "Jobs within serial macOS dependency builds")
    set(arguments)
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        list(APPEND arguments --arch "${arch}")
    endforeach()
    if(CMAKE_OSX_SYSROOT)
        list(APPEND arguments --sysroot "${CMAKE_OSX_SYSROOT}")
    endif()
    file(MAKE_DIRECTORY "${prefix}/include/SDL2")
    add_custom_command(OUTPUT "${stamp}"
        COMMAND ${Python3_EXECUTABLE} -B "${root}/tools/macos/build_sdl.py"
            --work "${work}" --prefix "${prefix}" ${arguments}
            --deployment-target "${CMAKE_OSX_DEPLOYMENT_TARGET}" --jobs "${HALO2_MACOS_DEPENDENCY_JOBS}"
        BYPRODUCTS "${library}" "${prefix}/lib/libSDL2.dylib"
            "${prefix}/include/SDL2/SDL.h" "${prefix}/include/SDL2/SDL_config.h"
            "${prefix}/manifest.json" "${prefix}/share/licenses/SDL2.txt"
        DEPENDS "${root}/tools/macos/build_sdl.py" "${root}/tools/macos/bundle.py"
            "${root}/cmake/macos/DependencyToolchain.cmake" "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
        COMMENT "Building pinned SDL for the requested macOS architectures" VERBATIM)
    add_custom_target(halo2_macos_sdl_sdk DEPENDS "${stamp}")
    add_library(SDL2::SDL2 SHARED IMPORTED GLOBAL)
    set_target_properties(SDL2::SDL2 PROPERTIES IMPORTED_LOCATION "${library}"
        IMPORTED_SONAME "@rpath/libSDL2-2.0.0.dylib"
        INTERFACE_INCLUDE_DIRECTORIES "${prefix}/include/SDL2")
    add_dependencies(SDL2::SDL2 halo2_macos_sdl_sdk)
    set(SDL2_VERSION "2.32.10" PARENT_SCOPE)
    set(HALO2_SDL_LICENSE "${prefix}/share/licenses/SDL2.txt" PARENT_SCOPE)
endfunction()
