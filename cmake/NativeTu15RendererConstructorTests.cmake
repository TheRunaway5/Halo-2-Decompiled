# Minimal standalone comparisons; no halo2_systems dependency is needed.
if(BUILD_TESTING)
    set(HALO2_TU15_GAMEPLAY_XBE "" CACHE FILEPATH "Owned original English TU1.5 gameplay executable for acceptance tests")
    set(_halo2_renderer_oracle_python "${Python3_EXECUTABLE}")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/bin/python")
        set(_halo2_renderer_oracle_python "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/bin/python")
    endif()
    set(HALO2_RENDERER_ORACLE_PYTHON "${_halo2_renderer_oracle_python}" CACHE FILEPATH
        "Python with Unicorn for offline native renderer comparisons")
    add_executable(native_tu15_lens_query_slots_tests
        tests/native_tu15_lens_query_slots_tests.cpp
        src/rendering/effects/native_tu15_lens_query_slots.cpp)
    target_include_directories(native_tu15_lens_query_slots_tests PRIVATE include)
    target_compile_features(native_tu15_lens_query_slots_tests PRIVATE cxx_std_20)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_tu15_lens_query_slots_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_lens_query_slots_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_tu15_lens_query_slots COMMAND native_tu15_lens_query_slots_tests)
    add_executable(native_tu15_lens_query_slot_bridge_tests
        tests/native_tu15_lens_query_slot_bridge_tests.cpp
        src/rendering/effects/native_tu15_lens_query_slots.cpp
        src/rendering/effects/source_lens_flare_tu15_generated.cpp
        src/rendering/effects/source_lens_query_tu15_generated.cpp
        src/rendering/renderer/source_renderer_tail_storage_tu15_generated.cpp)
    target_include_directories(native_tu15_lens_query_slot_bridge_tests PRIVATE include src)
    target_compile_features(native_tu15_lens_query_slot_bridge_tests PRIVATE cxx_std_20)
    target_compile_options(native_tu15_lens_query_slot_bridge_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_tu15_lens_query_slot_bridge_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -O0 -g1)
        target_link_options(native_tu15_lens_query_slot_bridge_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_tu15_lens_query_slot_bridges COMMAND native_tu15_lens_query_slot_bridge_tests)
    add_executable(native_tu15_campaign_lens_request_tests
        tests/campaign_lens_tu15_tests.cpp src/runtime/source_runtime_memory.cpp
        src/rendering/effects/source_lens_flare_tu15_generated.cpp
        src/rendering/effects/native_tu15_lens_query_slots.cpp)
    target_compile_definitions(native_tu15_campaign_lens_request_tests PRIVATE HALO2_LENS_REQUEST_ONLY=1)
    target_include_directories(native_tu15_campaign_lens_request_tests PRIVATE include src)
    target_link_libraries(native_tu15_campaign_lens_request_tests PRIVATE
        halo2_snapshot_storage Threads::Threads nlohmann_json::nlohmann_json)
    target_compile_features(native_tu15_campaign_lens_request_tests PRIVATE cxx_std_20)
    target_compile_options(native_tu15_campaign_lens_request_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_tu15_campaign_lens_request_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_campaign_lens_request_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME original_tu15_native_campaign_lens_request
        COMMAND native_tu15_campaign_lens_request_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_lens_tu15.json")
    set_tests_properties(original_tu15_native_campaign_lens_request PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_dependencies(native_tu15_lens_query_slots_tests native_tu15_lens_query_slot_bridge_tests)
    add_test(NAME original_tu15_native_lens_query_slots
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_lens_query_slots.py
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --native $<TARGET_FILE:native_tu15_lens_query_slots_tests>
        --bridge $<TARGET_FILE:native_tu15_lens_query_slot_bridge_tests>
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-lens-query-slots.json)
    set_tests_properties(original_tu15_native_lens_query_slots PROPERTIES TIMEOUT 90 SKIP_RETURN_CODE 77)
    add_executable(native_tu15_renderer_cache_pools_tests
        tests/native_tu15_renderer_cache_pools_tests.cpp
        src/rendering/renderer/native_tu15_renderer_cache_pools_data.cpp
        src/rendering/renderer/source_renderer_transparent_storage_tu15_generated.cpp
        src/network/native_tu15_network_storage_data.cpp
        src/rendering/renderer/native_tu15_renderer_pool_data.cpp
        src/runtime/source_runtime_memory.cpp
        src/runtime/source_runtime_host_storage.cpp
        src/physics/physics_pool_host_storage.cpp)
    add_executable(native_tu15_renderer_tail_storage_tests
        tests/native_tu15_renderer_tail_storage_tests.cpp
        src/rendering/renderer/source_renderer_cpu_interface_tu15.cpp
        src/rendering/renderer/source_renderer_tail_storage_tu15_generated.cpp
        src/rendering/renderer/native_tu15_renderer_tail_storage_data.cpp
        src/runtime/source_runtime_memory.cpp)
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
    add_test(NAME original_tu15_lens_query_scalar COMMAND native_lens_query_scalar_tests
        "${HALO2_TU15_GAMEPLAY_XBE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_lens_query_scalar_tu15_original.json")
    set_tests_properties(native_tu15_lens_query_scalar_ownership original_tu15_lens_query_scalar
        PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77)
endif()
