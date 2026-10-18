# Minimal standalone comparisons; no halo2_systems dependency is needed.
if(BUILD_TESTING)
    add_executable(native_tu15_renderer_cache_pools_tests
        tests/native_tu15_renderer_cache_pools_tests.cpp
        src/native_tu15_renderer_cache_pools_data.cpp
        src/source_renderer_transparent_storage_tu15_generated.cpp
        src/native_tu15_network_storage_data.cpp
        src/native_tu15_renderer_pool_data.cpp
        src/source_runtime_memory.cpp
        src/source_runtime_host_storage.cpp
        src/physics_pool_host_storage.cpp)
    add_executable(native_tu15_renderer_tail_storage_tests
        tests/native_tu15_renderer_tail_storage_tests.cpp
        src/source_renderer_cpu_interface_tu15.cpp
        src/source_renderer_tail_storage_tu15_generated.cpp
        src/native_tu15_renderer_tail_storage_data.cpp
        src/source_runtime_memory.cpp)
    foreach(target native_tu15_renderer_cache_pools_tests native_tu15_renderer_tail_storage_tests)
        target_include_directories(${target} PRIVATE include src)
        target_link_libraries(${target} PRIVATE halo2_physics_world_runtime halo2_snapshot_storage
            nlohmann_json::nlohmann_json Threads::Threads)
        target_compile_options(${target} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    endforeach()
    add_test(NAME original_tu15_renderer_cache_pools_constructor
        COMMAND native_tu15_renderer_cache_pools_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-renderer-cache-pools/original-differential/report.json)
    add_test(NAME original_tu15_renderer_tail_storage_constructor
        COMMAND native_tu15_renderer_tail_storage_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-renderer-tail-storage/original-differential/report.json)
    add_test(NAME original_tu15_renderer_cpu_interface_prefix
        COMMAND native_tu15_renderer_tail_storage_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-renderer-tail-storage/original-differential/report.json
        --native-prefix)
    set_tests_properties(original_tu15_renderer_cache_pools_constructor
        original_tu15_renderer_tail_storage_constructor PROPERTIES TIMEOUT 60)
    add_executable(native_lens_query_scalar_tests tests/native_lens_query_scalar_tests.cpp)
    target_link_libraries(native_lens_query_scalar_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    target_compile_options(native_lens_query_scalar_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME native_tu15_lens_query_scalar_ownership COMMAND native_lens_query_scalar_tests)
    set(HALO2_TU15_GAMEPLAY_XBE "" CACHE FILEPATH "Owned original English TU1.5 gameplay executable for acceptance tests")
    add_test(NAME original_tu15_lens_query_scalar COMMAND native_lens_query_scalar_tests
        "${HALO2_TU15_GAMEPLAY_XBE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_lens_query_scalar_tu15_original.json")
    set_tests_properties(native_tu15_lens_query_scalar_ownership original_tu15_lens_query_scalar
        PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77)
endif()
