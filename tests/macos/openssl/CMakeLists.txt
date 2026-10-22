cmake_minimum_required(VERSION 3.24)
get_filename_component(HALO2_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
include("${HALO2_ROOT}/cmake/macos/Defaults.cmake")
project(Halo2MacOSOpenSSLTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(Threads REQUIRED)
find_package(CURL 7.86 REQUIRED)
find_package(nlohmann_json REQUIRED CONFIG)
include("${HALO2_ROOT}/cmake/TestStorage.cmake")
include("${HALO2_ROOT}/cmake/macos/OpenSSL.cmake")
include(CTest)
halo2_macos_openssl_dependency()
# Exercise the later dependency lookup while its headers/libraries are still
# generated build outputs, just as libdatachannel configures in the engine.
find_package(OpenSSL 3.5 REQUIRED COMPONENTS Crypto SSL)
add_library(identity STATIC "${HALO2_ROOT}/src/network/services/discord_login.cpp" "${HALO2_ROOT}/src/network/services/steam_login.cpp")
target_include_directories(identity PUBLIC "${HALO2_ROOT}/include" "${HALO2_ROOT}/src")
target_link_libraries(identity PUBLIC CURL::libcurl OpenSSL::Crypto nlohmann_json::nlohmann_json Threads::Threads)
foreach(service steam discord)
    add_executable(${service}_login_tests "${HALO2_ROOT}/tests/${service}_login_tests.cpp")
    target_link_libraries(${service}_login_tests PRIVATE identity)
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        add_test(NAME macos_openssl_${service}_${arch} COMMAND /usr/bin/arch -${arch} $<TARGET_FILE:${service}_login_tests>)
        set_tests_properties(macos_openssl_${service}_${arch} PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
    endforeach()
endforeach()
add_executable(macos_openssl_dependency_tests MACOSX_BUNDLE dependency_tests.cpp)
target_link_libraries(macos_openssl_dependency_tests PRIVATE OpenSSL::SSL)
set_target_properties(macos_openssl_dependency_tests PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER net.halo2forever.openssl-dependency-check)
set(arch_args)
foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
    list(APPEND arch_args --arch "${arch}")
endforeach()
add_custom_command(TARGET macos_openssl_dependency_tests POST_BUILD
    COMMAND ${Python3_EXECUTABLE} -B "${HALO2_ROOT}/tools/macos/bundle.py"
        --app "$<TARGET_BUNDLE_DIR:macos_openssl_dependency_tests>" ${arch_args}
        --license "OpenSSL=${CMAKE_BINARY_DIR}/macos-dependencies/openssl/share/licenses/OpenSSL.txt" VERBATIM)
foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
    add_test(NAME macos_openssl_crypto_tls_${arch} COMMAND /usr/bin/arch -${arch}
        $<TARGET_FILE:macos_openssl_dependency_tests>
        "$<TARGET_BUNDLE_CONTENT_DIR:macos_openssl_dependency_tests>/Frameworks")
endforeach()
add_test(NAME macos_openssl_relocation COMMAND ${Python3_EXECUTABLE} -B
    "${CMAKE_CURRENT_SOURCE_DIR}/relocation_test.py"
    "$<TARGET_BUNDLE_DIR:macos_openssl_dependency_tests>" "${CMAKE_BINARY_DIR}/macos-dependencies/openssl"
    "${CMAKE_BINARY_DIR}/openssl-relocation.json" ${CMAKE_OSX_ARCHITECTURES})
set_tests_properties(macos_openssl_relocation PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
add_custom_target(macos_openssl_tests DEPENDS steam_login_tests discord_login_tests macos_openssl_dependency_tests)
