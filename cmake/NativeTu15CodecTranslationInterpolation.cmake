if(BUILD_TESTING)
    option(HALO2_NATIVE_CODEC_TRANSLATION_INTERPOLATION_REQUIRE_BINDING "Require installed codec2962F0 defaults" ON)
    set(_halo2_codec_translation_interpolation_binding)
    if(HALO2_NATIVE_CODEC_TRANSLATION_INTERPOLATION_REQUIRE_BINDING)
        list(APPEND _halo2_codec_translation_interpolation_binding --require-binding)
    endif()
    add_custom_target(native_codec_translation_interpolation_tests)
    set(_halo2_codec_translation_interpolation_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_codec_translation_interpolation_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_codec_translation_interpolation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_codec_translation_interpolation_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-codec-translation-interpolation"
        --compiler "${CMAKE_CXX_COMPILER}" --prefixes --mmx ${_halo2_codec_translation_interpolation_binding}
        ${_halo2_codec_translation_interpolation_sanitizers})
    add_test(NAME original_tu15_native_codec_translation_interpolation_source
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_codec_translation_interpolation_source_tu15.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-codec-translation-interpolation-source"
        --compiler "${CMAKE_CXX_COMPILER}" --prefixes --mmx --smoke --whole-syntax ${_halo2_codec_translation_interpolation_binding}
        ${_halo2_codec_translation_interpolation_sanitizers})
    set_tests_properties(original_tu15_native_codec_translation_interpolation
        original_tu15_native_codec_translation_interpolation_source PROPERTIES TIMEOUT 3600 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_codec_translation_interpolation_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_codec_translation_interpolation_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-codec-translation-interpolation-release"
            --compiler "${CMAKE_CXX_COMPILER}" --prefixes --mmx ${_halo2_codec_translation_interpolation_binding})
        set_tests_properties(original_tu15_native_codec_translation_interpolation_release
            PROPERTIES TIMEOUT 3600 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_codec_translation_interpolation_sanitizers)
    unset(_halo2_codec_translation_interpolation_binding)
endif()
