if(BUILD_TESTING)
    add_test(NAME native_renderer_session_refresh COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_renderer_native_kernel_session.py")
endif()
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_tu15_shadow_view_tests tests/native_tu15_shadow_view_320a0_tests.cpp
        src/rendering/shadows/native_tu15_shadow_view_320a0.cpp)
    target_include_directories(native_tu15_shadow_view_tests PRIVATE include)
    target_compile_features(native_tu15_shadow_view_tests PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_shadow_view_tests PRIVATE nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS)
        target_compile_options(native_tu15_shadow_view_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_shadow_view_tests PRIVATE -fsanitize=address,undefined)
    endif()
    if(NOT HALO2_SHADOW_VIEW_ORACLE_SCRIPT)
        set(HALO2_SHADOW_VIEW_ORACLE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_shadow_view.py")
    endif()
    set(_halo2_shadow_view_fixture_output)
    if(HALO2_SHADOW_VIEW_FIXTURE_OUTPUT)
        list(APPEND _halo2_shadow_view_fixture_output --fixture-output "${HALO2_SHADOW_VIEW_FIXTURE_OUTPUT}")
    endif()
    add_test(NAME original_tu15_native_shadow_view COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        "${HALO2_SHADOW_VIEW_ORACLE_SCRIPT}" --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --native $<TARGET_FILE:native_tu15_shadow_view_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-shadow-view.json" ${_halo2_shadow_view_fixture_output})
    set_tests_properties(original_tu15_native_shadow_view PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
    add_test(NAME native_tu15_shadow_view_fixture COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_tu15_shadow_view_fixture.py"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_tu15_shadow_view.json.gz"
        --native $<TARGET_FILE:native_tu15_shadow_view_tests>)
    set_tests_properties(native_tu15_shadow_view_fixture PROPERTIES TIMEOUT 90)
endif()
