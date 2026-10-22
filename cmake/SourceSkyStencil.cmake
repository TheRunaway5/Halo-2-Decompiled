# Bounded retained original20 sky stencil diagnostic. This library is not linked
# to Renderer and registers no live sky writer or authored vertex producer.
function(halo2_add_source_sky_stencil sky_source_root)
    option(HALO2_SKY_STENCIL_REFERENCE_TESTS
        "Build retained original20 sky stencil CPU/GPU reference checks (requires captured artifacts)" OFF)
    set(sky_shader_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    set(sky_shader_outputs)
    foreach(sky_shader source_sky_stencil.vert source_sky_stencil.geom
            source_sky_stencil.frag source_sky_stencil_passthrough.vert source_sky_stencil_instanced.vert)
        set(sky_output "${sky_shader_directory}/${sky_shader}.spv")
        add_custom_command(OUTPUT "${sky_output}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${sky_shader_directory}"
            COMMAND "${GLSLANG_VALIDATOR}" -V --target-env vulkan1.1 "-I${sky_source_root}/shaders"
                "${sky_source_root}/shaders/${sky_shader}" -o "${sky_output}"
            DEPENDS "${sky_source_root}/shaders/${sky_shader}"
                "${sky_source_root}/shaders/world_depth_geometry.glsl"
                "${sky_source_root}/shaders/source_depth_storage.glsl"
            VERBATIM)
        list(APPEND sky_shader_outputs "${sky_output}")
    endforeach()
    add_custom_target(source_sky_stencil_shaders DEPENDS ${sky_shader_outputs})
    add_library(halo2_sky_stencil "${sky_source_root}/src/rendering/world/source_sky_stencil.cpp")
    target_compile_features(halo2_sky_stencil PUBLIC cxx_std_20)
    target_include_directories(halo2_sky_stencil PUBLIC "${sky_source_root}/include")
    target_link_libraries(halo2_sky_stencil PUBLIC Vulkan::Vulkan)
    add_dependencies(halo2_sky_stencil source_sky_stencil_shaders)
    if(MSVC)
        target_compile_options(halo2_sky_stencil PRIVATE /fp:strict)
    else()
        target_compile_options(halo2_sky_stencil PRIVATE -fno-fast-math -ffp-contract=off)
    endif()
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(halo2_sky_stencil PUBLIC
            -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(halo2_sky_stencil PUBLIC -fsanitize=address,undefined)
    endif()

    if(NOT BUILD_TESTING OR NOT HALO2_SKY_STENCIL_REFERENCE_TESTS)
        return()
    endif()
    include("${sky_source_root}/cmake/SourceSkyStencilReference.cmake")
    set(HALO2_SKY_REFERENCE_ROOT "${sky_source_root}" CACHE PATH "Root containing authenticated retained sky references")
    halo2_verify_source_sky_references("${HALO2_SKY_REFERENCE_ROOT}")
    add_executable(source_sky_stencil_tests "${sky_source_root}/tests/source_sky_stencil_tests.cpp")
    target_include_directories(source_sky_stencil_tests PRIVATE "${sky_source_root}/tests")
    target_link_libraries(source_sky_stencil_tests PRIVATE halo2_sky_stencil)
    target_compile_definitions(source_sky_stencil_tests PRIVATE
        HALO2_SOURCE_DIR="${HALO2_SKY_REFERENCE_ROOT}"
        HALO2_SKY_STENCIL_SHADER_DIR="${sky_shader_directory}")
    if(MSVC)
        target_compile_options(source_sky_stencil_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_sky_stencil_tests PRIVATE -fno-fast-math -ffp-contract=off)
    endif()
    add_test(NAME original_source_sky_stencil_cpu COMMAND source_sky_stencil_tests --cpu)
    # Runtime outputs belong to this build tree, never the frozen source proof.
    add_test(NAME original_source_sky_stencil_gpu COMMAND source_sky_stencil_tests --gpu
        "${CMAKE_CURRENT_BINARY_DIR}/reference-results/source-sky-stencil")
    set_tests_properties(original_source_sky_stencil_cpu original_source_sky_stencil_gpu
        PROPERTIES RUN_SERIAL TRUE LABELS "sky-stencil;original-reference")
    set_tests_properties(original_source_sky_stencil_cpu PROPERTIES LABELS "sky-stencil;original-reference;cpu")
    set_tests_properties(original_source_sky_stencil_gpu PROPERTIES LABELS "sky-stencil;original-reference;gpu")
endfunction()
