if(BUILD_TESTING)
    add_custom_target(native_bsp_queue_find_tests)
    add_test(NAME native_bsp_queue_find_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_bsp_queue_find_replacements.py")
    set_tests_properties(native_bsp_queue_find_binding PROPERTIES TIMEOUT 120)
    set(_halo2_queue_find_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_queue_find_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_bsp_queue_find
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_bsp_queue_find_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-bsp-queue-find.json"
        --prefixes --require-binding ${_halo2_queue_find_sanitizers})
    set_tests_properties(original_tu15_native_bsp_queue_find PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    set(_halo2_queue_find_values_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_queue_find_values_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_bsp_queue_find_values
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_bsp_queue_find_values.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-bsp-queue-find-values.json"
        --require-binding ${_halo2_queue_find_values_sanitizers})
    set_tests_properties(original_tu15_native_bsp_queue_find_values PROPERTIES TIMEOUT 180)
    add_test(NAME original_tu15_native_bsp_queue_find_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_bsp_queue_find_generation_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-bsp-queue-find-generation.json"
        --require-binding)
    set_tests_properties(original_tu15_native_bsp_queue_find_generation PROPERTIES TIMEOUT 300)
    add_test(NAME original_tu15_native_bsp_queue_find_release
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_bsp_queue_find_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-bsp-queue-find-release.json"
        --prefixes --require-binding)
    set_tests_properties(original_tu15_native_bsp_queue_find_release PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
endif()
