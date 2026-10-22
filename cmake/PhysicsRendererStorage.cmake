# Complete renderer raw producer and descriptor bodies, with required heap owner.
target_sources(halo2_physics_world_runtime PRIVATE src/physics/physics_renderer_storage_runtime_generated.cpp)
target_sources(halo2_systems PRIVATE src/physics/physics_renderer_storage_host.cpp)
if(BUILD_TESTING)
    add_executable(physics_renderer_storage_tests tests/physics_renderer_storage_tests.cpp)
    target_link_libraries(physics_renderer_storage_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json OpenSSL::Crypto)
    add_test(NAME native_physics_renderer_storage_tu15 COMMAND physics_renderer_storage_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-renderer-storage-tu15-20261012/original-v1/fixture.json
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(native_physics_renderer_storage_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()
