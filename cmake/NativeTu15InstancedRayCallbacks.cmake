if(BUILD_TESTING)
    add_custom_target(native_instanced_ray_callbacks_tests)
    set(_halo2_callback_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_callback_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_instanced_ray_callbacks
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_mopp_ray_leaf_query_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-instanced-ray-callbacks"
        --source-routes --require-binding ${_halo2_callback_sanitizers})
    set_tests_properties(original_tu15_native_instanced_ray_callbacks PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_instanced_ray_callbacks_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_instanced_ray_callback_generation_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-instanced-ray-callbacks-generation.json")
    set_tests_properties(original_tu15_native_instanced_ray_callbacks_generation PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_instanced_ray_callbacks_release
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_mopp_ray_leaf_query_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-instanced-ray-callbacks-release"
        --source-routes --require-binding)
    set_tests_properties(original_tu15_native_instanced_ray_callbacks_release PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    unset(_halo2_callback_sanitizers)
endif()
