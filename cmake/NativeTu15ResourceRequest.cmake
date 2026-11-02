if(TARGET halo2_systems AND BUILD_TESTING)
    add_executable(source_native_resource_request_tests tests/source_native_resource_request_tests.cpp)
    target_link_libraries(source_native_resource_request_tests PRIVATE halo2_systems)
    add_test(NAME native_resource_request_world_tu15 COMMAND source_native_resource_request_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/00a_introduction.map")
    set_tests_properties(native_resource_request_world_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
if(BUILD_TESTING)
    add_executable(native_tu15_resource_request_tests tests/native_tu15_resource_request_tests.cpp
        src/rendering/source_geometry_resource_request.cpp src/rendering/source_geometry_resource.cpp)
    target_include_directories(native_tu15_resource_request_tests PRIVATE include)
    target_compile_features(native_tu15_resource_request_tests PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_resource_request_tests PRIVATE nlohmann_json::nlohmann_json)
    if(HALO2_SANITIZERS)
        target_compile_options(native_tu15_resource_request_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_resource_request_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_tu15_resource_request_controls COMMAND native_tu15_resource_request_tests)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i.86")
        if(NOT HALO2_RESOURCE_REQUEST_ORACLE_SCRIPT)
            set(HALO2_RESOURCE_REQUEST_ORACLE_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_resource_request.py")
        endif()
        add_test(NAME original_tu15_native_resource_request COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
            "${HALO2_RESOURCE_REQUEST_ORACLE_SCRIPT}" --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
            --native $<TARGET_FILE:native_tu15_resource_request_tests>
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-resource-request.json")
        set_tests_properties(original_tu15_native_resource_request PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
    endif()
endif()
