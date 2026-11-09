if(BUILD_TESTING)
    add_custom_target(native_mopp_ray_slab_frame_tests)
    set(_halo2_ray_slab_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_ray_slab_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_mopp_ray_slab_frame
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_mopp_ray_slab_frame_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-mopp-ray-slab-frame"
        --negative-controls ${_halo2_ray_slab_sanitizers})
    set_tests_properties(original_tu15_native_mopp_ray_slab_frame PROPERTIES
        TIMEOUT 900 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_mopp_ray_slab_frame_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_mopp_ray_slab_frame_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-mopp-ray-slab-frame-release")
        set_tests_properties(original_tu15_native_mopp_ray_slab_frame_release PROPERTIES
            TIMEOUT 900 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_ray_slab_sanitizers)
endif()
