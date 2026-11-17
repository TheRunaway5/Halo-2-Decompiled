if(BUILD_TESTING)
    set(_halo2_particle_envelope_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        set(_halo2_particle_envelope_sanitizers --sanitizers --semantic-mutants)
    endif()

    add_test(NAME original_tu15_native_particle_envelope_frame
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_envelope_frame_tu15.py"
        --prefixes --observers --compiler "${CMAKE_CXX_COMPILER}" --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-particle-envelope-frame" ${_halo2_particle_envelope_sanitizers})
    add_test(NAME native_particle_envelope_source
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_particle_envelope_source_tu15.py"
        --require-binding --compiler "${CMAKE_CXX_COMPILER}" --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --output "${CMAKE_CURRENT_BINARY_DIR}/native-particle-envelope-source" ${_halo2_particle_envelope_sanitizers})
    set_tests_properties(original_tu15_native_particle_envelope_frame native_particle_envelope_source
        PROPERTIES TIMEOUT 1800 SKIP_RETURN_CODE 77)
    unset(_halo2_particle_envelope_sanitizers)
endif()
