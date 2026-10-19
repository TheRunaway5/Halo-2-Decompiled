target_sources(halo2_systems PRIVATE src/native_tu15_contrail_draw_data.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_contrail_draw_data_tests tests/native_tu15_contrail_draw_data_tests.cpp
        src/native_tu15_contrail_draw_data.cpp src/source_runtime_memory.cpp)
    target_include_directories(native_tu15_contrail_draw_data_tests PRIVATE include)
    target_link_libraries(native_tu15_contrail_draw_data_tests PRIVATE halo2_snapshot_storage
        Threads::Threads nlohmann_json::nlohmann_json OpenSSL::Crypto)
    add_test(NAME native_tu15_contrail_draw_data COMMAND native_tu15_contrail_draw_data_tests)
    add_test(NAME original_tu15_contrail_draw_data COMMAND native_tu15_contrail_draw_data_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/contrail_state_tu15_original.json")
    add_test(NAME original_tu15_contrail_geometry_dispatch_data COMMAND native_tu15_contrail_draw_data_tests
        --geometry "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/contrail_geometry_dispatch_tu15_original.json")
    set_tests_properties(native_tu15_contrail_draw_data original_tu15_contrail_draw_data
        original_tu15_contrail_geometry_dispatch_data
        PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
endif()
