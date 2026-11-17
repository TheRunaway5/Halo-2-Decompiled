if(BUILD_TESTING)
    add_custom_target(native_solver_hull_context_value_tests)
    set(_halo2_solver_hull_reference_root "${CMAKE_CURRENT_SOURCE_DIR}")
    if(HALO2_TEST_ASSET_SOURCE_ROOT)
        set(_halo2_solver_hull_reference_root "${HALO2_TEST_ASSET_SOURCE_ROOT}")
    endif()
    set(_halo2_solver_hull_archive "${_halo2_solver_hull_reference_root}/analysis/multiplayer/physics-live-tu15-20261012/mcc-native-reorientation-joint-original-v1479-20261102/iteration-01-readonly-process-2e5320-two-frag625-joint-vehicle-camera/required-native-contact-process-2e5320-before/actual-native-before.tar.zst")
    foreach(_halo2_solver_hull_mode IN ITEMS declared genuine)
        add_test(NAME original_tu15_native_solver_hull_context_value_${_halo2_solver_hull_mode}
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_solver_hull_context_value_tu15.py"
            --mode "${_halo2_solver_hull_mode}"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --archive "${_halo2_solver_hull_archive}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-solver-hull-context-value-${_halo2_solver_hull_mode}.json")
        set_tests_properties(original_tu15_native_solver_hull_context_value_${_halo2_solver_hull_mode}
            PROPERTIES TIMEOUT 300 SKIP_RETURN_CODE 77)
    endforeach()
    unset(_halo2_solver_hull_reference_root)
    unset(_halo2_solver_hull_archive)
    unset(_halo2_solver_hull_mode)
endif()
