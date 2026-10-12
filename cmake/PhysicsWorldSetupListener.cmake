# Original-input comparison of complete source world setup listener and registration.
if(BUILD_TESTING)
    add_test(NAME native_physics_world_setup_listener COMMAND physics_platform_call_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-world-setup-listener-runtime-20261011/original-verified/fixture.json
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-bsp-construction-runtime-20261011/call-bindings-verified/report.json)
    set_tests_properties(native_physics_world_setup_listener PROPERTIES TIMEOUT 60)
endif()
