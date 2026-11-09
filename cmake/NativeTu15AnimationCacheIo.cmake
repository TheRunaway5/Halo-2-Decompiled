if(BUILD_TESTING)
    add_custom_target(native_animation_cache_io_tests)
    set(_halo2_animation_cache_io_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_animation_cache_io_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_animation_cache_io_policy
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_source_animation_cache_io_tu15.py"
        --report "${CMAKE_CURRENT_BINARY_DIR}/source-animation-cache-io-tu15.json"
        ${_halo2_animation_cache_io_sanitizers})
    set_tests_properties(original_tu15_animation_cache_io_policy PROPERTIES TIMEOUT 180)
    unset(_halo2_animation_cache_io_sanitizers)
endif()
