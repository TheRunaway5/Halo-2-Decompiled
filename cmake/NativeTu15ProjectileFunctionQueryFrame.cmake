if(BUILD_TESTING)
    option(HALO2_NATIVE_PROJECTILE_FUNCTION_QUERY_REQUIRE_BINDING
        "Require the native CombatAction projectile query owner" ON)
    add_custom_target(native_projectile_function_query_frame_tests)
    set(_halo2_projectile_query_frame_sanitizers)
    set(_halo2_projectile_query_binding)
    if(HALO2_NATIVE_PROJECTILE_FUNCTION_QUERY_REQUIRE_BINDING)
        list(APPEND _halo2_projectile_query_binding --require-binding)
    endif()
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_projectile_query_frame_sanitizers --sanitizers --semantic-mutants)
    endif()
    add_test(NAME original_tu15_native_projectile_function_query_frame
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_projectile_function_query_frame_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-projectile-function-query-frame"
        --extended-fp --prefixes --observers ${_halo2_projectile_query_frame_sanitizers})
    set_tests_properties(original_tu15_native_projectile_function_query_frame PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_tu15_native_projectile_function_query_frame_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_projectile_function_query_frame_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-projectile-function-query-frame-release"
            --extended-fp --prefixes --observers)
        set_tests_properties(original_tu15_native_projectile_function_query_frame_release PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    endif()
    add_test(NAME original_tu15_native_projectile_function_query_source
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_projectile_function_query_source_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-projectile-function-query-source"
        --extended-fp ${_halo2_projectile_query_frame_sanitizers} ${_halo2_projectile_query_binding})
    set_tests_properties(original_tu15_native_projectile_function_query_source PROPERTIES TIMEOUT 2400 SKIP_RETURN_CODE 77)
    add_test(NAME native_projectile_function_query_binding
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_projectile_function_query_binding_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-projectile-function-query-binding" ${_halo2_projectile_query_binding})
    set_tests_properties(native_projectile_function_query_binding PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    if(HALO2_NATIVE_PROJECTILE_FUNCTION_QUERY_REQUIRE_BINDING AND CMAKE_SYSTEM_NAME STREQUAL "Linux"
        AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|i.86)$")
        add_executable(native_projectile_function_query_public_tests tests/native_tu15_projectile_function_query_public_tests.cpp)
        target_include_directories(native_projectile_function_query_public_tests PRIVATE include src)
        target_compile_features(native_projectile_function_query_public_tests PRIVATE cxx_std_20)
        target_link_libraries(native_projectile_function_query_public_tests PRIVATE halo2_physics_world_runtime nlohmann_json::nlohmann_json)
        add_dependencies(native_projectile_function_query_frame_tests native_projectile_function_query_public_tests)
        add_test(NAME native_projectile_function_query_public
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_projectile_function_query_public_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --native "$<TARGET_FILE:native_projectile_function_query_public_tests>"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-projectile-function-query-public")
        set_tests_properties(native_projectile_function_query_public PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_projectile_query_frame_sanitizers)
    unset(_halo2_projectile_query_binding)
endif()
