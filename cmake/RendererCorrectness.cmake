if(BUILD_TESTING)
    add_executable(cinematic_camera_interpolation_tests tests/cinematic_camera_interpolation_tests.cpp
        src/campaign/cinematic_camera_interpolation.cpp src/campaign/campaign_frame_clock.cpp)
    target_include_directories(cinematic_camera_interpolation_tests PRIVATE include)
    add_test(NAME native_cinematic_camera_interpolation COMMAND cinematic_camera_interpolation_tests)
    add_test(NAME native_cinematic_capture_resolution COMMAND cinematic_crossfade_gpu_tests
        ${CMAKE_CURRENT_BINARY_DIR}/shaders)
    set_tests_properties(native_cinematic_capture_resolution PROPERTIES RUN_SERIAL TRUE
        ENVIRONMENT "VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation;VK_LAYER_VALIDATE_SYNC=1")
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        # Real translated stores pass the physical protection adapter even
        # when their source range cannot intersect a physical allocation.
        add_executable(source_physical_write_guard_tests tests/source_physical_write_guard_tests.cpp
            src/runtime/source_runtime_memory.cpp src/runtime/source_runtime_host_physical_memory.cpp)
        target_include_directories(source_physical_write_guard_tests PRIVATE include)
        target_link_libraries(source_physical_write_guard_tests PRIVATE halo2_snapshot_storage Threads::Threads)
        target_link_options(source_physical_write_guard_tests PRIVATE -Wl,--wrap=pthread_mutex_lock)
        add_test(NAME source_physical_write_guard COMMAND source_physical_write_guard_tests)
    endif()
    # Exact image fixtures need a Vulkan presentation surface, but no physical
    # desktop. A private server also keeps them independent of monitor hotplug.
    find_program(HALO2_RENDERER_XVFB_RUN xvfb-run)
    set(_renderer_image_display)
    if(HALO2_RENDERER_XVFB_RUN)
        set(_renderer_image_display ${HALO2_RENDERER_XVFB_RUN} -a
            "--server-args=-screen 0 1280x720x24 -nolisten tcp")
    endif()
    add_executable(campaign_material_selection_tests tests/campaign_material_selection_tests.cpp)
    target_include_directories(campaign_material_selection_tests PRIVATE src)
    target_link_libraries(campaign_material_selection_tests PRIVATE halo2_cache)
    add_test(NAME native_campaign_material_selection COMMAND campaign_material_selection_tests)

    add_executable(material_descriptor_range_tests tests/material_descriptor_range_tests.cpp)
    target_include_directories(material_descriptor_range_tests PRIVATE src)
    add_test(NAME native_material_descriptor_ranges COMMAND material_descriptor_range_tests)

    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/vertex_reciprocal.comp.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${GLSLANG_VALIDATOR} -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/vertex_reciprocal.comp
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/vertex_reciprocal.comp.spv
        DEPENDS tests/vertex_reciprocal.comp shaders/vertex_program.glsl shaders/render_view.glsl VERBATIM)
    add_custom_target(vertex_reciprocal_test_shader DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/vertex_reciprocal.comp.spv)
    add_executable(vertex_reciprocal_tests tests/vertex_reciprocal_tests.cpp)
    target_link_libraries(vertex_reciprocal_tests PRIVATE halo2_cache Vulkan::Vulkan)
    target_compile_definitions(vertex_reciprocal_tests PRIVATE
        HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        HALO2_VERTEX_RECIPROCAL_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/vertex_reciprocal.comp.spv")
    add_dependencies(vertex_reciprocal_tests vertex_reciprocal_test_shader)
    if(MSVC)
        target_compile_options(vertex_reciprocal_tests PRIVATE /fp:strict)
    else()
        target_compile_options(vertex_reciprocal_tests PRIVATE -fno-fast-math -ffp-contract=off)
    endif()
    add_test(NAME source_vertex_reciprocal_cpu COMMAND vertex_reciprocal_tests)
    add_test(NAME source_vertex_reciprocal_gpu COMMAND vertex_reciprocal_tests --gpu)
    set_tests_properties(source_vertex_reciprocal_gpu PROPERTIES ENVIRONMENT
        "VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation;VK_LAYER_VALIDATE_SYNC=1")

    add_executable(renderer_descriptor_tests tests/renderer_descriptor_tests.cpp)
    target_link_libraries(renderer_descriptor_tests PRIVATE halo2_renderer)
    # Rebuild only Renderer.cpp with the exact former full-scan iteration.
    # All other engine code, source shaders and Vulkan inputs stay shared.
    add_library(renderer_descriptor_reference OBJECT src/rendering/renderer/renderer.cpp)
    set_property(TARGET renderer_descriptor_reference PROPERTY EXCLUDE_FROM_ALL TRUE)
    target_link_libraries(renderer_descriptor_reference PRIVATE halo2_renderer)
    target_include_directories(renderer_descriptor_reference PRIVATE
        $<TARGET_PROPERTY:halo2_renderer,INCLUDE_DIRECTORIES>)
    target_compile_definitions(renderer_descriptor_reference PRIVATE
        $<TARGET_PROPERTY:halo2_renderer,COMPILE_DEFINITIONS> HALO2_REFERENCE_FULL_DESCRIPTOR_SCAN=1)
    add_dependencies(renderer_descriptor_reference halo2_renderer)
    add_executable(renderer_descriptor_reference_tests tests/renderer_descriptor_tests.cpp
        $<TARGET_OBJECTS:renderer_descriptor_reference>)
    target_link_libraries(renderer_descriptor_reference_tests PRIVATE halo2_renderer)
    add_test(NAME native_renderer_descriptor_equivalence COMMAND ${_renderer_image_display} ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_renderer_descriptor_equivalence.py
        --optimized $<TARGET_FILE:renderer_descriptor_tests>
        --reference $<TARGET_FILE:renderer_descriptor_reference_tests>
        --report ${CMAKE_CURRENT_BINARY_DIR}/renderer-descriptor-equivalence.json)
    set_tests_properties(native_renderer_descriptor_equivalence PROPERTIES RUN_SERIAL TRUE
        ENVIRONMENT "VK_LAYER_VALIDATE_SYNC=1")

    # Instrument the two real translation units containing the copied binding
    # lifetime and its actual lighting reads, sharing normal engine services.
    # This avoids requiring every translated physics/audio routine in ASAN.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT WIN32)
        add_library(renderer_lighting_sanitized OBJECT src/rendering/renderer/renderer.cpp src/gameplay/objects/object_lighting.cpp)
        set_property(TARGET renderer_lighting_sanitized PROPERTY EXCLUDE_FROM_ALL TRUE)
        target_link_libraries(renderer_lighting_sanitized PRIVATE halo2_renderer)
        target_include_directories(renderer_lighting_sanitized PRIVATE
            $<TARGET_PROPERTY:halo2_renderer,INCLUDE_DIRECTORIES>)
        target_compile_definitions(renderer_lighting_sanitized PRIVATE
            $<TARGET_PROPERTY:halo2_renderer,COMPILE_DEFINITIONS>)
        target_compile_options(renderer_lighting_sanitized PRIVATE
            -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -ffp-contract=off)
        add_dependencies(renderer_lighting_sanitized halo2_renderer)
        # This lifetime test never selects an upscaler. Compile the real bridge
        # without optional NGX discovery: VK_NVX_binary_import alone makes
        # NVIDIA's vkCreateDevice fail under ASAN before any renderer work.
        # Production and the dedicated DLSS tests retain their normal bridge.
        add_executable(renderer_transparent_lighting_tests tests/renderer_transparent_lighting_tests.cpp
            src/rendering/renderer/native_upscaler.cpp
            $<TARGET_OBJECTS:renderer_lighting_sanitized>)
        target_link_libraries(renderer_transparent_lighting_tests PRIVATE halo2_renderer)
        target_compile_definitions(renderer_transparent_lighting_tests PRIVATE
            HALO2_UPSCALER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/external/upscaling")
        target_compile_options(renderer_transparent_lighting_tests PRIVATE
            -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -UHALO2_HAS_NGX)
        target_link_options(renderer_transparent_lighting_tests PRIVATE -fsanitize=address,undefined)
        add_test(NAME native_renderer_transparent_lighting_sanitized COMMAND ${_renderer_image_display} $<TARGET_FILE:renderer_transparent_lighting_tests>
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
        set_tests_properties(native_renderer_transparent_lighting_sanitized PROPERTIES
            RUN_SERIAL TRUE SKIP_RETURN_CODE 77 ENVIRONMENT
            "ASAN_OPTIONS=detect_leaks=0:halt_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1;VK_LAYER_VALIDATE_SYNC=1")
    endif()
    if(HALO2_RENDERER_XVFB_RUN)
        foreach(test IN ITEMS native_renderer_descriptor_equivalence native_renderer_transparent_lighting_sanitized)
            if(TEST ${test})
                set_property(TEST ${test} APPEND PROPERTY ENVIRONMENT "SDL_VIDEODRIVER=x11")
            endif()
        endforeach()
    endif()
    # Match the existing source-world GPU tests: use the repository's bundled
    # validation layer when present, otherwise retain the system installation.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib/libVkLayer_khronos_validation.so")
        foreach(test IN ITEMS source_vertex_reciprocal_gpu native_cinematic_capture_resolution native_renderer_descriptor_equivalence
                native_renderer_transparent_lighting_sanitized)
            if(TEST ${test})
                set_property(TEST ${test} APPEND PROPERTY ENVIRONMENT
                    "VK_LAYER_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d"
                    "LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib:$ENV{LD_LIBRARY_PATH}")
            endif()
        endforeach()
    endif()
endif()
