include_guard(GLOBAL)
target_sources(halo2_cache PRIVATE src/rendering/mcc_native_meter.cpp)

if(BUILD_TESTING)
    add_executable(mcc_native_meter_tests tests/mcc_native_meter_tests.cpp)
    target_link_libraries(mcc_native_meter_tests PRIVATE halo2_cache)
    set(mcc_meter_asset_root "${HALO2_TEST_ASSET_SOURCE_ROOT}")
    if(NOT mcc_meter_asset_root)
        set(mcc_meter_asset_root "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
    add_test(NAME mcc_native_meter COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/mcc_native_meter_tests.py
        --repo-root "${CMAKE_CURRENT_SOURCE_DIR}" --asset-root "${mcc_meter_asset_root}"
        --test-executable $<TARGET_FILE:mcc_native_meter_tests>)
    set_tests_properties(mcc_native_meter PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME mcc_native_meter_generator COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_native_meter.py --check)
    add_test(NAME mcc_native_meter_actual_source COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/export_mcc_native_meter.py
        --cache "${mcc_meter_asset_root}/assets/imported/mcc-sky-water-tu15-20261025-v7"
        --policy "${CMAKE_CURRENT_SOURCE_DIR}/tools/data/mcc-native-meter.json" --check)
    set_tests_properties(mcc_native_meter_actual_source PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
