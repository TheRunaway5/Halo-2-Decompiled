target_sources(halo2_systems PRIVATE
    src/source_startup_sound_storage_tu15_generated.cpp
    src/native_tu15_startup_sound_storage_data.cpp)
if(BUILD_TESTING)
    foreach(component sound_arena_prefix sound_storage_prefix startup_sound_storage)
        add_executable(native_tu15_${component}_tests tests/native_tu15_${component}_tests.cpp
            src/source_startup_sound_storage_tu15_generated.cpp
            src/native_tu15_startup_sound_storage_data.cpp
            src/source_render_object_startup_tu15_generated.cpp
            src/source_runtime_memory.cpp src/source_runtime_host_storage.cpp
            src/source_runtime_host_virtual_memory.cpp src/physics_pool_host_storage.cpp)
        target_include_directories(native_tu15_${component}_tests PRIVATE src)
        target_link_libraries(native_tu15_${component}_tests PRIVATE
            halo2_physics_world_runtime halo2_snapshot_storage nlohmann_json::nlohmann_json)
        target_compile_options(native_tu15_${component}_tests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>)
    endforeach()
    set(reference_dir ${CMAKE_CURRENT_SOURCE_DIR}/analysis/asset-setup/mcc-native-20261015/native-startup-45-50)
    add_test(NAME original_tu15_sound_arena_prefix COMMAND native_tu15_sound_arena_prefix_tests
        ${reference_dir}/original-sound-arena-prefix/report.json)
    add_test(NAME original_tu15_sound_storage_prefix COMMAND native_tu15_sound_storage_prefix_tests
        ${reference_dir}/original-sound-storage-prefix/report.json)
    add_test(NAME original_tu15_startup46_through50_storage COMMAND native_tu15_startup_sound_storage_tests
        ${reference_dir}/original-46-50/report.json)
    set_tests_properties(original_tu15_sound_arena_prefix original_tu15_sound_storage_prefix
        original_tu15_startup46_through50_storage PROPERTIES TIMEOUT 60)
    foreach(target native_tu15_network_link_tests native_tu15_graphics_startup_tests)
        target_sources(${target} PRIVATE src/source_startup_sound_storage_tu15_generated.cpp
            src/native_tu15_startup_sound_storage_data.cpp)
    endforeach()
    foreach(input none virtual-four)
        if(input STREQUAL none)
            set(suffix "")
        else()
            set(suffix _controller)
        endif()
        add_test(NAME native_tu15_sound_storage${suffix}_startup
            COMMAND native_tu15_graphics_startup_tests --frontier sound-storage --input-devices ${input})
        set_tests_properties(native_tu15_sound_storage${suffix}_startup
            PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    endforeach()
endif()
