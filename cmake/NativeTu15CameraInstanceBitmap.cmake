if(BUILD_TESTING)
    add_custom_target(native_camera_instance_bitmap_tests)
    set(_halo2_camera_instance_bitmap_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_camera_instance_bitmap_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_camera_instance_bitmap
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_camera_instance_bitmap_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-camera-instance-bitmap"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding --mmx ${_halo2_camera_instance_bitmap_sanitizers})
    set_tests_properties(original_tu15_native_camera_instance_bitmap PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_camera_instance_bitmap_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_camera_instance_bitmap_generation_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-camera-instance-bitmap-generation.json"
        --compiler "${CMAKE_CXX_COMPILER}")
    set_tests_properties(original_tu15_native_camera_instance_bitmap_generation PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_camera_instance_bitmap_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_camera_instance_bitmap_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-camera-instance-bitmap-release"
            --compiler "${CMAKE_CXX_COMPILER}" --require-binding --mmx)
        set_tests_properties(original_tu15_native_camera_instance_bitmap_release PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_camera_instance_bitmap_sanitizers)
endif()
