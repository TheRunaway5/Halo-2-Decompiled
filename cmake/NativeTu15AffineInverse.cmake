if(BUILD_TESTING)
    add_custom_target(native_affine_inverse_tests)
    set(_halo2_inverse_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_inverse_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_affine_inverse
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_affine_inverse_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-affine-inverse"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding --source-copies --values --mmx --semantic-mutants ${_halo2_inverse_sanitizers})
    add_test(NAME original_tu15_native_affine_inverse_mappack
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_affine_inverse_tu15.py"
        --xbe "${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03"
        --code-image MapPack11
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-affine-inverse-mappack"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding --source-copies --mmx ${_halo2_inverse_sanitizers})
    add_test(NAME original_tu15_native_affine_inverse_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_affine_inverse_generation_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-affine-inverse-generation.json"
        --compiler "${CMAKE_CXX_COMPILER}")
    set_tests_properties(original_tu15_native_affine_inverse original_tu15_native_affine_inverse_mappack original_tu15_native_affine_inverse_generation PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_affine_inverse_motion_c1
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_motion_fst_c1_tu15.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-affine-inverse-motion-c1"
        --compiler "${CMAKE_CXX_COMPILER}" ${_halo2_inverse_sanitizers})
    set_tests_properties(original_tu15_native_affine_inverse_motion_c1 PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_affine_inverse_animation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_affine_inverse_animation_tu15.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-affine-inverse-animation"
        --compiler "${CMAKE_CXX_COMPILER}" --require-binding ${_halo2_inverse_sanitizers})
    set_tests_properties(original_tu15_native_affine_inverse_animation PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_affine_inverse_animation_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_affine_inverse_animation_tu15.py"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-affine-inverse-animation-release"
            --compiler "${CMAKE_CXX_COMPILER}" --require-binding)
        set_tests_properties(original_tu15_native_affine_inverse_animation_release PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
        foreach(_halo2_inverse_image IN ITEMS tu15 mappack)
            set(_halo2_inverse_image_args --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --values)
            set(_halo2_inverse_suffix "")
            if(_halo2_inverse_image STREQUAL "mappack")
                set(_halo2_inverse_image_args --xbe "${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03" --code-image MapPack11)
                set(_halo2_inverse_suffix "_mappack")
            endif()
            string(REPLACE "_" "-" _halo2_inverse_output_suffix "${_halo2_inverse_suffix}")
            add_test(NAME original_tu15_native_affine_inverse${_halo2_inverse_suffix}_release
                COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_affine_inverse_tu15.py"
                ${_halo2_inverse_image_args}
                --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-affine-inverse${_halo2_inverse_output_suffix}-release"
                --compiler "${CMAKE_CXX_COMPILER}" --require-binding --source-copies --mmx)
            set_tests_properties(original_tu15_native_affine_inverse${_halo2_inverse_suffix}_release PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
        endforeach()
    endif()
    unset(_halo2_inverse_sanitizers)
    unset(_halo2_inverse_image)
    unset(_halo2_inverse_image_args)
    unset(_halo2_inverse_suffix)
    unset(_halo2_inverse_output_suffix)
endif()
