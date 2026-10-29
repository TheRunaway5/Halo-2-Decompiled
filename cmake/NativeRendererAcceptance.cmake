target_include_directories(halo2_pstencil_render PRIVATE ${NATIVE_WORLD_KERNEL_CATALOG_DIR})
add_dependencies(halo2_pstencil_render native_world_kernels)
target_include_directories(halo2_shadow_render PRIVATE ${NATIVE_WORLD_KERNEL_CATALOG_DIR})
add_dependencies(halo2_shadow_render native_world_kernels)
if(BUILD_TESTING)
    # The instruction evaluator is an offline numerical oracle. It is never a
    # source or dependency of the game/cache/renderer libraries.
    add_library(halo2_vertex_program_reference STATIC EXCLUDE_FROM_ALL
        tests/reference/vertex_program_reference.cpp)
    target_include_directories(halo2_vertex_program_reference PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_compile_options(halo2_vertex_program_reference PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off>
        $<$<CXX_COMPILER_ID:MSVC>:/fp:strict>)
    function(halo2_link_vertex_reference_tests)
        get_property(reference_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
        foreach(candidate IN LISTS reference_targets)
            get_target_property(candidate_type ${candidate} TYPE)
            if(candidate_type STREQUAL "EXECUTABLE" AND candidate MATCHES "(_tests$|^test_)")
                target_link_libraries(${candidate} PRIVATE halo2_vertex_program_reference)
            endif()
        endforeach()
        foreach(candidate menu_world_vertex_source_tests menu_world_vertex_catalog_tests
                source_effect_light_raw_tests source_effect_light_frame_tests
                pstencil_projected_vertex_attributes_gpu_tests)
            if(TARGET ${candidate})
                target_link_libraries(${candidate} PRIVATE halo2_vertex_program_codegen)
            endif()
        endforeach()
    endfunction()
    cmake_language(DEFER CALL halo2_link_vertex_reference_tests)
    foreach(pair
            "native_renderer_instruction_guard;test_native_renderer_kernel_audit.py"
            "native_renderer_effect_programs;test_native_effect_fragment_registry.py"
            "native_renderer_weather_programs;test_native_weather_kernels.py"
            "native_renderer_fragment_families;test_native_fragment_kernels.py"
            "native_renderer_shader_build_completion;test_native_shader_build_completion.py"
            "native_renderer_combiner_admission;test_native_combiner_admission.py"
            "native_renderer_shader_package;test_stage_native_renderer_shaders.py"
            "native_renderer_aot_admission;test_native_renderer_aot_capability.py"
            "native_renderer_catalog_switch_domain;test_native_catalog_variants.py"
            "native_renderer_material_projection;test_project_native_material_fragments.py"
            "native_renderer_world_source_projection;test_project_native_world_sources.py"
            "native_renderer_raw_vertex_metadata;test_native_raw_vertex_metadata.py"
            "native_renderer_cpu_vertex_lowering;test_native_cpu_vertex_kernel.py"
            "native_shadow_ordered_position;test_source_world_depth_position.py"
            "native_shadow_model_depth_position;test_source_model_depth_position.py"
            "native_shadow_authored_cull;test_source_world_depth_raster.py")
        list(GET pair 0 test_name)
        list(GET pair 1 test_source)
        add_test(NAME ${test_name} COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
            -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p ${test_source})
        set_tests_properties(${test_name} PROPERTIES TIMEOUT 120 LABELS "native-renderer")
    endforeach()
    set_tests_properties(native_renderer_cpu_vertex_lowering PROPERTIES TIMEOUT 360)
    add_executable(native_weather_vertex_tests EXCLUDE_FROM_ALL tests/native_weather_vertex_tests.cpp)
    target_link_libraries(native_weather_vertex_tests PRIVATE halo2_weather_render)
    target_compile_options(native_weather_vertex_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off>
        $<$<CXX_COMPILER_ID:MSVC>:/fp:strict>)
    add_test(NAME native_weather_vertex COMMAND native_weather_vertex_tests)
    add_executable(native_renderer_all_maps_gpu_tests EXCLUDE_FROM_ALL tests/native_renderer_all_maps_gpu_tests.cpp)
    target_link_libraries(native_renderer_all_maps_gpu_tests PRIVATE halo2_renderer OpenSSL::Crypto nlohmann_json::nlohmann_json)
    target_compile_definitions(native_renderer_all_maps_gpu_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME native_renderer_no_runtime_interpretation COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/test_native_renderer_package.py
        --build-dir ${CMAKE_CURRENT_BINARY_DIR} --source-root ${CMAKE_CURRENT_SOURCE_DIR}
        --report ${CMAKE_CURRENT_BINARY_DIR}/native-renderer-production-acceptance.json)
    set_tests_properties(native_renderer_no_runtime_interpretation PROPERTIES TIMEOUT 180 RUN_SERIAL TRUE LABELS "native-renderer")
    add_test(NAME native_renderer_launcher_admission COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_renderer_launcher_admission.py
        --launcher $<TARGET_FILE:halo2_launcher>)
    set_tests_properties(native_renderer_launcher_admission PROPERTIES TIMEOUT 120 LABELS "native-renderer")
endif()
