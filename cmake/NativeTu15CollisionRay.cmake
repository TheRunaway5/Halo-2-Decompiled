# Native TU1.5 collision ray 16C620 binding (see
# docs/native_replacement/16C620_collision_ray.md). The binding itself is
# installed by the operand specializer (tools/native_collision_ray_replacements.py).
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    set(HALO2_COLLISION_RAY_TEST_SOURCES tests/native_tu15_collision_ray_tests.cpp
        src/physics/native_tu15_camera_geometric_query.cpp src/physics/native_tu15_camera_query_leaves.cpp
        src/physics/native_tu15_camera_dynamic_query.cpp src/physics/native_tu15_camera_query_object_context.cpp
        src/physics/native_tu15_camera_dynamic_query_leaves.cpp src/physics/native_tu15_camera_collision_ray.cpp
        src/physics/native_tu15_camera_collision_regions.cpp src/physics/native_tu15_bsp_segment_query.cpp
        src/physics/native_tu15_bsp_surface_query.cpp src/physics/native_tu15_constraint_effect_location.cpp
        src/physics/native_tu15_camera_instanced_query.cpp src/physics/native_tu15_instanced_ray_callbacks.cpp
        src/physics/native_tu15_instanced_scene_ray.cpp src/physics/native_tu15_instanced_ray_lifetime.cpp
        src/physics/native_tu15_mopp_ray_setup.cpp src/physics/native_tu15_mopp_ray_traversal.cpp
        src/physics/native_tu15_island_activation.cpp src/rendering/world/native_tu15_camera_projection.cpp)
    add_executable(native_tu15_collision_ray_tests ${HALO2_COLLISION_RAY_TEST_SOURCES})
    target_include_directories(native_tu15_collision_ray_tests PRIVATE include src)
    target_compile_features(native_tu15_collision_ray_tests PRIVATE cxx_std_20)
    target_compile_options(native_tu15_collision_ray_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
    target_link_libraries(native_tu15_collision_ray_tests PRIVATE nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS)
        target_compile_options(native_tu15_collision_ray_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_collision_ray_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_tu15_collision_ray_journal COMMAND native_tu15_collision_ray_tests --self-test)
    add_test(NAME original_tu15_collision_ray_call_site_liveness COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/analyze_call_site_liveness_tu15.py" --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --target 16c620 --wrappers --require-clean)
    set_tests_properties(original_tu15_collision_ray_call_site_liveness PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 1800)
endif()
