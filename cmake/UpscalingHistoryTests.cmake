# Opt-in actual TemporalUpscaler input regressions. run_tests.py owns all build
# products and each CTest's run_test.py wrapper owns its transient runtime data.
option(HALO2_UPSCALING_HISTORY_TESTS "Test prepared temporal motion/depth/history inputs" OFF)
if(BUILD_TESTING AND HALO2_UPSCALING_HISTORY_TESTS)
    add_executable(upscaling_history_tests tests/upscaling_history_tests.cpp)
    target_include_directories(upscaling_history_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries(upscaling_history_tests PRIVATE halo2_upscaling halo2_path_tracing)
    add_dependencies(upscaling_history_tests halo2_shaders)
    foreach(history_case IN ITEMS world foreground sky reorder jitter deferred-world deferred-foreground)
        add_test(NAME upscaling_history_${history_case} COMMAND upscaling_history_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders ${history_case})
        set_tests_properties(upscaling_history_${history_case} PROPERTIES
            RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 45)
        if(EXISTS "${PROJECT_SOURCE_DIR}/.tools/validation/usr/lib/libVkLayer_khronos_validation.so")
            set_property(TEST upscaling_history_${history_case} APPEND PROPERTY ENVIRONMENT
                "VK_LAYER_PATH=${PROJECT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d"
                "LD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/.tools/validation/usr/lib:$ENV{LD_LIBRARY_PATH}")
        endif()
    endforeach()
endif()
