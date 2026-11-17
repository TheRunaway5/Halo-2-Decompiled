if(BUILD_TESTING)
    add_executable(native_object_ratio_caller_world_tests tests/native_tu15_object_ratio_caller_world_tests.cpp)
    target_include_directories(native_object_ratio_caller_world_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(native_object_ratio_caller_world_tests PRIVATE halo2_systems)
    add_custom_target(native_object_ratio_caller_tests DEPENDS
        native_object_ratio_caller_world_tests native_object_function_query_public_tests)

    set(_halo2_ratio_accounting)
    set(_halo2_ratio_inherited_accounting --no-accounting)
    if(HALO2_NATIVE_EXECUTION_ACCOUNTING)
        set(_halo2_ratio_accounting --accounting)
        unset(_halo2_ratio_inherited_accounting)
    endif()
    set(_halo2_ratio_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        set(_halo2_ratio_sanitizers --sanitizers)
    endif()

    add_test(NAME original_tu15_native_object_ratio_caller_frame
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_ratio_caller_middle_frame_tu15.py"
        --whole --observers --compiler "${CMAKE_CXX_COMPILER}"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-ratio-caller-frame" ${_halo2_ratio_sanitizers})
    add_test(NAME native_object_ratio_caller_source
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_ratio_caller_source_tu15.py"
        --require-binding --returning-children --compiler "${CMAKE_CXX_COMPILER}"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-object-ratio-caller-source" ${_halo2_ratio_sanitizers})
    add_test(NAME native_object_ratio_caller_world
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_ratio_caller_world_tu15.py"
        --binary "$<TARGET_FILE:native_object_ratio_caller_world_tests>"
        --map "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/coagulation.map"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-object-ratio-caller-world.json" ${_halo2_ratio_accounting})
    add_test(NAME native_object_ratio_caller_inherited
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_object_ratio_caller_inherited_tu15.py"
        --binary "$<TARGET_FILE:native_object_function_query_public_tests>"
        --map "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/coagulation.map"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-object-ratio-caller-inherited.json" ${_halo2_ratio_inherited_accounting})
    set_tests_properties(original_tu15_native_object_ratio_caller_frame native_object_ratio_caller_source
        native_object_ratio_caller_world native_object_ratio_caller_inherited PROPERTIES TIMEOUT 1500 SKIP_RETURN_CODE 77)
    unset(_halo2_ratio_accounting)
    unset(_halo2_ratio_inherited_accounting)
    unset(_halo2_ratio_sanitizers)
endif()
