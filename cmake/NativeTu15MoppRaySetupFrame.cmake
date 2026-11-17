if(BUILD_TESTING)
    add_custom_target(native_mopp_ray_setup_frame_tests)
    set(_halo2_mopp_setup_frame_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_mopp_setup_frame_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_mopp_ray_setup_frame
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_mopp_ray_setup_frame_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-mopp-ray-setup-frame"
        ${_halo2_mopp_setup_frame_sanitizers})
    set_tests_properties(original_tu15_native_mopp_ray_setup_frame PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_mopp_ray_setup_frame_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_mopp_ray_setup_frame_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-mopp-ray-setup-frame-release")
        set_tests_properties(original_tu15_native_mopp_ray_setup_frame_release PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_mopp_setup_frame_sanitizers)
endif()
