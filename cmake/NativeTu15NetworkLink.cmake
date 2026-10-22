target_sources(halo2_systems PRIVATE src/network/native_tu15_network_link.cpp)
target_sources(halo2_systems PRIVATE src/network/native_tu15_network_service.cpp src/network/native_tu15_network_startup.cpp)
target_sources(halo2_systems PRIVATE src/network/native_tu15_online_startup.cpp)
target_sources(halo2_systems PRIVATE src/core/native_tu15_title_address.cpp)
if(APPLE)
    include("${CMAKE_CURRENT_LIST_DIR}/macos/Network.cmake")
    halo2_macos_network_observation(halo2_systems)
endif()
target_sources(halo2_systems PRIVATE src/platform/native_tu15_rng_startup.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/network/source_network_connection_startup_tu15.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/network/source_network_title_address_startup_tu15.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/network/source_network_availability_startup_tu15.cpp)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    pkg_check_modules(HALO2_NATIVE_NETWORK_MANAGER QUIET IMPORTED_TARGET libnm)
    if(HALO2_NATIVE_NETWORK_MANAGER_FOUND)
        set_property(SOURCE src/core/native_tu15_title_address.cpp APPEND PROPERTY COMPILE_DEFINITIONS HALO2_NATIVE_NETWORK_MANAGER=1)
        target_link_libraries(halo2_systems PRIVATE PkgConfig::HALO2_NATIVE_NETWORK_MANAGER)
    endif()
endif()
if(BUILD_TESTING)
    add_executable(native_tu15_network_link_tests tests/native_tu15_network_link_tests.cpp
        src/network/native_tu15_network_link.cpp src/network/native_tu15_network_service.cpp src/network/native_tu15_network_startup.cpp
        src/network/native_tu15_online_startup.cpp
        src/core/native_tu15_title_address.cpp
        src/platform/native_tu15_rng_startup.cpp
        src/network/native_tu15_network_storage_data.cpp
        src/gameplay/multiplayer/native_tu15_game_startup_data.cpp src/physics/native_tu15_arena_data.cpp
        src/physics/native_tu15_arena_bootstrap.cpp src/physics/physics_game_arena_native.cpp
        src/physics/physics_physical_storage_host.cpp src/physics/physics_pool_host_storage.cpp
        src/runtime/source_runtime_memory.cpp src/runtime/source_runtime_host_virtual_memory.cpp
        src/runtime/source_runtime_host_storage.cpp src/runtime/source_runtime_host_physical_memory.cpp
        src/runtime/source_runtime_image.cpp)
    target_link_libraries(native_tu15_network_link_tests PRIVATE halo2_snapshot_storage)
    target_link_libraries(native_tu15_network_link_tests PRIVATE halo2_native_tu15_heap_data
        halo2_physics_world_runtime halo2_physics_platform_locks halo2_native_synchronization
        nlohmann_json::nlohmann_json OpenSSL::Crypto)
    target_link_libraries(native_tu15_network_link_tests PRIVATE halo2_online_directory halo2_online_account halo2_online_matchmaking)
    if(APPLE)
        halo2_macos_network_observation(native_tu15_network_link_tests)
    endif()
    if(HALO2_NATIVE_NETWORK_MANAGER_FOUND)
        target_link_libraries(native_tu15_network_link_tests PRIVATE PkgConfig::HALO2_NATIVE_NETWORK_MANAGER)
    endif()
    add_test(NAME native_tu15_network_link_frontier COMMAND native_tu15_network_link_tests)
    add_test(NAME original_tu15_network_link_frontier COMMAND native_tu15_network_link_tests
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-network-link-v2/report.json)
    add_test(NAME native_tu15_network_connection_frontier COMMAND native_tu15_network_link_tests --frontier xnet-startup)
    add_test(NAME original_tu15_network_connection_frontier COMMAND native_tu15_network_link_tests --frontier xnet-startup
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-network-connection/report.json)
    add_test(NAME native_tu15_network_platform_frontier COMMAND native_tu15_network_link_tests --frontier online-startup)
    add_test(NAME original_tu15_network_platform_frontier COMMAND native_tu15_network_link_tests --frontier online-startup
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-network-platform/report.json)
    set_tests_properties(native_tu15_network_link_frontier original_tu15_network_link_frontier
        native_tu15_network_connection_frontier original_tu15_network_connection_frontier
        native_tu15_network_platform_frontier original_tu15_network_platform_frontier
        PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(native_tu15_network_service_tests tests/native_tu15_network_service_tests.cpp src/network/native_tu15_network_service.cpp)
    target_include_directories(native_tu15_network_service_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(native_tu15_network_service_tests PRIVATE nlohmann_json::nlohmann_json Threads::Threads)
    add_test(NAME native_tu15_network_service COMMAND native_tu15_network_service_tests)
    add_test(NAME original_tu15_network_configuration COMMAND native_tu15_network_service_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-network-configuration.json)
    set_tests_properties(native_tu15_network_service original_tu15_network_configuration PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_test(NAME native_tu15_online_services_frontier COMMAND native_tu15_network_link_tests --frontier title-address)
    add_test(NAME original_tu15_online_services_frontier COMMAND native_tu15_network_link_tests --frontier title-address
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-online-services/report.json)
    set_tests_properties(native_tu15_online_services_frontier original_tu15_online_services_frontier PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_test(NAME native_tu15_network_completed_callback COMMAND native_tu15_network_link_tests --frontier network-complete)
    add_test(NAME original_tu15_network_completed_callback COMMAND native_tu15_network_link_tests --frontier network-complete
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-network-complete-v2/report.json)
    set_tests_properties(native_tu15_network_completed_callback original_tu15_network_completed_callback PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_test(NAME native_tu15_rng_completed_callback COMMAND native_tu15_network_link_tests --frontier rng-complete)
    add_test(NAME original_tu15_rng_completed_callback COMMAND native_tu15_network_link_tests --frontier rng-complete
        --reference-xbe "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        --original-fixture ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261014/native-startup/original-rng-startup-v2/report.json)
    set_tests_properties(native_tu15_rng_completed_callback original_tu15_rng_completed_callback PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()
