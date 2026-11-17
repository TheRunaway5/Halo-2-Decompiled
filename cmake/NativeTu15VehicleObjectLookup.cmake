if(BUILD_TESTING)
    option(HALO2_NATIVE_VEHICLE_OBJECT_LOOKUP_REQUIRE_BINDING "Require installed VehicleEnvironment lookup default" ON)
    set(_halo2_vehicle_lookup_binding)
    if(HALO2_NATIVE_VEHICLE_OBJECT_LOOKUP_REQUIRE_BINDING)
        list(APPEND _halo2_vehicle_lookup_binding --require-binding)
    endif()
    add_custom_target(native_vehicle_object_lookup_tests)
    set(_halo2_vehicle_lookup_sanitizers)
    set(_halo2_vehicle_lookup_public_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_vehicle_lookup_sanitizers --sanitizers --semantic-mutants)
        list(APPEND _halo2_vehicle_lookup_public_sanitizers --sanitizers)
    endif()
    add_test(NAME native_tu15_vehicle_object_lookup_raw
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_vehicle_object_lookup_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-vehicle-object-lookup-raw"
        ${_halo2_vehicle_lookup_sanitizers})
    add_test(NAME native_tu15_vehicle_object_lookup_generation
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/audit_native_vehicle_object_lookup_generation.py"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-vehicle-object-lookup-generation"
        ${_halo2_vehicle_lookup_binding})
    add_test(NAME native_tu15_vehicle_object_lookup_public
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_vehicle_object_lookup_public_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-vehicle-object-lookup-public"
        ${_halo2_vehicle_lookup_binding} ${_halo2_vehicle_lookup_public_sanitizers})
    add_test(NAME native_tu15_vehicle_object_lookup_public_controls
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_vehicle_object_lookup_public_tu15.py"
        --compiler "${CMAKE_CXX_COMPILER}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-vehicle-object-lookup-public-controls"
        --controls-only --mutations ${_halo2_vehicle_lookup_binding}
        ${_halo2_vehicle_lookup_public_sanitizers})
    set_tests_properties(native_tu15_vehicle_object_lookup_raw native_tu15_vehicle_object_lookup_generation
        native_tu15_vehicle_object_lookup_public native_tu15_vehicle_object_lookup_public_controls
        PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME native_tu15_vehicle_object_lookup_raw_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_vehicle_object_lookup_tu15.py"
            --compiler "${CMAKE_CXX_COMPILER}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-vehicle-object-lookup-raw-release")
        set_tests_properties(native_tu15_vehicle_object_lookup_raw_release PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
        add_test(NAME native_tu15_vehicle_object_lookup_public_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_vehicle_object_lookup_public_tu15.py"
            --compiler "${CMAKE_CXX_COMPILER}"
            --output "${CMAKE_CURRENT_BINARY_DIR}/native-vehicle-object-lookup-public-release"
            ${_halo2_vehicle_lookup_binding})
        set_tests_properties(native_tu15_vehicle_object_lookup_public_release PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_vehicle_lookup_sanitizers)
    unset(_halo2_vehicle_lookup_public_sanitizers)
    unset(_halo2_vehicle_lookup_binding)
endif()
