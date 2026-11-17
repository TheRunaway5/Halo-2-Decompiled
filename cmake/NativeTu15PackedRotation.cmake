if(BUILD_TESTING)
    add_custom_target(native_packed_rotation_tests)
    set(_halo2_packed_rotation_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_packed_rotation_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_packed_rotation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_packed_rotation_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-packed-rotation"
        --compiler "${CMAKE_CXX_COMPILER}" --prefixes --mmx
        ${_halo2_packed_rotation_sanitizers})
    add_test(NAME original_tu15_native_packed_rotation_policy
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_packed_rotation_policy_tu15.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-packed-rotation-policy"
        --compiler "${CMAKE_CXX_COMPILER}" --mmx ${_halo2_packed_rotation_sanitizers})
    set(_halo2_packed_rotation_simple_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_packed_rotation_simple_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_packed_rotation_value
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_packed_rotation_value_tu15.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-packed-rotation-value"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding
        ${_halo2_packed_rotation_simple_sanitizers})
    add_test(NAME original_tu15_native_packed_rotation_routes
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_packed_rotation_routes_tu15.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-packed-rotation-routes"
        --compiler "${CMAKE_CXX_COMPILER}" --mmx --require-binding
        ${_halo2_packed_rotation_simple_sanitizers})
    add_test(NAME original_tu15_native_packed_rotation_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_packed_rotation_generation_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-packed-rotation-generation.json"
        --compiler "${CMAKE_CXX_COMPILER}" --candidate-preparation --require-binding)
    set_tests_properties(original_tu15_native_packed_rotation
        original_tu15_native_packed_rotation_policy original_tu15_native_packed_rotation_value
        original_tu15_native_packed_rotation_routes original_tu15_native_packed_rotation_generation
        PROPERTIES TIMEOUT 3600 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_packed_rotation_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_packed_rotation_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-packed-rotation-release"
            --compiler "${CMAKE_CXX_COMPILER}" --prefixes --mmx)
        set_tests_properties(original_tu15_native_packed_rotation_release
            PROPERTIES TIMEOUT 3600 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_packed_rotation_sanitizers)
    unset(_halo2_packed_rotation_simple_sanitizers)
endif()
