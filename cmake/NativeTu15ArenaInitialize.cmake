if(BUILD_TESTING)
    add_custom_target(native_arena_initialize_tests)
    add_test(NAME native_arena_initialize_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_arena_initialize_replacements.py")
    set_tests_properties(native_arena_initialize_binding PROPERTIES TIMEOUT 120)
    set(_halo2_arena_initialize_sanitizers)
    set(_halo2_arena_initialize_io_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_arena_initialize_sanitizers --sanitizers --semantic-mutants)
        list(APPEND _halo2_arena_initialize_io_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_arena_initialize
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_arena_initialize_frame_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --all-phases --prefixes --report "${CMAKE_CURRENT_BINARY_DIR}/native-arena-initialize.json"
        ${_halo2_arena_initialize_sanitizers})
    add_test(NAME original_tu15_native_arena_initialize_release
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_arena_initialize_frame_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --all-phases --prefixes --report "${CMAKE_CURRENT_BINARY_DIR}/native-arena-initialize-release.json")
    add_test(NAME original_tu15_native_arena_initialize_source
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_arena_owner_policy_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --source-routes --require-binding
        --memory-revocation-control --uncontrolled-reference-controls
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-arena-initialize-source.json"
        ${_halo2_arena_initialize_sanitizers})
    add_test(NAME original_tu15_native_arena_initialize_source_release
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_arena_owner_policy_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --source-routes --require-binding
        --memory-revocation-control --uncontrolled-reference-controls
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-arena-initialize-source-release.json")
    add_test(NAME original_tu15_native_arena_initialize_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_arena_initialize_generation_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-arena-initialize-generation.json")
    add_test(NAME original_tu15_native_arena_initialize_schedule
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_arena_schedule_policy_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-arena-initialize-schedule.json"
        ${_halo2_arena_initialize_io_sanitizers})
    add_test(NAME original_tu15_native_arena_initialize_transport
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_arena_initialize_source_transport_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-arena-initialize-transport.json"
        ${_halo2_arena_initialize_sanitizers})
    add_test(NAME original_tu15_native_arena_initialize_bootstrap
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_arena_initialize_bootstrap_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-arena-initialize-bootstrap.json"
        ${_halo2_arena_initialize_io_sanitizers})
    set_tests_properties(original_tu15_native_arena_initialize
        original_tu15_native_arena_initialize_release
        original_tu15_native_arena_initialize_source
        original_tu15_native_arena_initialize_source_release
        original_tu15_native_arena_initialize_generation
        original_tu15_native_arena_initialize_schedule
        original_tu15_native_arena_initialize_transport
        original_tu15_native_arena_initialize_bootstrap
        PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    unset(_halo2_arena_initialize_sanitizers)
    unset(_halo2_arena_initialize_io_sanitizers)
endif()
