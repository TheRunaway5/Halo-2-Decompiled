if(BUILD_TESTING)
    add_custom_target(native_animation_resource_wait_tests)
    set(_halo2_animation_resource_wait_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_animation_resource_wait_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_animation_resource_wait
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_animation_resource_wait.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-animation-resource-wait.json"
        --require-binding ${_halo2_animation_resource_wait_sanitizers})
    set_tests_properties(original_tu15_native_animation_resource_wait PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_animation_resource_wait_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_animation_resource_wait.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-animation-resource-wait-release.json"
            --require-binding)
        set_tests_properties(original_tu15_native_animation_resource_wait_release PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_animation_resource_wait_sanitizers)
endif()
