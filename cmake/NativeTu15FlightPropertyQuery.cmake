if(BUILD_TESTING)
    option(HALO2_NATIVE_FLIGHT_PROPERTY_QUERY_REQUIRE_BINDING
        "Require the native Primary E0900 flight property query" ON)
    add_custom_target(native_flight_property_query_tests)
    set(_halo2_flight_query_sanitizers)
    set(_halo2_flight_query_binding)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_flight_query_sanitizers --sanitizers --semantic-mutants)
    endif()
    if(HALO2_NATIVE_FLIGHT_PROPERTY_QUERY_REQUIRE_BINDING)
        list(APPEND _halo2_flight_query_binding --require-binding)
    endif()
    add_test(NAME original_tu15_native_flight_property_query_frame
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_flight_property_query_frame_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-flight-property-query-frame"
        --prefixes --observers ${_halo2_flight_query_sanitizers})
    add_test(NAME original_tu15_native_flight_property_query_source
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_flight_property_query_source_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-flight-property-query-source"
        ${_halo2_flight_query_sanitizers} ${_halo2_flight_query_binding})
    add_test(NAME native_flight_property_query_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_flight_property_query_binding_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-flight-property-query-binding" ${_halo2_flight_query_binding})
    add_test(NAME original_tu15_native_death_cycle_flight_query_source
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_flight_property_query_source_tu15.py"
        --death-cycle --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-death-cycle-flight-query-source"
        ${_halo2_flight_query_sanitizers} ${_halo2_flight_query_binding})
    add_test(NAME native_death_cycle_flight_query_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_flight_property_query_binding_tu15.py"
        --death-cycle --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-death-cycle-flight-query-binding" ${_halo2_flight_query_binding})
    set_tests_properties(original_tu15_native_flight_property_query_frame
        original_tu15_native_flight_property_query_source native_flight_property_query_binding
        original_tu15_native_death_cycle_flight_query_source native_death_cycle_flight_query_binding
        PROPERTIES TIMEOUT 2400 SKIP_RETURN_CODE 77)
    if(HALO2_NATIVE_FLIGHT_PROPERTY_QUERY_REQUIRE_BINDING AND CMAKE_SYSTEM_NAME STREQUAL "Linux"
        AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|i.86)$")
        add_executable(native_flight_property_query_public_tests tests/native_tu15_flight_property_query_public_tests.cpp)
        target_include_directories(native_flight_property_query_public_tests PRIVATE include src)
        target_compile_features(native_flight_property_query_public_tests PRIVATE cxx_std_20)
        target_link_libraries(native_flight_property_query_public_tests PRIVATE halo2_physics_world_runtime nlohmann_json::nlohmann_json)
        add_dependencies(native_flight_property_query_tests native_flight_property_query_public_tests)
        add_test(NAME native_flight_property_query_public
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_flight_property_query_public_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --native "$<TARGET_FILE:native_flight_property_query_public_tests>"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-flight-property-query-public")
        add_test(NAME native_death_cycle_flight_query_public
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_flight_property_query_public_tu15.py"
            --death-cycle --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --native "$<TARGET_FILE:native_flight_property_query_public_tests>"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-death-cycle-flight-query-public")
        set_tests_properties(native_flight_property_query_public native_death_cycle_flight_query_public
            PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_flight_query_sanitizers)
    unset(_halo2_flight_query_binding)
endif()
