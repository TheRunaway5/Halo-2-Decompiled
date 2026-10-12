# Complete retained original-input return, including game callback and iterator.
if(BUILD_TESTING)
    add_test(NAME native_physics_world_initialization COMMAND physics_platform_call_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-world-game-callback-runtime-20261011/original-verified/fixture.json
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-bsp-construction-runtime-20261011/call-bindings-verified/report.json)
    set_tests_properties(native_physics_world_initialization PROPERTIES TIMEOUT 60)
endif()
