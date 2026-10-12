# Actual original-input continuation through source world setup and shape bounds.
if(BUILD_TESTING)
    add_test(NAME native_physics_world_manager COMMAND physics_platform_call_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-world-manager-runtime-20261011/original-verified/fixture.json
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-bsp-construction-runtime-20261011/call-bindings-verified/report.json)
    set_tests_properties(native_physics_world_manager PROPERTIES TIMEOUT 60)
endif()
