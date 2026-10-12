target_sources(halo2_systems PRIVATE src/physics_solver_generated.cpp)
if(MSVC)
    set_source_files_properties(src/physics_solver_generated.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/physics_solver_generated.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING)
    add_executable(physics_solver_tests tests/physics_solver_tests.cpp)
    target_link_libraries(physics_solver_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_full_solver COMMAND physics_solver_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-solver-20261011/disc-verified/fixture.json)
    add_test(NAME original_physics_full_solver_tu15 COMMAND physics_solver_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-solver-20261011/u5-verified/fixture.json)
    add_test(NAME original_physics_constraint_completed COMMAND physics_world_step_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-solver-20261011/completed-verified/fixture.json)
    set_tests_properties(original_physics_full_solver original_physics_full_solver_tu15
        original_physics_constraint_completed PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_jacobian_tests tests/physics_jacobian_tests.cpp)
    target_link_libraries(physics_jacobian_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_jacobians COMMAND physics_jacobian_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-constraint-runtime-20261011/disc-full-v1/fixture.json)
    add_test(NAME original_physics_jacobians_tu15 COMMAND physics_jacobian_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-constraint-runtime-20261011/u5-full-v1/fixture.json)
    add_test(NAME original_physics_constraint_produced COMMAND physics_world_step_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-constraint-runtime-20261011/produced-verified/fixture.json)
    set_tests_properties(original_physics_jacobians original_physics_jacobians_tu15
        original_physics_constraint_produced PROPERTIES SKIP_RETURN_CODE 77)
endif()
