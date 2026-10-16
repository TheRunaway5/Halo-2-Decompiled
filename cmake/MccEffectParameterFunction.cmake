# Selected type7 source math with explicit inputs, speaker geometry and FP.
# Actual MCC ownership is retained; no clock or install receipt is supplied.
add_library(halo2_mcc_effect_parameter_function STATIC
    src/mcc_effect_parameter_function.cpp)
target_include_directories(halo2_mcc_effect_parameter_function PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_mcc_effect_parameter_function PUBLIC cxx_std_20)
target_compile_options(halo2_mcc_effect_parameter_function PRIVATE
    -frounding-math -fno-fast-math -ffp-contract=off)
target_link_libraries(halo2_mcc_effect_parameter_function PUBLIC halo2_cache)
target_link_libraries(halo2_systems PUBLIC halo2_mcc_effect_parameter_function)
if(BUILD_TESTING)
    add_executable(mcc_effect_parameter_function_probe
        tests/mcc_effect_parameter_function_probe.cpp)
    target_link_libraries(mcc_effect_parameter_function_probe PRIVATE
        halo2_mcc_effect_parameter_function)
    add_executable(mcc_effect_parameter_function_definition_probe
        tests/mcc_effect_parameter_function_definition_probe.cpp)
    target_link_libraries(mcc_effect_parameter_function_definition_probe PRIVATE
        halo2_mcc_effect_parameter_function nlohmann_json::nlohmann_json)
    add_executable(mcc_effect_parameter_function_state_tests
        tests/mcc_effect_parameter_function_state_tests.cpp)
    target_link_libraries(mcc_effect_parameter_function_state_tests PRIVATE
        halo2_mcc_effect_parameter_function)
    add_test(NAME mcc_effect_parameter_function_original_values
        COMMAND "${Python3_EXECUTABLE}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_mcc_effect_parameter_function.py
        --probe $<TARGET_FILE:mcc_effect_parameter_function_probe>
        --vectors ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/mcc_effect_parameter_function_values.json.gz
        --report ${CMAKE_CURRENT_BINARY_DIR}/mcc-effect-parameter-function-values-report.json)
    add_test(NAME mcc_effect_parameter_function_retained_definitions
        COMMAND "${Python3_EXECUTABLE}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_mcc_effect_parameter_function.py
        --definition-probe $<TARGET_FILE:mcc_effect_parameter_function_definition_probe>
        --maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/imported/mcc-zanzibar-restored-20261013/halo2/h2_maps_win64_dx11
        --vectors ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/mcc_effect_parameter_function_values.json.gz
        --report ${CMAKE_CURRENT_BINARY_DIR}/mcc-effect-parameter-function-definitions-report.json)
    add_test(NAME mcc_effect_parameter_function_complete_FP_state
        COMMAND mcc_effect_parameter_function_state_tests)
    set_tests_properties(mcc_effect_parameter_function_original_values
        mcc_effect_parameter_function_retained_definitions
        mcc_effect_parameter_function_complete_FP_state PROPERTIES TIMEOUT 180)
endif()
