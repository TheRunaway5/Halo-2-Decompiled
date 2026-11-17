if(BUILD_TESTING)
    option(HALO2_NATIVE_CODEC_COPY_ROWS_REQUIRE_BINDING "Require installed codec row-copy owner groups" ON)
    set(_halo2_codec_copy_binding)
    if(HALO2_NATIVE_CODEC_COPY_ROWS_REQUIRE_BINDING)
        list(APPEND _halo2_codec_copy_binding --require-binding)
    endif()
    set(_halo2_codec_copy_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_codec_copy_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_custom_target(native_codec_copy_rows_tests)
    foreach(_halo2_codec_copy_kind IN ITEMS row_copy scale_row)
        string(REPLACE "_" "-" _halo2_codec_copy_report_kind "${_halo2_codec_copy_kind}")
        set(_halo2_codec_copy_test "original_tu15_native_codec_${_halo2_codec_copy_kind}")
        set(_halo2_codec_copy_args
            "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_codec_${_halo2_codec_copy_kind}_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --prefixes --mmx ${_halo2_codec_copy_binding})
        add_test(NAME "${_halo2_codec_copy_test}"
            COMMAND ${_halo2_codec_copy_args}
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-codec-${_halo2_codec_copy_report_kind}"
            ${_halo2_codec_copy_sanitizers})
        set_tests_properties("${_halo2_codec_copy_test}" PROPERTIES TIMEOUT 3600 SKIP_RETURN_CODE 77)
        if(HALO2_SANITIZERS AND NOT MSVC)
            add_test(NAME "${_halo2_codec_copy_test}_release"
                COMMAND ${_halo2_codec_copy_args}
                --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-codec-${_halo2_codec_copy_report_kind}-release")
            set_tests_properties("${_halo2_codec_copy_test}_release" PROPERTIES TIMEOUT 3600 SKIP_RETURN_CODE 77)
        endif()
    endforeach()
    add_test(NAME original_tu15_native_codec_copy_rows_source
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_codec_copy_source_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-codec-copy-rows-source"
        --prefixes --mmx --smoke --whole-syntax ${_halo2_codec_copy_binding} ${_halo2_codec_copy_sanitizers})
    set_tests_properties(original_tu15_native_codec_copy_rows_source PROPERTIES TIMEOUT 5400 SKIP_RETURN_CODE 77)
    unset(_halo2_codec_copy_report_kind)
    unset(_halo2_codec_copy_test)
    unset(_halo2_codec_copy_args)
    unset(_halo2_codec_copy_sanitizers)
    unset(_halo2_codec_copy_binding)
endif()
