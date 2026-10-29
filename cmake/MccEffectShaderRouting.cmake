target_sources(halo2_cache PRIVATE src/rendering/effects/particle_material_validation.cpp)
if(BUILD_TESTING)
    set(HALO2_MCC_PARTICLE_ROUTING_MAP "" CACHE FILEPATH
        "Prepared MCC map for breakable particle custom Vulkan shader routing validation")
    set(HALO2_MCC_PARTICLE_GPU_CATALOG "${HALO2_MCC_PARTICLE_ROUTING_MAP}" CACHE PATH
        "Prepared MCC map or maps directory for complete particle GPU admission validation")
    add_executable(particle_material_validation_tests tests/particle_material_validation_tests.cpp)
    target_link_libraries(particle_material_validation_tests PRIVATE halo2_cache)
    add_test(NAME particle_material_resource_validation COMMAND particle_material_validation_tests)
    add_executable(world_effect_resource_admission_tests tests/world_effect_resource_admission_tests.cpp)
    target_link_libraries(world_effect_resource_admission_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME world_effect_resource_admission COMMAND world_effect_resource_admission_tests
        "${HALO2_MCC_PARTICLE_ROUTING_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/world-effect-resource-admission.json")
    set_tests_properties(world_effect_resource_admission PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    if(TARGET halo2_particle_renderer)
        add_executable(particle_gpu_admission_tests tests/particle_gpu_admission_tests.cpp)
        target_link_libraries(particle_gpu_admission_tests PRIVATE halo2_particle_renderer nlohmann_json::nlohmann_json)
        if(UNIX AND NOT APPLE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_definitions(particle_gpu_admission_tests PRIVATE HALO2_PARTICLE_TEST_LINK_WRAP=1)
            target_link_options(particle_gpu_admission_tests PRIVATE "LINKER:--wrap=vkCreateSampler" "LINKER:--wrap=vkCreateBuffer")
        endif()
        add_test(NAME particle_gpu_resource_admission COMMAND particle_gpu_admission_tests
            "${HALO2_RUNTIME_SHADER_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/particle-gpu-admission.json"
            "${HALO2_MCC_PARTICLE_GPU_CATALOG}")
        set_tests_properties(particle_gpu_resource_admission PROPERTIES TIMEOUT 900 RESOURCE_LOCK halo2_gpu)
    endif()
    add_executable(mcc_particle_shader_routing_tests tests/mcc_particle_shader_routing_tests.cpp)
    target_link_libraries(mcc_particle_shader_routing_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME mcc_particle_shader_routing COMMAND mcc_particle_shader_routing_tests
        "${HALO2_MCC_PARTICLE_ROUTING_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/mcc-particle-shader-routing.json")
    set_tests_properties(mcc_particle_shader_routing PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    if(EXISTS "${HALO2_MCC_PARTICLE_ROUTING_MAP}")
        get_filename_component(particle_routing_maps "${HALO2_MCC_PARTICLE_ROUTING_MAP}" DIRECTORY)
        foreach(level needle street_sweeper 03a_oldmombasa)
            if(EXISTS "${particle_routing_maps}/${level}.map")
                add_test(NAME mcc_particle_shader_routing_${level} COMMAND mcc_particle_shader_routing_tests
                    "${particle_routing_maps}/${level}.map"
                    "${CMAKE_CURRENT_BINARY_DIR}/mcc-particle-shader-routing-${level}.json")
                set_tests_properties(mcc_particle_shader_routing_${level} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
            endif()
        endforeach()
    endif()
endif()
