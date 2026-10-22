# This probe includes the existing x86 SSE/x87 execution adapter. Universal
# macOS numeric admission remains separate from this native-x86 component test.
if(BUILD_TESTING AND NOT APPLE AND
   CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86_64|[aA][mM][dD]64|[xX]64|i[3-6]86)$")
    add_executable(source_runtime_vector_read_tests
        tests/source_runtime_vector_read_tests.cpp src/runtime/source_runtime_memory.cpp)
    target_include_directories(source_runtime_vector_read_tests PRIVATE include src)
    target_link_libraries(source_runtime_vector_read_tests PRIVATE halo2_snapshot_storage Threads::Threads)
    add_test(NAME source_runtime_vector_read COMMAND source_runtime_vector_read_tests)
    add_test(NAME source_runtime_vector_read_reference COMMAND source_runtime_vector_read_tests)
    set_tests_properties(source_runtime_vector_read_reference PROPERTIES
        ENVIRONMENT "HALO2_REFERENCE_SCALAR_VECTOR_READS=1")
endif()
if(BUILD_TESTING)
    add_test(NAME framerate_benchmark_dimensions COMMAND "${Python3_EXECUTABLE}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_framerate_benchmark_dimensions.py")
    add_test(NAME performance_goal_capture COMMAND "${Python3_EXECUTABLE}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_performance_goal_capture.py")
endif()
