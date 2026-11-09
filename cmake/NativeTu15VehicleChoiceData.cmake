if(BUILD_TESTING)
    add_custom_target(native_vehicle_choice_data_tests)
    set(_halo2_vehicle_choice_data_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_vehicle_choice_data_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_vehicle_choice_data
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_vehicle_choice.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-vehicle-choice-data.json"
        --require-binding --prefixes ${_halo2_vehicle_choice_data_sanitizers})
    set_tests_properties(original_tu15_native_vehicle_choice_data PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_vehicle_choice_data_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_vehicle_choice.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-vehicle-choice-data-release.json"
            --require-binding --prefixes)
        set_tests_properties(original_tu15_native_vehicle_choice_data_release PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_vehicle_choice_data_sanitizers)
endif()
