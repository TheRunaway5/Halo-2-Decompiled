if(BUILD_TESTING)
    add_custom_target(native_lazy_heap_data_tests)
    set(_halo2_lazy_heap_data_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_lazy_heap_data_sanitizers --sanitizers)
    endif()
    add_test(NAME native_tu15_lazy_heap_controller_data
        COMMAND "${Python3_EXECUTABLE}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_lazy_heap_data.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-lazy-heap-data.json"
        ${_halo2_lazy_heap_data_sanitizers})
    set_tests_properties(native_tu15_lazy_heap_controller_data PROPERTIES TIMEOUT 180)
    unset(_halo2_lazy_heap_data_sanitizers)
endif()
