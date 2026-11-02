if(BUILD_TESTING)
    add_executable(campaign_material_wiring_tests tests/campaign_material_wiring_tests.cpp
        src/campaign/source_campaign_model.cpp)
    target_link_libraries(campaign_material_wiring_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    set(HALO2_CAMPAIGN_MATERIAL_WIRING_CACHE
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-zanzibar-tu15-20261101-v9"
        CACHE PATH "Prepared MCC cache for cinematic PRT and texture-camera regression")
    add_test(NAME native_campaign_material_wiring COMMAND campaign_material_wiring_tests
        "${HALO2_CAMPAIGN_MATERIAL_WIRING_CACHE}/halo2/h2_maps_win64_dx11"
        "${CMAKE_CURRENT_BINARY_DIR}/campaign-material-wiring.json")
    set_tests_properties(native_campaign_material_wiring PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    add_executable(mcc_missing_shader_bitmap_tests tests/mcc_missing_shader_bitmap_tests.cpp)
    target_link_libraries(mcc_missing_shader_bitmap_tests PRIVATE halo2_cache)
    add_test(NAME native_mcc_missing_shader_bitmaps COMMAND mcc_missing_shader_bitmap_tests
        "${HALO2_CAMPAIGN_MATERIAL_WIRING_CACHE}/halo2/h2_maps_win64_dx11")
    set_tests_properties(native_mcc_missing_shader_bitmaps PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
    add_executable(native_water_beach_owner_tests tests/native_water_beach_owner_tests.cpp)
    target_link_libraries(native_water_beach_owner_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json OpenSSL::Crypto)
endif()
