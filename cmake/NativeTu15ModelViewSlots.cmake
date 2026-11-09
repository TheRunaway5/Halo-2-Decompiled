target_sources(halo2_physics_world_runtime PRIVATE
    src/rendering/models/native_tu15_model_view_slots.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_prt_helpers_tests tests/native_tu15_prt_helpers_tests.cpp)
    target_include_directories(native_tu15_prt_helpers_tests PRIVATE include src)
    target_compile_features(native_tu15_prt_helpers_tests PRIVATE cxx_std_20)
    target_compile_options(native_tu15_prt_helpers_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_tu15_prt_helpers_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_prt_helpers_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME original_tu15_native_prt_helpers
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_prt_helpers.py
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --native $<TARGET_FILE:native_tu15_prt_helpers_tests>
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-prt-helpers.json)
    set_tests_properties(original_tu15_native_prt_helpers PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
    # Authenticated real130980 test boundary is compiled inside CTest scratch.
    add_custom_target(native_tu15_prt_cache_build_tests)
    set(_halo2_prt_build_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_prt_build_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_prt_cache_build
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_prt_cache_build.py
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-prt-cache-build.json
        --require-binding ${_halo2_prt_build_sanitizers})
    set_tests_properties(original_tu15_native_prt_cache_build PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    unset(_halo2_prt_build_sanitizers)
    add_executable(native_tu15_model_lighting_leaves_tests tests/native_tu15_model_lighting_leaves_tests.cpp)
    target_include_directories(native_tu15_model_lighting_leaves_tests PRIVATE include src)
    target_compile_features(native_tu15_model_lighting_leaves_tests PRIVATE cxx_std_20)
    target_compile_options(native_tu15_model_lighting_leaves_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_tu15_model_lighting_leaves_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_model_lighting_leaves_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME original_tu15_native_model_lighting_leaves
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_model_lighting_leaves.py
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --native $<TARGET_FILE:native_tu15_model_lighting_leaves_tests>
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-model-lighting-leaves.json --require-binding)
    set_tests_properties(original_tu15_native_model_lighting_leaves PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
    add_executable(native_tu15_model_light_source_tests tests/native_tu15_model_light_source_tests.cpp)
    target_include_directories(native_tu15_model_light_source_tests PRIVATE include src)
    target_compile_features(native_tu15_model_light_source_tests PRIVATE cxx_std_20)
    target_compile_options(native_tu15_model_light_source_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_tu15_model_light_source_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_model_light_source_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME original_tu15_native_model_light_source
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_model_light_source.py
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --native $<TARGET_FILE:native_tu15_model_light_source_tests>
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-model-light-source.json --require-binding)
    set_tests_properties(original_tu15_native_model_light_source PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
    add_executable(native_tu15_model_vector_length_tests tests/native_tu15_model_vector_length_tests.cpp)
    target_include_directories(native_tu15_model_vector_length_tests PRIVATE include src)
    target_compile_features(native_tu15_model_vector_length_tests PRIVATE cxx_std_20)
    target_compile_options(native_tu15_model_vector_length_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_tu15_model_vector_length_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_model_vector_length_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME original_tu15_native_model_vector_length
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_model_vector_length.py
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --native $<TARGET_FILE:native_tu15_model_vector_length_tests>
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-model-vector-length.json --require-binding)
    set_tests_properties(original_tu15_native_model_vector_length PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 77)
    # The actual production pool-owner APIs are linked only in owned CTest
    # scratch, retaining their still-instruction-shaped work as explicit debt.
    add_custom_target(native_tu15_model_light_cache_tests)
    set(_halo2_light_cache_sanitizers)
    if(HALO2_SANITIZERS AND NOT MSVC)
        list(APPEND _halo2_light_cache_sanitizers --sanitizers)
    endif()
    add_test(NAME original_tu15_native_model_light_cache
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_model_light_cache.py
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-model-light-cache.json
        --require-binding ${_halo2_light_cache_sanitizers})
    set_tests_properties(original_tu15_native_model_light_cache PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    unset(_halo2_light_cache_sanitizers)
    add_executable(native_tu15_model_view_slots_tests
        tests/native_tu15_model_view_slots_tests.cpp
        src/rendering/models/native_tu15_model_view_slots.cpp)
    target_include_directories(native_tu15_model_view_slots_tests PRIVATE include src)
    target_compile_features(native_tu15_model_view_slots_tests PRIVATE cxx_std_20)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(native_tu15_model_view_slots_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(native_tu15_model_view_slots_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_tu15_model_view_slots COMMAND native_tu15_model_view_slots_tests)
    add_test(NAME original_tu15_native_model_view_slots
        COMMAND "${HALO2_RENDERER_ORACLE_PYTHON}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_native_tu15_model_view_slots.py
        --xbe "${HALO2_TU15_GAMEPLAY_XBE}"
        --native $<TARGET_FILE:native_tu15_model_view_slots_tests>
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-tu15-model-view-slots.json)
    set_tests_properties(original_tu15_native_model_view_slots PROPERTIES TIMEOUT 60 SKIP_RETURN_CODE 77)
endif()
