# Source-proven bounded value proposals require an explicit FP mode.
# Definitions retain their actual MCC Cache; these supply no install receipt.
add_library(halo2_native_tu15_sound_mono_parameters STATIC
    src/native_tu15_sound_mono_parameters.cpp)
target_include_directories(halo2_native_tu15_sound_mono_parameters PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_native_tu15_sound_mono_parameters PUBLIC cxx_std_20)
target_compile_options(halo2_native_tu15_sound_mono_parameters PRIVATE
    -frounding-math -fno-fast-math -ffp-contract=off)
add_library(halo2_mcc_mono_distortion_definition STATIC
    src/mcc_mono_distortion_definition.cpp)
target_include_directories(halo2_mcc_mono_distortion_definition PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_mcc_mono_distortion_definition PUBLIC cxx_std_20)
target_link_libraries(halo2_mcc_mono_distortion_definition PUBLIC
    halo2_native_tu15_sound_mono_parameters halo2_cache
    halo2_mcc_effect_parameter_function)
target_link_libraries(halo2_systems PUBLIC halo2_mcc_mono_distortion_definition)
if(BUILD_TESTING)
    add_executable(native_tu15_sound_mono_parameters_probe
        tests/native_tu15_sound_mono_parameters_probe.cpp)
    target_link_libraries(native_tu15_sound_mono_parameters_probe PRIVATE
        halo2_native_tu15_sound_mono_parameters)
    add_executable(mcc_mono_distortion_definition_probe
        tests/mcc_mono_distortion_definition_probe.cpp)
    target_link_libraries(mcc_mono_distortion_definition_probe PRIVATE
        halo2_mcc_mono_distortion_definition nlohmann_json::nlohmann_json)
    add_executable(native_tu15_sound_mono_parameters_fp_tests
        tests/native_tu15_sound_mono_parameters_fp_tests.cpp)
    target_link_libraries(native_tu15_sound_mono_parameters_fp_tests PRIVATE
        halo2_native_tu15_sound_mono_parameters)
    add_test(NAME native_tu15_sound_mono_parameter_reference_values
        COMMAND "${Python3_EXECUTABLE}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_sound_mono_parameters.py
        --math-probe $<TARGET_FILE:native_tu15_sound_mono_parameters_probe>
        --vectors ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_sound_mono_parameter_values.json.gz
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-sound-mono-parameter-values-report.json)
    add_test(NAME mcc_mono_distortion_retained_definitions
        COMMAND "${Python3_EXECUTABLE}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_sound_mono_parameters.py
        --definition-probe $<TARGET_FILE:mcc_mono_distortion_definition_probe>
        --maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/imported/mcc-zanzibar-restored-20261013/halo2/h2_maps_win64_dx11
        --vectors ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/native_tu15_sound_mono_parameter_values.json.gz
        --report ${CMAKE_CURRENT_BINARY_DIR}/mcc-mono-distortion-definitions-report.json)
    add_test(NAME native_tu15_sound_mono_parameter_FP_contract
        COMMAND native_tu15_sound_mono_parameters_fp_tests)
    add_executable(native_tu15_sound_mono_parameters_condition_tests
        tests/native_tu15_sound_mono_parameters_condition_tests.cpp)
    target_link_libraries(native_tu15_sound_mono_parameters_condition_tests PRIVATE
        halo2_native_tu15_sound_mono_parameters)
    add_executable(native_tu15_sound_mono_parameters_state_tests
        tests/native_tu15_sound_mono_parameters_state_tests.cpp)
    target_link_libraries(native_tu15_sound_mono_parameters_state_tests PRIVATE
        halo2_native_tu15_sound_mono_parameters)
    add_test(NAME native_tu15_sound_mono_parameter_condition_preservation
        COMMAND native_tu15_sound_mono_parameters_condition_tests)
    add_test(NAME native_tu15_sound_mono_parameter_complete_FP_state
        COMMAND native_tu15_sound_mono_parameters_state_tests)
    set_tests_properties(native_tu15_sound_mono_parameter_condition_preservation
        native_tu15_sound_mono_parameter_complete_FP_state PROPERTIES TIMEOUT 180)
    set_tests_properties(native_tu15_sound_mono_parameter_reference_values
        mcc_mono_distortion_retained_definitions native_tu15_sound_mono_parameter_FP_contract
        PROPERTIES TIMEOUT 180)
endif()
