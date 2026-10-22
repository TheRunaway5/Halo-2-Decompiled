include_guard(GLOBAL)
function(halo2_macos_openssl_dependency)
    if(TARGET OpenSSL::Crypto OR TARGET OpenSSL::SSL)
        message(FATAL_ERROR "Select the macOS OpenSSL SDK before importing another OpenSSL")
    endif()
    if(NOT APPLE OR NOT CMAKE_OSX_ARCHITECTURES)
        message(FATAL_ERROR "The macOS OpenSSL dependency needs explicit Mac architectures")
    endif()
    get_filename_component(root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../.." ABSOLUTE)
    set(prefix "${CMAKE_BINARY_DIR}/macos-dependencies/openssl")
    set(work "${CMAKE_BINARY_DIR}/macos-dependencies/openssl-work")
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
        COMMAND ${Python3_EXECUTABLE} -B "${root}/tools/macos/build_openssl.py"
            --work "${work}" --prefix "${prefix}" ${arguments}
            --deployment-target "${CMAKE_OSX_DEPLOYMENT_TARGET}" --jobs "${HALO2_MACOS_DEPENDENCY_JOBS}"
        BYPRODUCTS "${prefix}/lib/libcrypto.3.dylib" "${prefix}/lib/libssl.3.dylib"
            "${prefix}/lib/libcrypto.dylib" "${prefix}/lib/libssl.dylib"
            "${prefix}/include/openssl/evp.h" "${prefix}/include/openssl/ssl.h"
            "${prefix}/manifest.json" "${prefix}/share/licenses/OpenSSL.txt"
        DEPENDS "${root}/tools/macos/build_openssl.py" "${root}/tools/macos/bundle.py"
            "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
        COMMENT "Building pinned OpenSSL for the requested macOS architectures" VERBATIM)
    add_custom_target(halo2_macos_openssl_sdk DEPENDS "${stamp}")
    foreach(component Crypto SSL)
        string(TOLOWER "${component}" name)
        add_library(OpenSSL::${component} SHARED IMPORTED GLOBAL)
        set_target_properties(OpenSSL::${component} PROPERTIES
            IMPORTED_LOCATION "${prefix}/lib/lib${name}.3.dylib"
            IMPORTED_SONAME "@rpath/lib${name}.3.dylib"
            INTERFACE_INCLUDE_DIRECTORIES "${prefix}/include")
        add_dependencies(OpenSSL::${component} halo2_macos_openssl_sdk)
    endforeach()
    set_property(TARGET OpenSSL::SSL PROPERTY INTERFACE_LINK_LIBRARIES OpenSSL::Crypto)
    # libdatachannel repeats find_package(OpenSSL) during configuration, before
    # generated headers exist. CMake's redirect mechanism keeps that lookup on
    # these real SDK targets instead of rediscovering thin Homebrew libraries.
    include(CMakePackageConfigHelpers)
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/OpenSSLConfig.cmake.in"
        "${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/openssl-config.cmake" @ONLY)
    write_basic_package_version_file("${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/openssl-config-version.cmake"
        VERSION "3.5.9" COMPATIBILITY AnyNewerVersion)
    set(OPENSSL_FOUND TRUE PARENT_SCOPE)
    set(OpenSSL_FOUND TRUE PARENT_SCOPE)
    set(OPENSSL_VERSION "3.5.9" PARENT_SCOPE)
    set(OpenSSL_VERSION "3.5.9" PARENT_SCOPE)
    set(OPENSSL_INCLUDE_DIR "${prefix}/include" PARENT_SCOPE)
    set(OPENSSL_ROOT_DIR "${prefix}" PARENT_SCOPE)
    set(OPENSSL_CRYPTO_LIBRARY "${prefix}/lib/libcrypto.3.dylib" PARENT_SCOPE)
    set(OPENSSL_SSL_LIBRARY "${prefix}/lib/libssl.3.dylib" PARENT_SCOPE)
    set(HALO2_MACOS_ENGINE_LICENSES ${HALO2_MACOS_ENGINE_LICENSES}
        "OpenSSL=${prefix}/share/licenses/OpenSSL.txt" PARENT_SCOPE)
endfunction()
