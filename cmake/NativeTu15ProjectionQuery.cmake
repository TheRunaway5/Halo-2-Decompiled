# Native 44E80/2E79A0/2E7C40 projection queries
# (src/rendering/shadows/native_tu15_projection_query.hpp, installed in the
# Shadow-frame unit by tools/native_projection_query_replacements.py).
if(BUILD_TESTING)
    add_test(NAME native_projection_query_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_projection_query_replacements.py")
    set_tests_properties(native_projection_query_binding PROPERTIES TIMEOUT 120)
    set(_halo2_projection_query_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_projection_query_sanitizers --sanitizers)
    endif()
    # Unchanged TU1.5 i386 machine code (physical original) versus the native
    # bridges and the translated bodies on generated programs, including
    # genuine growth; 77 skips without the XBE or an i386 toolchain.
    add_test(NAME original_tu15_native_projection_query
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_projection_query_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-projection-query.json"
        --seeds 101-104 --cases 2000 ${_halo2_projection_query_sanitizers})
    set_tests_properties(original_tu15_native_projection_query PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
endif()
