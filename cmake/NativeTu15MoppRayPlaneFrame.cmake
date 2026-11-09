if(BUILD_TESTING)
    add_custom_target(native_mopp_ray_plane_frame_tests)
    add_custom_target(native_mopp_ray_branch_frame_tests)
    add_custom_target(native_mopp_ray_crossing_frame_tests)
    add_custom_target(native_mopp_ray_recursion_frame_tests)
    add_custom_target(native_mopp_ray_resume_frame_tests)
    add_custom_target(native_mopp_ray_leaf_frame_tests)
    add_custom_target(native_mopp_ray_diagnostic_frame_tests)
    add_custom_target(native_mopp_ray_split_controller_tests)
    add_custom_target(native_mopp_ray_vector_store_frame_tests)
    set(_halo2_ray_plane_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_ray_plane_sanitizers --sanitizers)
    endif()
    foreach(stage plane branch crossing recursion resume leaf diagnostic vector_store)
        string(REPLACE "_" "-" _halo2_ray_stage_report "${stage}")
        add_test(NAME original_tu15_native_mopp_ray_${stage}_frame
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_mopp_ray_${stage}_frame_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-mopp-ray-${_halo2_ray_stage_report}-frame"
            ${_halo2_ray_plane_sanitizers})
        set_tests_properties(original_tu15_native_mopp_ray_${stage}_frame PROPERTIES
            TIMEOUT 900 SKIP_RETURN_CODE 77)
        if(HALO2_SANITIZERS AND NOT MSVC)
            add_test(NAME original_tu15_native_mopp_ray_${stage}_frame_release
                COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_mopp_ray_${stage}_frame_tu15.py"
                --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
                --output "${CMAKE_CURRENT_BINARY_DIR}/native-mopp-ray-${_halo2_ray_stage_report}-frame-release")
            set_tests_properties(original_tu15_native_mopp_ray_${stage}_frame_release PROPERTIES
                TIMEOUT 900 SKIP_RETURN_CODE 77)
        endif()
    endforeach()
    add_test(NAME original_tu15_native_mopp_ray_split_controller
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_mopp_ray_resume_frame_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-mopp-ray-split-controller"
        --composed ${_halo2_ray_plane_sanitizers})
    set_tests_properties(original_tu15_native_mopp_ray_split_controller PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_mopp_ray_split_controller_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_mopp_ray_resume_frame_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-mopp-ray-split-controller-release"
            --composed)
        set_tests_properties(original_tu15_native_mopp_ray_split_controller_release PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_ray_plane_sanitizers)
    unset(_halo2_ray_stage_report)
endif()
