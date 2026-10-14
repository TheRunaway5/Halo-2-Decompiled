target_sources(halo2_physics_world_runtime PRIVATE src/source_network_storage_startup_tu15.cpp)
target_sources(halo2_systems PRIVATE src/native_tu15_network_storage_data.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_network_storage_tests tests/native_tu15_network_storage_tests.cpp
        src/native_tu15_network_storage_data.cpp src/native_tu15_game_startup_data.cpp
        src/native_tu15_arena_data.cpp src/native_tu15_arena_bootstrap.cpp
        src/physics_game_arena_native.cpp src/physics_physical_storage_host.cpp
        src/physics_pool_host_storage.cpp src/source_runtime_memory.cpp
        src/source_runtime_host_virtual_memory.cpp src/source_runtime_host_storage.cpp
        src/source_runtime_host_physical_memory.cpp src/source_runtime_image.cpp)
    target_link_libraries(native_tu15_network_storage_tests PRIVATE halo2_snapshot_storage)
    target_link_libraries(native_tu15_network_storage_tests PRIVATE halo2_native_tu15_heap_data
        halo2_physics_world_runtime halo2_physics_platform_locks halo2_native_synchronization
        nlohmann_json::nlohmann_json OpenSSL::Crypto)
    add_test(NAME native_tu15_network_storage_frontier COMMAND native_tu15_network_storage_tests)
    add_test(NAME original_tu15_network_storage_frontier COMMAND native_tu15_network_storage_tests
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-network-storage-v2/report.json)
    set_tests_properties(native_tu15_network_storage_frontier original_tu15_network_storage_frontier
        PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()
