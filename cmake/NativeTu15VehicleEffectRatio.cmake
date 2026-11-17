if(BUILD_TESTING)
    add_custom_target(native_vehicle_effect_ratio_tests)
    set(_halo2_effect_ratio_raw_checks)
    set(_halo2_effect_ratio_source_checks)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_effect_ratio_raw_checks --sanitizers --mutations)
        list(APPEND _halo2_effect_ratio_source_checks --sanitizers --mutations
            --mutant use-modeled-shift-in-native-source --mutant ignore-reference-control --mutant ignore-title-image)
    endif()
    function(halo2_add_vehicle_effect_ratio_test name driver report)
        add_test(NAME "${name}"
            COMMAND "${CMAKE_COMMAND}" -E env --unset=HALO2_REFERENCE_VEHICLE_EFFECT_RATIO
            "${HALO2_RENDERER_ORACLE_PYTHON}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py"
            --scratch-root "${CMAKE_CURRENT_BINARY_DIR}/native-vehicle-effect-ratio-scratch" --
            "${HALO2_RENDERER_ORACLE_PYTHON}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/${driver}"
            --compiler "${CMAKE_CXX_COMPILER}" --report "${CMAKE_CURRENT_BINARY_DIR}/${report}" ${ARGN})
        set_tests_properties("${name}" PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    endfunction()
    halo2_add_vehicle_effect_ratio_test(original_tu15_native_vehicle_effect_ratio_raw
        validate_native_vehicle_effect_ratio_tu15.py native-vehicle-effect-ratio-raw.json
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --prefixes ${_halo2_effect_ratio_raw_checks})
    halo2_add_vehicle_effect_ratio_test(original_tu15_native_vehicle_effect_ratio_source
        validate_native_vehicle_effect_ratio_source_tu15.py native-vehicle-effect-ratio-source.json
        --binding-candidate --require-installed ${_halo2_effect_ratio_source_checks})
    unset(_halo2_effect_ratio_raw_checks)
    unset(_halo2_effect_ratio_source_checks)
endif()
