if(BUILD_TESTING)
    option(HALO2_NATIVE_MAP_WORLD_PROPERTY_QUERY_REQUIRE_BINDING
        "Require both native MapWorld property owners" ON)
    add_custom_target(native_map_world_property_query_frame_tests)
    set(_halo2_map_world_frame_sanitizers)
    set(_halo2_map_world_source_sanitizers)
    set(_halo2_map_world_binding)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_map_world_frame_sanitizers --sanitizers)
        list(APPEND _halo2_map_world_source_sanitizers --sanitizers --semantic-mutants)
    endif()
    if(HALO2_NATIVE_MAP_WORLD_PROPERTY_QUERY_REQUIRE_BINDING)
        list(APPEND _halo2_map_world_binding --require-binding)
    endif()
    foreach(_halo2_property_owner query caller)
        set(_halo2_map_world_current_frame ${_halo2_map_world_frame_sanitizers})
        if(_halo2_property_owner STREQUAL "query" AND HALO2_SANITIZERS AND NOT MSVC)
            list(APPEND _halo2_map_world_current_frame --semantic-mutants)
        endif()
        add_test(NAME original_tu15_native_map_world_property_${_halo2_property_owner}_frame
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_map_world_property_${_halo2_property_owner}_frame_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-map-world-property-${_halo2_property_owner}-frame"
            --prefixes ${_halo2_map_world_current_frame})
        add_test(NAME original_tu15_native_map_world_property_${_halo2_property_owner}_source
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_map_world_property_${_halo2_property_owner}_source_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-map-world-property-${_halo2_property_owner}-source"
            ${_halo2_map_world_source_sanitizers} ${_halo2_map_world_binding})
        set_tests_properties(original_tu15_native_map_world_property_${_halo2_property_owner}_frame
            original_tu15_native_map_world_property_${_halo2_property_owner}_source
            PROPERTIES TIMEOUT 2400 SKIP_RETURN_CODE 77)
    endforeach()
    add_test(NAME native_map_world_property_queries_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_map_world_property_query_binding_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-map-world-property-binding" ${_halo2_map_world_binding})
    set_tests_properties(native_map_world_property_queries_binding PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    if(HALO2_NATIVE_MAP_WORLD_PROPERTY_QUERY_REQUIRE_BINDING AND CMAKE_SYSTEM_NAME STREQUAL "Linux"
        AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|i.86)$")
        add_executable(native_map_world_property_queries_public_tests tests/native_tu15_map_world_property_queries_public_tests.cpp)
        target_include_directories(native_map_world_property_queries_public_tests PRIVATE include src)
        target_compile_features(native_map_world_property_queries_public_tests PRIVATE cxx_std_20)
        target_link_libraries(native_map_world_property_queries_public_tests PRIVATE halo2_physics_world_runtime nlohmann_json::nlohmann_json)
        add_dependencies(native_map_world_property_query_frame_tests native_map_world_property_queries_public_tests)
        add_test(NAME native_map_world_property_queries_public
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_map_world_property_queries_public_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --native "$<TARGET_FILE:native_map_world_property_queries_public_tests>"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-map-world-property-public")
        set_tests_properties(native_map_world_property_queries_public PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_map_world_frame_sanitizers)
    unset(_halo2_map_world_source_sanitizers)
    unset(_halo2_map_world_current_frame)
    unset(_halo2_map_world_binding)
endif()
