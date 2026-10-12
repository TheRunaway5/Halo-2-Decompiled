# Reuse the native platform comparison for the independently captured BSP/body
# construction continuation. The earlier capture remains a retained prefix test.
if(BUILD_TESTING)
    add_test(NAME native_physics_bsp_construction COMMAND physics_platform_call_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-bsp-construction-runtime-20261011/original-verified/fixture.json
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-bsp-construction-runtime-20261011/call-bindings-verified/report.json)
    set_tests_properties(native_physics_bsp_construction PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
endif()
