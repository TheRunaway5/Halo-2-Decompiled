# Complete TU1.5 callback regression, including its real installed shape calls.
if(BUILD_TESTING)
    add_test(NAME original_physics_world_qword_tu15 COMMAND physics_phantom_world_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/world-qword-tu15-20261012/original-verified/u5-native.json)
    set_tests_properties(original_physics_world_qword_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_scalar_width_tests tests/physics_scalar_width_tests.cpp)
    target_include_directories(physics_scalar_width_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(physics_scalar_width_tests PRIVATE halo2_physics_world_runtime)
    add_test(NAME physics_scalar_widths COMMAND physics_scalar_width_tests)
endif()
