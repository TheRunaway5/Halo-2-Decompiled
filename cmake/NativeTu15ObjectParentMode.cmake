if(BUILD_TESTING)
    add_custom_target(native_object_parent_mode_tests)
    set(_halo2_object_parent_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_object_parent_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_object_parent_mode
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_parent_mode_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-object-parent-mode"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding --source-copies
        --prefixes --mmx ${_halo2_object_parent_sanitizers})
    set_tests_properties(original_tu15_native_object_parent_mode PROPERTIES TIMEOUT 1800)
    add_test(NAME original_tu15_native_object_parent_mode_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_parent_mode_generation_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-object-parent-mode-generation.json"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding)
    set_tests_properties(original_tu15_native_object_parent_mode_generation PROPERTIES TIMEOUT 1800)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_object_parent_mode_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_parent_mode_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-object-parent-mode-release"
            --compiler "${CMAKE_CXX_COMPILER}" --require-binding --source-copies --prefixes --mmx)
        set_tests_properties(original_tu15_native_object_parent_mode_release PROPERTIES TIMEOUT 1800)
    endif()
    unset(_halo2_object_parent_sanitizers)
endif()
