if(BUILD_TESTING)
    add_custom_target(native_instanced_ray_child_tests)
    add_test(NAME native_instanced_ray_child_bindings
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_instanced_ray_child_bindings.py")
    set_tests_properties(native_instanced_ray_child_bindings PROPERTIES TIMEOUT 60)
    set(_halo2_instanced_child_sanitizers)
    set(_halo2_instanced_child_value_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_instanced_child_sanitizers --sanitizers)
        list(APPEND _halo2_instanced_child_value_sanitizers --sanitizers --leak-sanitizer)
    endif()
    # Both actual complete source owners, the raw bridge, and their reference
    # routes are compared to unchanged i386 instructions. The public value
    # facade retains its separate captured-input and observer policy.
    add_test(NAME original_tu15_native_instanced_ray_child
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_instanced_ray_child_binding.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-instanced-ray-child"
        --require-binding --mutations ${_halo2_instanced_child_sanitizers})
    set_tests_properties(original_tu15_native_instanced_ray_child PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    add_test(NAME native_instanced_ray_child_public_values
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_instanced_ray_child_values.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-instanced-ray-child-values.json"
        --require-binding ${_halo2_instanced_child_value_sanitizers})
    set_tests_properties(native_instanced_ray_child_public_values PROPERTIES TIMEOUT 300)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_instanced_ray_child_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_instanced_ray_child_binding.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-instanced-ray-child-release"
            --require-binding --mutations)
        set_tests_properties(original_tu15_native_instanced_ray_child_release PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_instanced_child_sanitizers)
    unset(_halo2_instanced_child_value_sanitizers)
endif()
