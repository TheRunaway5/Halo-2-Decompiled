# Function values remain proposals, with actual MCC definitions retained.
# Original parameter bounds precede the existing native Q23 conversion.
if(BUILD_TESTING)
    add_executable(mcc_effect_parameter_function_raw_probe
        tests/mcc_effect_parameter_function_raw_probe.cpp)
    target_link_libraries(mcc_effect_parameter_function_raw_probe PRIVATE
        halo2_mcc_effect_parameter_function)
    add_executable(mcc_mono_function_bridge_tests
        tests/mcc_mono_function_bridge_tests.cpp)
    target_link_libraries(mcc_mono_function_bridge_tests PRIVATE
        halo2_mcc_mono_distortion_definition nlohmann_json::nlohmann_json)
    add_executable(mcc_mono_function_boundary_tests
        tests/mcc_mono_function_boundary_tests.cpp)
    target_link_libraries(mcc_mono_function_boundary_tests PRIVATE
        halo2_mcc_mono_distortion_definition nlohmann_json::nlohmann_json)
    foreach(kind IN ITEMS raw bridge boundary)
        if(kind STREQUAL "raw")
            set(probe mcc_effect_parameter_function_raw_probe)
        else()
            set(probe mcc_mono_function_${kind}_tests)
        endif()
        add_test(NAME mcc_mono_function_${kind}_reference
            COMMAND "${Python3_EXECUTABLE}"
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_mcc_mono_function_bridge.py
            --kind ${kind} --probe $<TARGET_FILE:${probe}>
            --maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/imported/mcc-zanzibar-restored-20261013/halo2/h2_maps_win64_dx11
            --functions ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/mcc_effect_parameter_function_values.json.gz
            --builder ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/mcc_mono_function_builder_values.json.gz
            --boundary ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/mcc_mono_function_boundary_values.json.gz
            --report ${CMAKE_CURRENT_BINARY_DIR}/mcc-mono-function-${kind}-report.json)
        set_tests_properties(mcc_mono_function_${kind}_reference PROPERTIES TIMEOUT 180)
    endforeach()
endif()
