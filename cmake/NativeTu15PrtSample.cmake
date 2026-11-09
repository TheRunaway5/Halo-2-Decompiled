if(BUILD_TESTING)
    add_test(NAME native_tu15_prt_bindings
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_model_prt_native_bindings.py)
    add_executable(native_tu15_prt_sample_tests tests/native_tu15_prt_sample_tests.cpp)
    target_include_directories(native_tu15_prt_sample_tests PRIVATE include src)
    target_compile_features(native_tu15_prt_sample_tests PRIVATE cxx_std_20)
    target_compile_options(native_tu15_prt_sample_tests PRIVATE
        "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-fast-math;-ffp-contract=off;-frounding-math>")
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_tu15_prt_sample_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_prt_sample_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME original_tu15_native_prt_sample
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_prt_sample.py
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --native $<TARGET_FILE:native_tu15_prt_sample_tests>
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-prt-sample.json)
    set_tests_properties(original_tu15_native_prt_sample PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
    # The genuine130980 reference header is authenticated/extracted and
    # compiled only inside the CTest-owned scratch directory.
    add_custom_target(native_tu15_prt_cache_tests)
    set(_halo2_prt_cache_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_prt_cache_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_prt_cache
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_prt_cache.py
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-prt-cache.json
        ${_halo2_prt_cache_sanitizers})
    set_tests_properties(original_tu15_native_prt_cache PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
    unset(_halo2_prt_cache_sanitizers)
    add_executable(native_tu15_prt_update_tests tests/native_tu15_prt_update_tests.cpp
        src/rendering/models/native_tu15_model_view_slots.cpp)
    target_include_directories(native_tu15_prt_update_tests PRIVATE include src)
    target_compile_features(native_tu15_prt_update_tests PRIVATE cxx_std_20)
    target_compile_options(native_tu15_prt_update_tests PRIVATE
        "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-fast-math;-ffp-contract=off;-frounding-math>")
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_tu15_prt_update_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_prt_update_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME original_tu15_native_prt_update
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_prt_update.py
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --native $<TARGET_FILE:native_tu15_prt_update_tests>
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-prt-update.json)
    set_tests_properties(original_tu15_native_prt_update PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
endif()
