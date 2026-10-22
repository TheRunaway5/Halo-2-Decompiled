cmake_minimum_required(VERSION 3.24)
get_filename_component(HALO2_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
include("${HALO2_ROOT}/cmake/macos/Defaults.cmake")
project(Halo2MacOSPlatformTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)
include("${HALO2_ROOT}/cmake/TestStorage.cmake")
include(CTest)

add_library(halo2_platform_memory STATIC
    "${HALO2_ROOT}/src/assets/cache_file_snapshot.cpp"
    "${HALO2_ROOT}/src/runtime/source_runtime_snapshot_bytes.cpp"
    "${HALO2_ROOT}/src/runtime/source_runtime_memory.cpp"
    "${HALO2_ROOT}/src/runtime/source_runtime_host_virtual_memory.cpp"
    "${HALO2_ROOT}/src/runtime/source_runtime_host_storage.cpp"
    "${HALO2_ROOT}/src/runtime/source_runtime_host_physical_memory.cpp"
    "${HALO2_ROOT}/src/gameplay/objects/object_body_heap.cpp"
    "${HALO2_ROOT}/src/physics/physics_pool_host_storage.cpp")
target_include_directories(halo2_platform_memory PUBLIC "${HALO2_ROOT}/include" "${HALO2_ROOT}/src")
target_link_libraries(halo2_platform_memory PUBLIC Threads::Threads)
add_executable(source_runtime_host_virtual_memory_tests
    "${HALO2_ROOT}/tests/source_runtime_host_virtual_memory_tests.cpp")
target_link_libraries(source_runtime_host_virtual_memory_tests PRIVATE halo2_platform_memory)
add_executable(macos_source_memory_tests source_memory_tests.cpp)
target_link_libraries(macos_source_memory_tests PRIVATE halo2_platform_memory)
target_include_directories(macos_source_memory_tests PRIVATE "${HALO2_ROOT}/src")
add_executable(macos_physical_memory_tests physical_memory_tests.cpp)
target_link_libraries(macos_physical_memory_tests PRIVATE halo2_platform_memory)
add_executable(source_physical_release_tests "${HALO2_ROOT}/tests/source_physical_release_tests.cpp")
target_link_libraries(source_physical_release_tests PRIVATE halo2_platform_memory)
add_executable(source_host_resource_continuation_tests
    "${HALO2_ROOT}/tests/source_host_resource_continuation_tests.cpp")
target_link_libraries(source_host_resource_continuation_tests PRIVATE halo2_platform_memory)
add_executable(cache_file_snapshot_tests "${HALO2_ROOT}/tests/cache_file_snapshot_tests.cpp")
target_link_libraries(cache_file_snapshot_tests PRIVATE halo2_platform_memory)
add_executable(source_runtime_snapshot_memory_tests
    "${HALO2_ROOT}/tests/source_runtime_snapshot_memory_tests.cpp")
target_link_libraries(source_runtime_snapshot_memory_tests PRIVATE halo2_platform_memory)
add_executable(macos_snapshot_storage_tests snapshot_storage_tests.cpp)
target_link_libraries(macos_snapshot_storage_tests PRIVATE halo2_platform_memory)
target_include_directories(macos_snapshot_storage_tests PRIVATE "${HALO2_ROOT}/src")
add_library(halo2_platform_network STATIC
    "${HALO2_ROOT}/src/network/native_tu15_network_service.cpp"
    "${HALO2_ROOT}/src/network/native_tu15_network_startup.cpp"
    "${HALO2_ROOT}/src/network/native_tu15_network_link.cpp"
    "${HALO2_ROOT}/src/core/native_tu15_title_address.cpp")
target_include_directories(halo2_platform_network PUBLIC "${HALO2_ROOT}/include" "${HALO2_ROOT}/src")
target_link_libraries(halo2_platform_network PUBLIC Threads::Threads halo2_platform_memory)
if(APPLE)
    include("${HALO2_ROOT}/cmake/macos/Network.cmake")
    halo2_macos_network_observation(halo2_platform_network)
endif()
add_executable(native_tu15_network_service_tests "${HALO2_ROOT}/tests/native_tu15_network_service_tests.cpp")
target_link_libraries(native_tu15_network_service_tests PRIVATE halo2_platform_network nlohmann_json::nlohmann_json)
add_executable(macos_datagram_tests datagram_tests.cpp)
target_include_directories(macos_datagram_tests PRIVATE "${HALO2_ROOT}/src")
target_link_libraries(macos_datagram_tests PRIVATE halo2_platform_network)
add_executable(macos_network_observation_tests network_observation_tests.cpp)
target_include_directories(macos_network_observation_tests PRIVATE "${HALO2_ROOT}/src")
target_link_libraries(macos_network_observation_tests PRIVATE halo2_platform_network)
add_library(halo2_platform_startup_timer STATIC "${HALO2_ROOT}/src/platform/native_tu15_startup_timer.cpp")
target_include_directories(halo2_platform_startup_timer PUBLIC "${HALO2_ROOT}/include" "${HALO2_ROOT}/src")
target_link_libraries(halo2_platform_startup_timer PUBLIC halo2_platform_memory)
add_library(halo2_platform_sound_owners OBJECT
    "${HALO2_ROOT}/src/audio/runtime/source_runtime_sound_event_port.cpp"
    "${HALO2_ROOT}/src/audio/runtime/source_runtime_sound_timed_notifications.cpp"
    "${HALO2_ROOT}/src/audio/runtime/source_runtime_sound_codec_event_port.cpp"
    "${HALO2_ROOT}/src/platform/native_tu15_startup_timer_execution.cpp")
target_include_directories(halo2_platform_sound_owners PRIVATE "${HALO2_ROOT}/include" "${HALO2_ROOT}/src" "${HALO2_ROOT}/src")
add_executable(macos_native_event_tests native_event_tests.cpp)
target_include_directories(macos_native_event_tests PRIVATE "${HALO2_ROOT}/src")
target_link_libraries(macos_native_event_tests PRIVATE halo2_platform_startup_timer)
add_dependencies(macos_native_event_tests halo2_platform_sound_owners)

if(APPLE)
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        add_test(NAME macos_source_memory_${arch} COMMAND /usr/bin/arch -${arch}
            $<TARGET_FILE:macos_source_memory_tests>)
        add_test(NAME source_runtime_virtual_memory_${arch} COMMAND /usr/bin/arch -${arch}
            $<TARGET_FILE:source_runtime_host_virtual_memory_tests>)
        foreach(target macos_physical_memory_tests source_physical_release_tests
                       source_host_resource_continuation_tests cache_file_snapshot_tests macos_snapshot_storage_tests
                       native_tu15_network_service_tests macos_datagram_tests macos_network_observation_tests macos_native_event_tests)
            add_test(NAME ${target}_${arch} COMMAND /usr/bin/arch -${arch} $<TARGET_FILE:${target}>)
        endforeach()
        foreach(snapshot_case checkpoint restore copy contracts storage adoption)
            add_test(NAME source_runtime_snapshot_${snapshot_case}_${arch} COMMAND /usr/bin/arch -${arch}
                $<TARGET_FILE:source_runtime_snapshot_memory_tests> ${snapshot_case})
        endforeach()
    endforeach()
else()
    add_test(NAME macos_source_memory_portability COMMAND macos_source_memory_tests)
    add_test(NAME source_runtime_virtual_memory COMMAND source_runtime_host_virtual_memory_tests)
    add_test(NAME macos_physical_memory_portability COMMAND macos_physical_memory_tests)
    add_test(NAME source_physical_release COMMAND source_physical_release_tests)
    add_test(NAME source_host_resource_continuation COMMAND source_host_resource_continuation_tests)
    add_test(NAME cache_file_snapshot COMMAND cache_file_snapshot_tests)
    add_test(NAME macos_snapshot_storage_portability COMMAND macos_snapshot_storage_tests)
    add_test(NAME native_tu15_network_service COMMAND native_tu15_network_service_tests)
    add_test(NAME macos_datagram_portability COMMAND macos_datagram_tests)
    add_test(NAME macos_network_observation_portability COMMAND macos_network_observation_tests)
    add_test(NAME macos_native_event_portability COMMAND macos_native_event_tests)
    foreach(snapshot_case checkpoint restore copy contracts storage adoption)
        add_test(NAME source_runtime_snapshot_${snapshot_case} COMMAND source_runtime_snapshot_memory_tests ${snapshot_case})
    endforeach()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_executable(source_physical_backing_tests "${HALO2_ROOT}/tests/source_physical_backing_tests.cpp")
        target_link_libraries(source_physical_backing_tests PRIVATE halo2_platform_memory)
        add_test(NAME source_physical_backing_protection COMMAND source_physical_backing_tests protection)
    endif()
endif()

# Every snapshot inode lives on the disposable build's disk, including after
# a killed test. The wrapper expands this token separately for each CTest.
get_property(platform_tests DIRECTORY PROPERTY TESTS)
set_tests_properties(${platform_tests} PROPERTIES
    ENVIRONMENT "XDG_CACHE_HOME=@HALO2_TEST_SCRATCH@/cache" TIMEOUT 180)
