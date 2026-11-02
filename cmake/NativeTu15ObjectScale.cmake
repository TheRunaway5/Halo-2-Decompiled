if(BUILD_TESTING AND TARGET halo2_systems)
    add_executable(campaign_object_scale_tu15_tests tests/campaign_object_scale_tu15_tests.cpp)
    target_link_libraries(campaign_object_scale_tu15_tests PRIVATE halo2_systems)
    add_test(NAME native_campaign_object_scale_tu15 COMMAND campaign_object_scale_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/00a_introduction.map")
    set_tests_properties(native_campaign_object_scale_tu15 PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 180)
endif()
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_tu15_object_scale_tests tests/native_tu15_object_scale_tests.cpp
        src/rendering/native_tu15_object_scale.cpp)
    target_include_directories(native_tu15_object_scale_tests PRIVATE include)
    target_compile_features(native_tu15_object_scale_tests PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_object_scale_tests PRIVATE nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS)
        target_compile_options(native_tu15_object_scale_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_object_scale_tests PRIVATE -fsanitize=address,undefined)
    endif()
    if(NOT HALO2_OBJECT_SCALE_ORACLE_SCRIPT)
        set(HALO2_OBJECT_SCALE_ORACLE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_object_scale.py")
    endif()
    set(_halo2_object_scale_fixture_output)
    if(HALO2_OBJECT_SCALE_FIXTURE_OUTPUT)
        list(APPEND _halo2_object_scale_fixture_output --fixture-output "${HALO2_OBJECT_SCALE_FIXTURE_OUTPUT}")
    endif()
    add_test(NAME original_tu15_native_object_scale COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        "${HALO2_OBJECT_SCALE_ORACLE_SCRIPT}" --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --native $<TARGET_FILE:native_tu15_object_scale_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-object-scale.json" ${_halo2_object_scale_fixture_output})
    set_tests_properties(original_tu15_native_object_scale PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
    add_test(NAME native_tu15_object_scale_fixture COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_tu15_object_scale_fixture.py"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_tu15_object_scale.json.gz"
        --native $<TARGET_FILE:native_tu15_object_scale_tests>)
    set_tests_properties(native_tu15_object_scale_fixture PROPERTIES TIMEOUT 90)
endif()
