if(BUILD_TESTING AND NOT MSVC AND CMAKE_SYSTEM_PROCESSOR MATCHES "^([xX]86_64|[aA][mM][dD]64|[xX]64)$")
    # Recover the exact old header from its namespace-only test copy and pin
    # its bytes. Both predicates remain independent, including bit helpers.
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/tests/helpers/model_skin_gpu_admission_reference_20261023.hpp" admission_reference)
    string(REPLACE "// Exact pre-row-summary predicate. Only the enclosing namespace is renamed.\n// Original SHA-256: 7a5539bda19f59738aa84d01a33afaed5414fe50fafb940ebdcc92194410fc0a\n" "" admission_reference "${admission_reference}")
    string(REPLACE "namespace halo2::model_skin_gpu_admission_reference_20261023 {" "namespace halo2 {" admission_reference "${admission_reference}")
    string(SHA256 admission_reference_sha "${admission_reference}")
    if(NOT admission_reference_sha STREQUAL "7a5539bda19f59738aa84d01a33afaed5414fe50fafb940ebdcc92194410fc0a")
        message(FATAL_ERROR "Model skin arithmetic admission reference differs from its source-pinned old predicate")
    endif()
    foreach(admission_target model_skin_gpu_admission_tests model_skin_gpu_admission_mcc_tests)
        add_executable(${admission_target} EXCLUDE_FROM_ALL tests/model_skin_gpu_admission_tests.cpp)
        target_include_directories(${admission_target} PRIVATE include src)
        target_compile_features(${admission_target} PRIVATE cxx_std_20)
        target_compile_options(${admission_target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
        if(HALO2_SANITIZERS)
            target_compile_options(${admission_target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${admission_target} PRIVATE -fsanitize=address,undefined)
        endif()
    endforeach()
    target_compile_definitions(model_skin_gpu_admission_mcc_tests PRIVATE HALO2_ADMISSION_MCC_POSES=1)
    target_link_libraries(model_skin_gpu_admission_mcc_tests PRIVATE halo2_systems)
    add_test(NAME native_model_skin_gpu_admission COMMAND model_skin_gpu_admission_tests)
    set_tests_properties(native_model_skin_gpu_admission PROPERTIES TIMEOUT 90)
    # Reach the real pose factory without linking the full gameplay runtime.
    # Unrelated cache/animation entry points in these owners are discarded.
    add_executable(model_skin_admission_cache_tests EXCLUDE_FROM_ALL
        tests/model_skin_admission_cache_tests.cpp
        src/rendering/models/model_pose.cpp
        src/rendering/models/source_model_palette.cpp src/core/transform.cpp)
    target_include_directories(model_skin_admission_cache_tests PRIVATE include src)
    target_link_libraries(model_skin_admission_cache_tests PRIVATE halo2_glm Threads::Threads)
    target_compile_features(model_skin_admission_cache_tests PRIVATE cxx_std_20)
    target_compile_options(model_skin_admission_cache_tests PRIVATE
        -fno-fast-math -ffp-contract=off -frounding-math -ffunction-sections -fdata-sections)
    if(APPLE)
        target_link_options(model_skin_admission_cache_tests PRIVATE -Wl,-dead_strip)
    else()
        target_link_options(model_skin_admission_cache_tests PRIVATE -Wl,--gc-sections)
    endif()
    if(HALO2_SANITIZERS)
        target_compile_options(model_skin_admission_cache_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(model_skin_admission_cache_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME native_model_skin_admission_cache COMMAND ${CMAKE_COMMAND} -E env
        --unset=HALO2_NATIVE_SKIN_ADMISSION_CACHE $<TARGET_FILE:model_skin_admission_cache_tests>)
    add_test(NAME native_model_skin_admission_cache_enabled COMMAND ${CMAKE_COMMAND} -E env
        HALO2_NATIVE_SKIN_ADMISSION_CACHE=1 $<TARGET_FILE:model_skin_admission_cache_tests>)
    add_test(NAME native_model_skin_admission_cache_reference_zero COMMAND ${CMAKE_COMMAND} -E env
        HALO2_NATIVE_SKIN_ADMISSION_CACHE=0 $<TARGET_FILE:model_skin_admission_cache_tests> --reference-cache)
    add_test(NAME native_model_skin_admission_cache_reference_empty COMMAND ${CMAKE_COMMAND} -E env
        HALO2_NATIVE_SKIN_ADMISSION_CACHE= $<TARGET_FILE:model_skin_admission_cache_tests> --reference-cache)
    set_tests_properties(native_model_skin_admission_cache native_model_skin_admission_cache_enabled
        native_model_skin_admission_cache_reference_zero native_model_skin_admission_cache_reference_empty
        PROPERTIES TIMEOUT 30)
    set(HALO2_MODEL_SKIN_ADMISSION_MCC_MAP "" CACHE FILEPATH "Explicit prepared MCC map for source-pinned skin arithmetic pose differential")
    if(NOT "${HALO2_MODEL_SKIN_ADMISSION_MCC_MAP}" STREQUAL "")
        if(NOT IS_ABSOLUTE "${HALO2_MODEL_SKIN_ADMISSION_MCC_MAP}" OR
           NOT EXISTS "${HALO2_MODEL_SKIN_ADMISSION_MCC_MAP}" OR IS_DIRECTORY "${HALO2_MODEL_SKIN_ADMISSION_MCC_MAP}")
            message(FATAL_ERROR "HALO2_MODEL_SKIN_ADMISSION_MCC_MAP requires an existing absolute prepared MCC map")
        endif()
        add_test(NAME native_mcc_model_skin_gpu_admission COMMAND model_skin_gpu_admission_mcc_tests "${HALO2_MODEL_SKIN_ADMISSION_MCC_MAP}")
        set_tests_properties(native_mcc_model_skin_gpu_admission PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE
            LABELS "genuine-mcc-input;selected-mcc-native;gpu-skin-admission-component")
    endif()
endif()
