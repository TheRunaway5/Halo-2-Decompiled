if(BUILD_TESTING)
    add_custom_target(native_arena_world_initialize_tests)
    add_test(NAME native_arena_world_initialize_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_arena_world_initialize_replacements.py")
    set_tests_properties(native_arena_world_initialize_binding PROPERTIES TIMEOUT 180)
    set(_halo2_arena_world_sanitizers)
    set(_halo2_arena_world_interface_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_arena_world_sanitizers --sanitizers --semantic-mutants)
        list(APPEND _halo2_arena_world_interface_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_arena_world_initialize_source
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_arena_world_source_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-arena-world-initialize-source.json"
        ${_halo2_arena_world_sanitizers})
    add_test(NAME original_tu15_native_arena_world_initialize_source_release
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_arena_world_source_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-arena-world-initialize-source-release.json")
    add_test(NAME original_tu15_native_arena_world_initialize_publication
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_arena_world_publication_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-arena-world-initialize-publication.json"
        ${_halo2_arena_world_interface_sanitizers})
    set_tests_properties(original_tu15_native_arena_world_initialize_source
        original_tu15_native_arena_world_initialize_source_release
        original_tu15_native_arena_world_initialize_publication
        PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    unset(_halo2_arena_world_sanitizers)
    unset(_halo2_arena_world_interface_sanitizers)
endif()
