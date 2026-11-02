if(TARGET halo2_systems)
    target_sources(halo2_systems PRIVATE src/rendering/world/native_tu15_cull_walk_44710.cpp
        src/rendering/world/source_native_cull_walk_tu15.cpp)
    if(BUILD_TESTING)
        add_executable(source_native_cull_walk_tu15_tests tests/source_native_cull_walk_tu15_tests.cpp)
        target_link_libraries(source_native_cull_walk_tu15_tests PRIVATE halo2_systems)
        add_test(NAME native_cull_walk_world_tu15 COMMAND source_native_cull_walk_tu15_tests
            "${HALO2_TU15_GAMEPLAY_XBE}" "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/00a_introduction.map")
        set_tests_properties(native_cull_walk_world_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    endif()
endif()
if(MSVC)
    set_source_files_properties(src/rendering/world/native_tu15_cull_walk_44710.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/world/native_tu15_cull_walk_44710.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(BUILD_TESTING AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
    add_executable(native_tu15_cull_walk_tests tests/native_tu15_cull_walk_44710_tests.cpp
        src/rendering/world/native_tu15_cull_walk_44710.cpp)
    target_include_directories(native_tu15_cull_walk_tests PRIVATE include)
    target_compile_features(native_tu15_cull_walk_tests PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_cull_walk_tests PRIVATE nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS)
        target_compile_options(native_tu15_cull_walk_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_cull_walk_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_tu15_cull_walk_controls COMMAND native_tu15_cull_walk_tests)
    if(NOT HALO2_CULL_WALK_ORACLE_SCRIPT)
        set(HALO2_CULL_WALK_ORACLE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_cull_walk.py")
    endif()
    set(_halo2_cull_walk_fixture_output)
    if(HALO2_CULL_WALK_FIXTURE_OUTPUT)
        list(APPEND _halo2_cull_walk_fixture_output --fixture-output "${HALO2_CULL_WALK_FIXTURE_OUTPUT}")
    endif()
    add_test(NAME original_tu15_native_cull_walk COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        "${HALO2_CULL_WALK_ORACLE_SCRIPT}" --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --native $<TARGET_FILE:native_tu15_cull_walk_tests>
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-cull-walk.json" ${_halo2_cull_walk_fixture_output})
    set_tests_properties(original_tu15_native_cull_walk PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
    add_test(NAME native_tu15_cull_walk_fixture COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_tu15_cull_walk_fixture.py"
        --fixture "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_tu15_cull_walk.json.gz"
        --native $<TARGET_FILE:native_tu15_cull_walk_tests>)
    set_tests_properties(native_tu15_cull_walk_fixture PROPERTIES TIMEOUT 90)
endif()
