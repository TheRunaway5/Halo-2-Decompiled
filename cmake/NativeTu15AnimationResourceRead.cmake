if(BUILD_TESTING)
    add_custom_target(native_animation_resource_read_tests)
    set(_halo2_animation_read_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_animation_read_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_animation_resource_read
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_animation_resource_read.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-animation-resource-read.json"
        --require-binding ${_halo2_animation_read_sanitizers})
    set_tests_properties(original_tu15_native_animation_resource_read PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    unset(_halo2_animation_read_sanitizers)
endif()
