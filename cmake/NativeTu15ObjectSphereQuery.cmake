# Native TU1.5 object sphere query closure (BD080/BA970/BAA40): seeded
# comparison against the pinned translated bodies of all three BD080 variants,
# census and verify modes, sanitizer replay and mutation screen.
if(BUILD_TESTING)
    add_custom_target(native_object_sphere_query_tests)
    add_test(NAME native_tu15_object_sphere_query
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_sphere_query_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-object-sphere-query.json" --jobs 2)
    set_tests_properties(native_tu15_object_sphere_query PROPERTIES TIMEOUT 1800)
endif()
