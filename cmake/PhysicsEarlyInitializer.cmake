# Complete original early initializer, with real pool/main storage joins.
target_sources(halo2_physics_world_runtime PRIVATE src/physics_early_initializer_runtime_generated.cpp)
target_sources(halo2_systems PRIVATE src/physics_early_initializer_host.cpp)
if(BUILD_TESTING)
    add_executable(physics_early_initializer_tests tests/physics_early_initializer_tests.cpp)
    target_link_libraries(physics_early_initializer_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json OpenSSL::Crypto)
    add_test(NAME native_physics_early_initializer_tu15 COMMAND physics_early_initializer_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-early-initializer-tu15-20261012/original-v1/report.json
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(native_physics_early_initializer_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()
