if(BUILD_TESTING)
    add_custom_target(native_host_fp_io_tests)
    set(_halo2_host_fp_io_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_host_fp_io_sanitizers --sanitizers --negative-controls)
    endif()
    add_test(NAME native_tu15_host_fp_io
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_host_fp_io_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-host-fp-io.json"
        ${_halo2_host_fp_io_sanitizers})
    set_tests_properties(native_tu15_host_fp_io PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME native_tu15_host_fp_io_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_host_fp_io_tu15.py"
            --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-host-fp-io-release.json")
        set_tests_properties(native_tu15_host_fp_io_release PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_host_fp_io_sanitizers)
endif()
