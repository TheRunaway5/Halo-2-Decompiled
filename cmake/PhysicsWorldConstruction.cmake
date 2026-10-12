# Compare the retained original world setup and the independently sampled MMX
# index repair. Source startup and live match ownership remain explicit work.
if(BUILD_TESTING)
    add_test(NAME native_physics_world_construction COMMAND physics_platform_call_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-world-construction-runtime-20261011/original-verified/fixture.json
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-bsp-construction-runtime-20261011/call-bindings-verified/report.json)
    set_tests_properties(native_physics_world_construction PROPERTIES TIMEOUT 30)
    add_executable(physics_world_mmx_tests tests/physics_world_mmx_tests.cpp)
    target_link_libraries(physics_world_mmx_tests PRIVATE halo2_physics_world_runtime nlohmann_json::nlohmann_json)
    add_test(NAME original_physics_world_mmx COMMAND physics_world_mmx_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-world-construction-runtime-20261011/mmx-original-verified/fixture.json)
    set_tests_properties(original_physics_world_mmx PROPERTIES TIMEOUT 30)
endif()
