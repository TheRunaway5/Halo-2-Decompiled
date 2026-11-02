if(BUILD_TESTING)
    add_executable(native_tu15_damage_region_eligibility_tests
        tests/native_tu15_damage_region_eligibility_tests.cpp)
    target_include_directories(native_tu15_damage_region_eligibility_tests PRIVATE include src)
    target_compile_features(native_tu15_damage_region_eligibility_tests PRIVATE cxx_std_20)
    target_link_libraries(native_tu15_damage_region_eligibility_tests PRIVATE halo2_glm nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_damage_region_eligibility_reference COMMAND "${Python3_EXECUTABLE}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_damage_region_eligibility_tu15.py
        --probe $<TARGET_FILE:native_tu15_damage_region_eligibility_tests>
        --reference ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_tu15_damage_region_eligibility.json.gz
        --counts ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_region_descriptor_counts.bin.gz
        --manifest ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_tu15_damage_region_eligibility.json
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-damage-region-eligibility-report.json)
    set_tests_properties(native_tu15_damage_region_eligibility_reference PROPERTIES TIMEOUT 150)
endif()
