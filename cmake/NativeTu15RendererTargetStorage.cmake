target_sources(halo2_systems PRIVATE src/rendering/renderer/source_renderer_target_storage_tu15_generated.cpp src/rendering/renderer/native_tu15_renderer_target_storage_data.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_renderer_target_storage_tests
        tests/native_tu15_renderer_target_storage_tests.cpp
        src/rendering/renderer/source_renderer_target_storage_tu15_generated.cpp
        src/rendering/renderer/native_tu15_renderer_target_storage_data.cpp
        src/runtime/source_runtime_memory.cpp src/runtime/source_runtime_host_physical_memory.cpp)
    target_include_directories(native_tu15_renderer_target_storage_tests PRIVATE src)
    target_link_libraries(native_tu15_renderer_target_storage_tests PRIVATE
        halo2_physics_world_runtime halo2_snapshot_storage nlohmann_json::nlohmann_json)
    target_compile_options(native_tu15_renderer_target_storage_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME original_tu15_renderer_target_storage_constructor COMMAND native_tu15_renderer_target_storage_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-render-target-storage/original-constructor-abi/report.json)
    set_tests_properties(original_tu15_renderer_target_storage_constructor PROPERTIES TIMEOUT 60)
    foreach(target native_tu15_network_link_tests native_tu15_graphics_startup_tests)
        target_sources(${target} PRIVATE src/rendering/renderer/source_renderer_target_storage_tu15_generated.cpp src/rendering/renderer/native_tu15_renderer_target_storage_data.cpp)
    endforeach()
    foreach(input none virtual-four)
        if(input STREQUAL none)
            set(suffix "")
        else()
            set(suffix _controller)
        endif()
        add_test(NAME native_tu15_graphics_target_storage${suffix}_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-target-storage --input-devices ${input})
        set_tests_properties(native_tu15_graphics_target_storage${suffix}_startup PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    endforeach()
endif()
