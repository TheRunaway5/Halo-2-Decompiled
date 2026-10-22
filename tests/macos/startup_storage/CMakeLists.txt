cmake_minimum_required(VERSION 3.24)
get_filename_component(HALO2_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
if(APPLE)
    include("${HALO2_ROOT}/cmake/macos/Defaults.cmake")
endif()

project(Halo2NativeStartupStorage LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(Threads REQUIRED)
include("${HALO2_ROOT}/cmake/TestStorage.cmake")
include(CTest)
add_library(startup_storage_owners OBJECT
    "${HALO2_ROOT}/src/physics/physics_game_arena_native.cpp"
    "${HALO2_ROOT}/src/gameplay/units/death_startup_native.cpp")
target_include_directories(startup_storage_owners PRIVATE "${HALO2_ROOT}/include" "${HALO2_ROOT}/src")
add_executable(startup_storage_host_tests host_tests.cpp)
target_include_directories(startup_storage_host_tests PRIVATE "${HALO2_ROOT}/src")
target_link_libraries(startup_storage_host_tests PRIVATE Threads::Threads)
add_custom_target(startup_storage_tests DEPENDS startup_storage_owners startup_storage_host_tests)
if(APPLE)
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        add_test(NAME startup_storage_host_${arch}
            COMMAND /usr/bin/arch -${arch} $<TARGET_FILE:startup_storage_host_tests>)
    endforeach()
else()
    add_test(NAME startup_storage_host COMMAND startup_storage_host_tests)
endif()

# The original arena executor still includes the deliberately deferred exact
# x87/SSE backend. Keep its real no-asset startup check opt-in and x86-only;
# the host resources and production owner compilation above cover both CPUs.
option(HALO2_STARTUP_ARENA_TEST "Execute the original arena startup on x86_64" OFF)
if(HALO2_STARTUP_ARENA_TEST)
    if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
        message(FATAL_ERROR "The arena executor requires the retained x86_64 numerical backend")
    endif()
    find_package(nlohmann_json REQUIRED CONFIG)
    if(APPLE)
        include("${HALO2_ROOT}/cmake/macos/OpenSSL.cmake")
        halo2_macos_openssl_dependency()
    else()
        find_package(OpenSSL REQUIRED COMPONENTS Crypto)
    endif()
    add_executable(startup_storage_arena_tests
        "${HALO2_ROOT}/tests/native_tu15_arena_data_tests.cpp")
    foreach(source IN ITEMS
        physics/native_tu15_arena_data physics/native_tu15_arena_bootstrap physics/physics_game_arena_native
        physics/physics_game_arena_runtime_generated physics/physics_physical_storage_runtime_generated
        physics/physics_physical_storage_host platform/native_tu15_heap_data platform/native_tu15_heap_bootstrap
        platform/source_process_heap_platform platform/source_process_heap_runtime_generated
        platform/source_process_shutdown platform/source_process_startup_records physics/physics_platform_call
        physics/physics_platform_locks physics/physics_platform_locks_continuation platform/native_synchronization
        platform/native_threads runtime/source_runtime_memory runtime/source_runtime_host_virtual_memory
        runtime/source_runtime_host_storage runtime/source_runtime_host_physical_memory runtime/source_runtime_image
        assets/cache_file_snapshot runtime/source_runtime_snapshot_bytes)
        target_sources(startup_storage_arena_tests PRIVATE "${HALO2_ROOT}/src/${source}.cpp")
    endforeach()
    target_include_directories(startup_storage_arena_tests PRIVATE "${HALO2_ROOT}/include" "${HALO2_ROOT}/src")
    target_link_libraries(startup_storage_arena_tests PRIVATE
        Threads::Threads nlohmann_json::nlohmann_json OpenSSL::Crypto)
    # Discard unused heap/world entry points, without changing the compiled
    # original arithmetic or supplying substitutes for their callees.
    target_compile_options(startup_storage_arena_tests PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)
    if(APPLE)
        target_link_options(startup_storage_arena_tests PRIVATE -Wl,-dead_strip)
        set(arena_test startup_storage_arena_x86_64)
        add_test(NAME ${arena_test} COMMAND /usr/bin/arch -x86_64
            $<TARGET_FILE:startup_storage_arena_tests> --capture "${CMAKE_BINARY_DIR}/arena.json")
    else()
        target_link_options(startup_storage_arena_tests PRIVATE -Wl,--gc-sections)
        set(arena_test startup_storage_arena)
        add_test(NAME ${arena_test} COMMAND startup_storage_arena_tests
            --capture "${CMAKE_BINARY_DIR}/arena.json")
    endif()
    set_tests_properties(${arena_test} PROPERTIES TIMEOUT 120)
    set(HALO2_STARTUP_REFERENCE_XBE "" CACHE FILEPATH "Optional authenticated TU1.5 reference for arena comparison")
    if(HALO2_STARTUP_REFERENCE_XBE)
        add_test(NAME startup_storage_arena_original COMMAND startup_storage_arena_tests
            --reference-xbe "${HALO2_STARTUP_REFERENCE_XBE}"
            --original-fixture "${HALO2_ROOT}/analysis/multiplayer/sandbox-source/physics-game-arena-tu15-20261012/original-v2/report.json"
            --capture "${CMAKE_BINARY_DIR}/arena-original.json")
        set_tests_properties(startup_storage_arena_original PROPERTIES TIMEOUT 120)
    endif()
    add_dependencies(startup_storage_tests startup_storage_arena_tests)
endif()
