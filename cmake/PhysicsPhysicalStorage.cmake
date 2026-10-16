# Complete physical-memory stage and partition paths for fresh TU1.5 startup.
target_sources(halo2_physics_world_runtime PRIVATE src/physics_physical_storage_runtime_generated.cpp)
target_sources(halo2_systems PRIVATE src/physics_physical_storage_host.cpp)
if(BUILD_TESTING)
    add_executable(source_physical_allocation_requests_tests tests/source_physical_allocation_requests_tests.cpp)
    target_link_libraries(source_physical_allocation_requests_tests PRIVATE halo2_systems)
    add_test(NAME native_source_physical_allocation_requests COMMAND source_physical_allocation_requests_tests)
    set_tests_properties(native_source_physical_allocation_requests PROPERTIES RUN_SERIAL TRUE TIMEOUT 30)
    add_executable(physics_physical_storage_tests tests/physics_physical_storage_tests.cpp)
    target_link_libraries(physics_physical_storage_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_physics_physical_storage_tu15 COMMAND physics_physical_storage_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-physical-storage-tu15-20261012/original-v1/fixture.json
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(native_physics_physical_storage_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()
