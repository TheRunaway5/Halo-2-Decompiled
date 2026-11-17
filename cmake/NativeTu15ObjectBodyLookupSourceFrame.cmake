if(BUILD_TESTING)
    add_custom_target(native_object_body_lookup_source_frame_tests)
    set(_halo2_object_body_source_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_object_body_source_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME native_tu15_object_body_lookup_source_frame
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_body_source_frame_tu15.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-body-lookup-source-frame"
        --compiler "${CMAKE_CXX_COMPILER}" ${_halo2_object_body_source_sanitizers})
    set_tests_properties(native_tu15_object_body_lookup_source_frame PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    add_test(NAME native_tu15_object_body_observer_branch
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_body_observer_branch_tu15.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-body-observer-branch"
        --compiler "${CMAKE_CXX_COMPILER}" ${_halo2_object_body_source_sanitizers})
    set_tests_properties(native_tu15_object_body_observer_branch PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    add_test(NAME native_tu15_object_body_foreign_continuation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_body_foreign_continuation_tu15.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-body-foreign-continuation"
        --compiler "${CMAKE_CXX_COMPILER}" ${_halo2_object_body_source_sanitizers})
    set_tests_properties(native_tu15_object_body_foreign_continuation PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME native_tu15_object_body_lookup_source_frame_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_body_source_frame_tu15.py"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-body-lookup-source-frame-release"
            --compiler "${CMAKE_CXX_COMPILER}")
        set_tests_properties(native_tu15_object_body_lookup_source_frame_release PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
        add_test(NAME native_tu15_object_body_observer_branch_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_body_observer_branch_tu15.py"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-body-observer-branch-release"
            --compiler "${CMAKE_CXX_COMPILER}")
        set_tests_properties(native_tu15_object_body_observer_branch_release PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
        add_test(NAME native_tu15_object_body_foreign_continuation_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_body_foreign_continuation_tu15.py"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-body-foreign-continuation-release"
            --compiler "${CMAKE_CXX_COMPILER}")
        set_tests_properties(native_tu15_object_body_foreign_continuation_release PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_object_body_source_sanitizers)
endif()
