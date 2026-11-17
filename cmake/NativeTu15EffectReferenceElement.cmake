if(BUILD_TESTING)
    add_custom_target(native_effect_reference_element_tests)
    set(_halo2_effect_reference_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_effect_reference_sanitizers --sanitizers --semantic-mutants --source-mutants)
    endif()
    foreach(_halo2_effect_reference_mode IN ITEMS native release)
        set(_halo2_effect_reference_flags)
        set(_halo2_effect_reference_suffix)
        if(_halo2_effect_reference_mode STREQUAL native)
            set(_halo2_effect_reference_flags ${_halo2_effect_reference_sanitizers})
        else()
            set(_halo2_effect_reference_suffix _release)
        endif()
        set(_halo2_effect_reference_folder native-effect-reference-element)
        if(_halo2_effect_reference_suffix)
            string(APPEND _halo2_effect_reference_folder -release)
        endif()
        add_test(NAME original_tu15_native_effect_reference_element${_halo2_effect_reference_suffix}
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_effect_reference_element_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/${_halo2_effect_reference_folder}"
            --prefixes --source-copies --require-binding ${_halo2_effect_reference_flags})
        set_tests_properties(original_tu15_native_effect_reference_element${_halo2_effect_reference_suffix}
            PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    endforeach()
    add_test(NAME original_tu15_native_effect_reference_element_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_effect_reference_element_generation_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-effect-reference-element-generation.json")
    set_tests_properties(original_tu15_native_effect_reference_element_generation PROPERTIES TIMEOUT 600)
endif()
