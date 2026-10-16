target_sources(halo2_cache PRIVATE src/mcc_sound_effect_definitions.cpp)
if(BUILD_TESTING)
    add_executable(mcc_sound_effect_definitions_tests tests/mcc_sound_effect_definitions_tests.cpp)
    target_link_libraries(mcc_sound_effect_definitions_tests PRIVATE halo2_cache)
    add_test(NAME mcc_sound_effect_definitions
        COMMAND mcc_sound_effect_definitions_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/imported/mcc-zanzibar-restored-20261013
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/mcc-dsp-resource-inventory/asset-inventory.json
        ${CMAKE_CURRENT_BINARY_DIR}/mcc-sound-effect-definitions-report.json)
    set_tests_properties(mcc_sound_effect_definitions PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
endif()
