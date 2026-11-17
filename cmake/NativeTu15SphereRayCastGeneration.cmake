if(BUILD_TESTING)
    add_custom_target(native_sphere_ray_cast_generation_tests)
    add_test(NAME original_tu15_native_sphere_ray_cast_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_sphere_ray_cast_generation_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-sphere-ray-cast-generation.json"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding)
    set_tests_properties(original_tu15_native_sphere_ray_cast_generation PROPERTIES TIMEOUT 600)
endif()
