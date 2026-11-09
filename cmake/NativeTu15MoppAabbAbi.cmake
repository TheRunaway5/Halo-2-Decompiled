if(BUILD_TESTING)
    add_custom_target(native_mopp_aabb_abi_tests)
    add_test(NAME native_world_observation_generator
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_physics_world_observation_tu15.py")
    set_tests_properties(native_world_observation_generator PROPERTIES TIMEOUT 120)
    set(_halo2_mopp_aabb_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_mopp_aabb_sanitizers --sanitizers)
    endif()
    set(HALO2_MAPPACK_GAMEPLAY_XBE
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03"
        CACHE FILEPATH "Owned offline MapPack 1.1 kernel reference")
    add_test(NAME original_mappack_native_mopp_aabb_abi
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_mopp_aabb_mappack_tu11.py"
        --xbe "${HALO2_MAPPACK_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-mopp-aabb-mappack.json"
        --require-binding --require-canonical-binding ${_halo2_mopp_aabb_sanitizers})
    set_tests_properties(original_mappack_native_mopp_aabb_abi PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_native_mopp_aabb_abi
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_mopp_aabb_abi_tu15.py"
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
        --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-mopp-aabb-abi.json"
        --require-binding --require-canonical-binding ${_halo2_mopp_aabb_sanitizers})
    set_tests_properties(original_tu15_native_mopp_aabb_abi PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    if(HALO2_SANITIZERS AND NOT MSVC)
        add_test(NAME original_mappack_native_mopp_aabb_abi_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_mopp_aabb_mappack_tu11.py"
            --xbe "${HALO2_MAPPACK_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-mopp-aabb-mappack-release.json"
            --require-binding --require-canonical-binding)
        set_tests_properties(original_mappack_native_mopp_aabb_abi_release PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
        add_test(NAME original_tu15_native_mopp_aabb_abi_release
            COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_mopp_aabb_abi_tu15.py"
            --xbe "${HALO2_TU15_GAMEPLAY_XBE}" --compiler "${CMAKE_CXX_COMPILER}"
            --report "${CMAKE_CURRENT_BINARY_DIR}/native-tu15-mopp-aabb-abi-release.json"
            --require-binding --require-canonical-binding)
        set_tests_properties(original_tu15_native_mopp_aabb_abi_release PROPERTIES TIMEOUT 900 SKIP_RETURN_CODE 77)
    endif()
    unset(_halo2_mopp_aabb_sanitizers)
endif()
