include_guard(GLOBAL)
function(halo2_macos_libdeflate_dependency)
    if(TARGET PkgConfig::LIBDEFLATE)
        message(FATAL_ERROR "Select the owned libdeflate SDK before importing another libdeflate")
    endif()
    if(NOT APPLE OR NOT CMAKE_OSX_ARCHITECTURES)
        message(FATAL_ERROR "The macOS libdeflate dependency needs explicit Mac architectures")
    endif()
    get_filename_component(root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../.." ABSOLUTE)
    set(prefix "${CMAKE_BINARY_DIR}/macos-dependencies/libdeflate")
    set(work "${CMAKE_BINARY_DIR}/macos-dependencies/libdeflate-work")
    set(library "${prefix}/lib/libdeflate.0.dylib")
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
    add_custom_command(OUTPUT "${stamp}"
        COMMAND ${Python3_EXECUTABLE} -B "${root}/tools/macos/build_libdeflate.py"
            --work "${work}" --prefix "${prefix}" ${arguments}
            --deployment-target "${CMAKE_OSX_DEPLOYMENT_TARGET}" --jobs "${HALO2_MACOS_DEPENDENCY_JOBS}"
        BYPRODUCTS "${library}" "${prefix}/lib/libdeflate.dylib"
            "${prefix}/include/libdeflate.h" "${prefix}/manifest.json"
            "${prefix}/share/licenses/Libdeflate.txt"
        DEPENDS "${root}/tools/macos/build_libdeflate.py" "${root}/tools/macos/bundle.py"
            "${root}/cmake/macos/DependencyToolchain.cmake" "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
        COMMENT "Building pinned libdeflate for the requested macOS architectures" VERBATIM)
    add_custom_target(halo2_macos_libdeflate_sdk DEPENDS "${stamp}")
    add_library(PkgConfig::LIBDEFLATE SHARED IMPORTED GLOBAL)
    set_target_properties(PkgConfig::LIBDEFLATE PROPERTIES IMPORTED_LOCATION "${library}"
        IMPORTED_SONAME "@rpath/libdeflate.0.dylib" INTERFACE_INCLUDE_DIRECTORIES "${prefix}/include")
    add_dependencies(PkgConfig::LIBDEFLATE halo2_macos_libdeflate_sdk)
    set(HALO2_MACOS_ENGINE_LICENSES ${HALO2_MACOS_ENGINE_LICENSES}
        "Libdeflate=${prefix}/share/licenses/Libdeflate.txt" PARENT_SCOPE)
endfunction()
