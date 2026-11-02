if(TARGET halo2_systems)
    target_sources(halo2_systems PRIVATE src/rendering/world/native_tu15_render_prepare.cpp
        src/rendering/world/source_native_render_prepare_tu15.cpp)
    if(BUILD_TESTING)
        add_executable(source_native_render_prepare_tests tests/source_native_render_prepare_tests.cpp)
        target_link_libraries(source_native_render_prepare_tests PRIVATE halo2_systems)
        add_test(NAME native_render_prepare_world_tu15 COMMAND source_native_render_prepare_tests
            "${HALO2_TU15_GAMEPLAY_XBE}" "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/00a_introduction.map")
        set_tests_properties(native_render_prepare_world_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    endif()
endif()
if(BUILD_TESTING)
    add_executable(native_tu15_render_prepare_tests tests/native_tu15_render_prepare_tests.cpp
        src/rendering/world/native_tu15_render_prepare.cpp src/rendering/source_geometry_resource_request.cpp
        src/rendering/source_geometry_resource.cpp)
    target_include_directories(native_tu15_render_prepare_tests PRIVATE include)
    target_compile_features(native_tu15_render_prepare_tests PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_render_prepare_tests PRIVATE nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS)
        target_compile_options(native_tu15_render_prepare_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_render_prepare_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_tu15_render_prepare_controls COMMAND native_tu15_render_prepare_tests)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
        if(NOT HALO2_RENDER_PREPARE_ORACLE_SCRIPT)
            set(HALO2_RENDER_PREPARE_ORACLE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_render_prepare.py")
        endif()
        add_test(NAME original_tu15_native_render_prepare COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            "${HALO2_RENDER_PREPARE_ORACLE_SCRIPT}" --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --native $<TARGET_FILE:native_tu15_render_prepare_tests>
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-render-prepare.json")
        set_tests_properties(original_tu15_native_render_prepare PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
    endif()
endif()
