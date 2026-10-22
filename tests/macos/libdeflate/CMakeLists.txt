cmake_minimum_required(VERSION 3.24)
get_filename_component(HALO2_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
include("${HALO2_ROOT}/cmake/macos/Defaults.cmake")
project(Halo2MacOSDecompressionTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)
find_package(nlohmann_json REQUIRED CONFIG)
include("${HALO2_ROOT}/cmake/Glm.cmake")
include("${HALO2_ROOT}/cmake/TestStorage.cmake")
include("${HALO2_ROOT}/cmake/MccDecompression.cmake")
include(CTest)

add_library(accelerated_decoder OBJECT "${HALO2_ROOT}/src/assets/mcc_cache_decode.cpp")
target_include_directories(accelerated_decoder PRIVATE "${HALO2_ROOT}/include" "${HALO2_ROOT}/src")
target_link_libraries(accelerated_decoder PRIVATE halo2_glm ZLIB::ZLIB Threads::Threads)
halo2_mcc_decompression(accelerated_decoder)
get_target_property(decoder_definitions accelerated_decoder COMPILE_DEFINITIONS)
if(NOT "HALO2_HAVE_LIBDEFLATE=1" IN_LIST decoder_definitions)
    message(FATAL_ERROR "This cohort must exercise the production accelerated decoder")
endif()
# Observe calls by forwarding every parameter and the real library result.
# Neither decoded bytes nor source admission/results are supplied by this hook.
target_compile_definitions(accelerated_decoder PRIVATE
    libdeflate_zlib_decompress_ex=halo2_test_libdeflate_decompress)

set(reference_file "${HALO2_ROOT}/analysis/five-second-loading-20261020/original-map-decoder.txt")
file(SHA256 "${reference_file}" serial_sha)
if(NOT serial_sha STREQUAL "50cb564a9bbecc95e3ba8fbbeeeb16604b4bd57e7fb881ff00c161f38886d577")
    message(FATAL_ERROR "Retained pre-optimization MCC decoder changed")
endif()
file(READ "${reference_file}" serial)
string(REPLACE "decode_mcc_cache(" "decode_mcc_cache_reference(" serial "${serial}")
file(READ "${HALO2_ROOT}/src/assets/mcc_cache.cpp" mcc_source)
string(FIND "${mcc_source}" "std::vector<std::byte> inflate(" start)
string(FIND "${mcc_source}" "void word(" finish)
if(start LESS 0 OR finish LESS start)
    message(FATAL_ERROR "Cannot locate the retained production zlib inflater")
endif()
math(EXPR length "${finish}-${start}")
string(SUBSTRING "${mcc_source}" ${start} ${length} inflater)
file(WRITE "${CMAKE_BINARY_DIR}/serial_decoder.cpp"
    "#include \"halo2/cache.hpp\"\n#include <algorithm>\n#include <bit>\n#include <zlib.h>\nnamespace halo2 {namespace {constexpr std::size_t header_size=896,maximum_cache=1024ull*1024*1024;\n${inflater}\n}\n${serial}\n}\n")
file(SHA256 "${HALO2_ROOT}/src/assets/mcc_cache_decode.cpp" decoder_sha)
file(SHA256 "${HALO2_ROOT}/src/assets/cache.cpp" bytes_sha)
string(SHA256 inflater_sha "${inflater}")
file(WRITE "${CMAKE_BINARY_DIR}/decoder-sources.json"
    "{\"serial_sha256\":\"${serial_sha}\",\"inflater_sha256\":\"${inflater_sha}\",\"production_decoder_sha256\":\"${decoder_sha}\",\"bytes_owner_sha256\":\"${bytes_sha}\"}\n")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${reference_file}" "${HALO2_ROOT}/src/assets/mcc_cache.cpp")
add_library(decoder_support OBJECT "${HALO2_ROOT}/src/assets/cache.cpp"
    "${CMAKE_BINARY_DIR}/serial_decoder.cpp" decoder_observer.cpp)
target_include_directories(decoder_support PRIVATE "${HALO2_ROOT}/include" "${HALO2_ROOT}/src")
target_link_libraries(decoder_support PRIVATE halo2_glm ZLIB::ZLIB PkgConfig::LIBDEFLATE)
target_compile_options(decoder_support PRIVATE -ffunction-sections -fdata-sections)

set(HALO2_LIBDEFLATE_TEST_MAP "" CACHE FILEPATH "Optional owned MCC map for complete byte comparison")
foreach(target mcc_decoder_contract_tests macos_libdeflate_dependency_tests)
    if(target STREQUAL "mcc_decoder_contract_tests")
        set(source "${HALO2_ROOT}/tests/mcc_cache_decode_tests.cpp")
    else()
        set(source dependency_tests.cpp)
    endif()
    add_executable(${target} ${source})
    target_include_directories(${target} PRIVATE "${HALO2_ROOT}/include" "${HALO2_ROOT}/src")
    target_compile_definitions(${target} PRIVATE HALO2_DECODER_REFERENCE=1)
    target_link_libraries(${target} PRIVATE accelerated_decoder decoder_support
        halo2_glm ZLIB::ZLIB Threads::Threads PkgConfig::LIBDEFLATE nlohmann_json::nlohmann_json)
    if(APPLE)
        target_link_options(${target} PRIVATE -Wl,-dead_strip)
    else()
        find_package(OpenSSL REQUIRED COMPONENTS Crypto)
        target_link_libraries(${target} PRIVATE OpenSSL::Crypto ${CMAKE_DL_LIBS})
        target_link_options(${target} PRIVATE -Wl,--gc-sections)
    endif()
endforeach()
if(APPLE)
    set_target_properties(macos_libdeflate_dependency_tests PROPERTIES MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER net.halo2forever.libdeflate-dependency-check)
    set(arch_args)
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        list(APPEND arch_args --arch "${arch}")
    endforeach()
    add_custom_command(TARGET macos_libdeflate_dependency_tests POST_BUILD
        COMMAND ${Python3_EXECUTABLE} -B "${HALO2_ROOT}/tools/macos/bundle.py"
            --app "$<TARGET_BUNDLE_DIR:macos_libdeflate_dependency_tests>" ${arch_args}
            --license "Libdeflate=${CMAKE_BINARY_DIR}/macos-dependencies/libdeflate/share/licenses/Libdeflate.txt"
        VERBATIM)
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        add_test(NAME macos_libdeflate_contract_${arch} COMMAND /usr/bin/arch -${arch}
            $<TARGET_FILE:mcc_decoder_contract_tests>)
        add_test(NAME macos_libdeflate_dependency_${arch} COMMAND /usr/bin/arch -${arch}
            $<TARGET_FILE:macos_libdeflate_dependency_tests>
            "$<TARGET_BUNDLE_CONTENT_DIR:macos_libdeflate_dependency_tests>/Frameworks")
        set_tests_properties(macos_libdeflate_contract_${arch} macos_libdeflate_dependency_${arch}
            PROPERTIES RUN_SERIAL TRUE TIMEOUT 180)
    endforeach()
    add_test(NAME macos_libdeflate_relocation COMMAND ${Python3_EXECUTABLE} -B
        "${CMAKE_CURRENT_SOURCE_DIR}/relocation_test.py" "$<TARGET_BUNDLE_DIR:macos_libdeflate_dependency_tests>"
        "${CMAKE_BINARY_DIR}/macos-dependencies/libdeflate" "${CMAKE_BINARY_DIR}/libdeflate-relocation.json"
        "${HALO2_LIBDEFLATE_TEST_MAP}" ${CMAKE_OSX_ARCHITECTURES})
    set_tests_properties(macos_libdeflate_relocation PROPERTIES RUN_SERIAL TRUE TIMEOUT 180)
else()
    pkg_get_variable(host_libdeflate_directory libdeflate libdir)
    add_test(NAME libdeflate_contract COMMAND mcc_decoder_contract_tests)
    add_test(NAME libdeflate_dependency COMMAND macos_libdeflate_dependency_tests
        "${host_libdeflate_directory}" "${HALO2_LIBDEFLATE_TEST_MAP}")
endif()
add_custom_target(macos_libdeflate_tests DEPENDS mcc_decoder_contract_tests macos_libdeflate_dependency_tests)
