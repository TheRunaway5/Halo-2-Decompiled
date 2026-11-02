if(BUILD_TESTING)
  if(TARGET halo2_systems)
    add_executable(campaign_render_script_flags_tu15_tests tests/campaign_render_script_flags_tu15_tests.cpp)
    target_link_libraries(campaign_render_script_flags_tu15_tests PRIVATE halo2_systems)
    add_test(NAME native_campaign_render_script_flags_tu15 COMMAND campaign_render_script_flags_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/00a_introduction.map")
    set_tests_properties(native_campaign_render_script_flags_tu15 PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 120)
  endif()
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
        add_executable(native_tu15_render_script_flags_tests tests/native_tu15_render_script_flags_tests.cpp
            src/rendering/native_tu15_render_script_flags.cpp)
        target_include_directories(native_tu15_render_script_flags_tests PRIVATE include)
        target_compile_features(native_tu15_render_script_flags_tests PRIVATE cxx_std_20)
        target_link_libraries(native_tu15_render_script_flags_tests PRIVATE nlohmann_json::nlohmann_json)
        if(HALO2_SANITIZERS)
            target_compile_options(native_tu15_render_script_flags_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(native_tu15_render_script_flags_tests PRIVATE -fsanitize=address,undefined)
        endif()
        if(NOT HALO2_RENDER_SCRIPT_FLAGS_ORACLE_SCRIPT)
            set(HALO2_RENDER_SCRIPT_FLAGS_ORACLE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_render_script_flags.py")
        endif()
        set(_halo2_render_flags_fixture_output)
        if(HALO2_RENDER_SCRIPT_FLAGS_FIXTURE_OUTPUT)
            list(APPEND _halo2_render_flags_fixture_output --fixture-output "${HALO2_RENDER_SCRIPT_FLAGS_FIXTURE_OUTPUT}")
        endif()
        add_test(NAME original_tu15_native_render_script_flags COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            "${HALO2_RENDER_SCRIPT_FLAGS_ORACLE_SCRIPT}"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --native $<TARGET_FILE:native_tu15_render_script_flags_tests>
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-render-script-flags.json" ${_halo2_render_flags_fixture_output})
        set_tests_properties(original_tu15_native_render_script_flags PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
        add_test(NAME native_tu15_render_script_flags_fixture COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_tu15_render_script_flags_fixture.py"
            --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_tu15_render_script_flags.json.gz"
            --native $<TARGET_FILE:native_tu15_render_script_flags_tests>)
        set_tests_properties(native_tu15_render_script_flags_fixture PROPERTIES TIMEOUT 60)
    endif()
endif()
