if(BUILD_TESTING)
    add_custom_target(native_resource_composition_tests)
    set(_halo2_resource_composition_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_resource_composition_sanitizers --sanitizers --leak-sanitizer)
    endif()
    add_test(NAME original_tu15_native_resource_composition
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_resource_composition.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-resource-composition.json"
        --physical ${_halo2_resource_composition_sanitizers})
    set_tests_properties(original_tu15_native_resource_composition PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_resource_composition_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_resource_composition.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-resource-composition-release.json"
            --physical)
        set_tests_properties(original_tu15_native_resource_composition_release PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_resource_composition_sanitizers)
endif()
