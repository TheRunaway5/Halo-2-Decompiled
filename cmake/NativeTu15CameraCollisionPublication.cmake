if(BUILD_TESTING)
    add_custom_target(native_camera_collision_publication_tests)
    set(_halo2_camera_collision_publication_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_camera_collision_publication_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_camera_collision_publication
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_camera_collision_publication_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-camera-collision-publication"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding --mmx ${_halo2_camera_collision_publication_sanitizers})
    set_tests_properties(original_tu15_native_camera_collision_publication PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_camera_collision_publication_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_camera_collision_publication_generation_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-camera-collision-publication-generation.json"
        --compiler "${CMAKE_CXX_COMPILER}")
    set_tests_properties(original_tu15_native_camera_collision_publication_generation PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_camera_collision_publication_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_camera_collision_publication_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-camera-collision-publication-release"
            --compiler "${CMAKE_CXX_COMPILER}" --require-binding --mmx)
        set_tests_properties(original_tu15_native_camera_collision_publication_release PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_camera_collision_publication_sanitizers)
endif()
