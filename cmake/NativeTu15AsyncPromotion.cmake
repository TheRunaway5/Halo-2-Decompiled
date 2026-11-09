if(BUILD_TESTING)
    # Actual native synchronization and the retained async child oracle are
    # compiled only in owned CTest scratch. Original i386 proof is segmented
    # at the declared host mutex boundaries, not a kernel-equivalence claim.
    add_custom_target(native_tu15_async_promote_tests)
    set(_halo2_async_promote_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_async_promote_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_async_promotion
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_async_promote_tu15.py
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-async-promotion.json
        --require-binding ${_halo2_async_promote_sanitizers})
    set_tests_properties(original_tu15_native_async_promotion PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    unset(_halo2_async_promote_sanitizers)
endif()
