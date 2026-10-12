# Bounded retained original20 sky stencil diagnostic. This library is not linked
# to Renderer and registers no live sky writer or authored vertex producer.
function(halo2_add_source_sky_stencil sky_source_root)
    option(HALO2_SKY_STENCIL_REFERENCE_TESTS
        "Build retained original20 sky stencil CPU/GPU reference checks (requires captured artifacts)" OFF)
    set(sky_shader_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    set(sky_shader_outputs)
    foreach(sky_shader source_sky_stencil.vert source_sky_stencil.geom
            source_sky_stencil.frag source_sky_stencil_passthrough.vert)
        set(sky_output "${sky_shader_directory}/${sky_shader}.spv")
        add_custom_command(OUTPUT "${sky_output}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${sky_shader_directory}"
            COMMAND "${GLSLANG_VALIDATOR}" -V --target-env vulkan1.1
                "${sky_source_root}/shaders/${sky_shader}" -o "${sky_output}"
            DEPENDS "${sky_source_root}/shaders/${sky_shader}"
            VERBATIM)
        list(APPEND sky_shader_outputs "${sky_output}")
    endforeach()
    add_custom_target(source_sky_stencil_shaders DEPENDS ${sky_shader_outputs})
    add_library(halo2_sky_stencil "${sky_source_root}/src/source_sky_stencil.cpp")
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
    # These are the exact consumed captured geometry/uniform subsets and
    # original depth/stencil snapshots, pinned by the frozen prototype proof.
    # Missing or changed references fail explicitly; no substitute is generated.
    set(sky_reference_files
        "analysis/multiplayer-shading/source-sky-stencil-prototype/8097-retained-strip.fixture|1676|0f3d25f06c02e05239875f2e7a34c9112ef25c3ad37cd9790345ba8518808aa8"
        "analysis/multiplayer-shading/source-sky-stencil-prototype/8097-retained-gsout.fixture|14884|49b98d0c439178a0c2fc202d587059bb690bc2474901a1bda5b3e6dadc313158"
        "analysis/multiplayer-shading/original20-sky-stencil-analysis/8097-before-packed-d24s8.bin|1228800|98728be819022c19654a4fbf2cfc6bc6727e8ad061988caba36b6b940b2c189e"
        "analysis/multiplayer-shading/original20-sky-stencil-analysis/8097-after-packed-d24s8.bin|1228800|5248ac22b7f3ddfd2a6f3e5fec2d6b6d839b4482b343aac7b9209aacc390b856"
        "analysis/multiplayer-shading/source-sky-stencil-prototype/8102-retained-strip.fixture|1192|3cbeef14711c441094a4d1cb8ca3c50a1218358853b3bb15a696424f7807c627"
        "analysis/multiplayer-shading/source-sky-stencil-prototype/8102-retained-gsout.fixture|9784|f9cb89cfe6a55d498eb4bcf78896676192bcfe2ed7dc99d6f8d3cc551e4df826"
        "analysis/multiplayer-shading/original20-sky-stencil-analysis/8102-before-packed-d24s8.bin|1228800|5248ac22b7f3ddfd2a6f3e5fec2d6b6d839b4482b343aac7b9209aacc390b856"
        "analysis/multiplayer-shading/original20-sky-stencil-analysis/8102-after-packed-d24s8.bin|1228800|5248ac22b7f3ddfd2a6f3e5fec2d6b6d839b4482b343aac7b9209aacc390b856"
        "analysis/multiplayer-shading/source-sky-stencil-prototype/8110-retained-strip.fixture|340|c4c98723ca9901adf05e033a79883376a996d48f6b90d199ccd6fd269a4bc302"
        "analysis/multiplayer-shading/source-sky-stencil-prototype/8110-retained-gsout.fixture|604|69b07a6681996b2db5df12f0ceff299320dd0f64f692d5bc0d4e798ee9104dc7"
        "analysis/multiplayer-shading/original20-sky-stencil-analysis/8110-before-packed-d24s8.bin|1228800|5248ac22b7f3ddfd2a6f3e5fec2d6b6d839b4482b343aac7b9209aacc390b856"
        "analysis/multiplayer-shading/original20-sky-stencil-analysis/8110-after-packed-d24s8.bin|1228800|5248ac22b7f3ddfd2a6f3e5fec2d6b6d839b4482b343aac7b9209aacc390b856"
    )
    foreach(sky_reference IN LISTS sky_reference_files)
        string(REPLACE "|" ";" sky_reference_parts "${sky_reference}")
        list(GET sky_reference_parts 0 sky_relative)
        list(GET sky_reference_parts 1 sky_expected_size)
        list(GET sky_reference_parts 2 sky_expected_hash)
        set(sky_reference_path "${sky_source_root}/${sky_relative}")
        if(NOT EXISTS "${sky_reference_path}")
            message(FATAL_ERROR "Retained original20 sky reference missing: ${sky_relative}")
        endif()
        file(SIZE "${sky_reference_path}" sky_actual_size)
        file(SHA256 "${sky_reference_path}" sky_actual_hash)
        if(NOT sky_actual_size EQUAL sky_expected_size OR
                NOT sky_actual_hash STREQUAL sky_expected_hash)
            message(FATAL_ERROR "Retained original20 sky reference identity changed: ${sky_relative}")
        endif()
    endforeach()
    add_executable(source_sky_stencil_tests "${sky_source_root}/tests/source_sky_stencil_tests.cpp")
    target_include_directories(source_sky_stencil_tests PRIVATE "${sky_source_root}/tests")
    target_link_libraries(source_sky_stencil_tests PRIVATE halo2_sky_stencil)
    target_compile_definitions(source_sky_stencil_tests PRIVATE
        HALO2_SOURCE_DIR="${sky_source_root}"
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
