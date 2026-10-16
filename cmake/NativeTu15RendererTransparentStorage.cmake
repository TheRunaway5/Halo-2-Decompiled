target_sources(halo2_systems PRIVATE
    src/source_renderer_transparent_storage_tu15_generated.cpp
    src/native_tu15_renderer_transparent_storage_data.cpp
    src/source_renderer_tail_storage_tu15_generated.cpp
    src/native_tu15_renderer_tail_storage_data.cpp)
if(BUILD_TESTING)
    add_executable(native_tu15_renderer_transparent_storage_tests
        tests/native_tu15_renderer_transparent_storage_tests.cpp
        src/source_renderer_transparent_storage_tu15_generated.cpp
        src/native_tu15_renderer_transparent_storage_data.cpp
        src/native_tu15_network_storage_data.cpp
        src/physics_pool_host_storage.cpp src/source_runtime_memory.cpp
        src/source_runtime_host_storage.cpp src/source_runtime_host_virtual_memory.cpp)
    target_include_directories(native_tu15_renderer_transparent_storage_tests PRIVATE src)
    target_link_libraries(native_tu15_renderer_transparent_storage_tests PRIVATE
        halo2_physics_world_runtime halo2_snapshot_storage nlohmann_json::nlohmann_json OpenSSL::Crypto)
    target_compile_options(native_tu15_renderer_transparent_storage_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    add_test(NAME original_tu15_renderer_transparent_storage_constructor COMMAND native_tu15_renderer_transparent_storage_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-renderer-transparent-storage/original-differential/report.json)
    set_tests_properties(original_tu15_renderer_transparent_storage_constructor PROPERTIES TIMEOUT 60)
    foreach(target native_tu15_network_link_tests native_tu15_graphics_startup_tests)
        target_sources(${target} PRIVATE
            src/source_renderer_transparent_storage_tu15_generated.cpp
            src/native_tu15_renderer_transparent_storage_data.cpp
            src/source_renderer_tail_storage_tu15_generated.cpp
            src/native_tu15_renderer_tail_storage_data.cpp)
    endforeach()
    foreach(input none virtual-four)
        if(input STREQUAL none)
            set(suffix "")
        else()
            set(suffix _controller)
        endif()
        add_test(NAME native_tu15_graphics_transparent_storage${suffix}_startup COMMAND native_tu15_graphics_startup_tests --frontier graphics-transparent-storage --input-devices ${input})
        set_tests_properties(native_tu15_graphics_transparent_storage${suffix}_startup PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    endforeach()
endif()
