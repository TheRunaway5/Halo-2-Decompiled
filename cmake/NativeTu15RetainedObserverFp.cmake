if(BUILD_TESTING)
    add_custom_target(native_retained_observer_fp_tests)
    set(_halo2_retained_observer_fp_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_retained_observer_fp_sanitizers --sanitizers)
    endif()
    add_test(NAME native_tu15_retained_observer_fp
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_retained_observer_fp_tu15.py
        --compiler "${CMAKE_CXX_COMPILER}"
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-retained-observer-fp.json
        --negative-control --regression ${_halo2_retained_observer_fp_sanitizers})
    set_tests_properties(native_tu15_retained_observer_fp PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME native_tu15_retained_observer_fp_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_retained_observer_fp_tu15.py
            --compiler "${CMAKE_CXX_COMPILER}"
            --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-retained-observer-fp-release.json)
        set_tests_properties(native_tu15_retained_observer_fp_release PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_retained_observer_fp_sanitizers)
endif()
