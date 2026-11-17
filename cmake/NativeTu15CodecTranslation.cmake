if(BUILD_TESTING)
    add_custom_target(native_codec_translation_tests)
    set(_halo2_codec_translation_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_codec_translation_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_codec_translation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_codec_translation_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-codec-translation"
        --compiler "${CMAKE_CXX_COMPILER}" --prefixes --mmx --require-binding
        ${_halo2_codec_translation_sanitizers})
    add_test(NAME original_tu15_native_codec_translation_source
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_codec_translation_source_tu15.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-codec-translation-source"
        --compiler "${CMAKE_CXX_COMPILER}" --prefixes --mmx --smoke --whole-syntax --require-binding
        ${_halo2_codec_translation_sanitizers})
    set_tests_properties(original_tu15_native_codec_translation
        original_tu15_native_codec_translation_source
        PROPERTIES TIMEOUT 3600 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_codec_translation_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_codec_translation_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-codec-translation-release"
            --compiler "${CMAKE_CXX_COMPILER}" --prefixes --mmx --require-binding)
        set_tests_properties(original_tu15_native_codec_translation_release
            PROPERTIES TIMEOUT 3600 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_codec_translation_sanitizers)
endif()
