# Native TU1.5 point/segment distance leaf (14DD70, scene device): seeded
# comparison against the pinned translated body (SSE + x87 FSQRT/FABS/FSTP,
# C1), leaf verify mode, sanitizer replay and mutation screen.
if(BUILD_TESTING)
    add_custom_target(native_point_segment_distance_tests)
    add_test(NAME native_tu15_point_segment_distance
        COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/run_test.py" --
        "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_point_segment_distance_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-point-segment-distance.json" --jobs 2)
    set_tests_properties(native_tu15_point_segment_distance PROPERTIES TIMEOUT 1800)
endif()
