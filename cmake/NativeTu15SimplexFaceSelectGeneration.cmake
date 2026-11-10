if(BUILD_TESTING)
    add_custom_target(native_simplex_face_select_generation_tests)
    add_test(NAME original_tu15_native_simplex_face_select_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_simplex_face_select_generation_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-simplex-face-select-generation.json")
    set_tests_properties(original_tu15_native_simplex_face_select_generation PROPERTIES TIMEOUT 360)
endif()
