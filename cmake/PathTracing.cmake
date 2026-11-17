# Optional portable BVH path tracing. Retail source shading remains independent.
add_library(halo2_path_tracing STATIC
    src/rendering/path_tracing/scene.cpp
    src/rendering/path_tracing/material_alias.cpp
    src/rendering/path_tracing/emitter_sampling.cpp
    src/rendering/path_tracing/geometry.cpp
    src/rendering/path_tracing/color_guide.cpp
    src/rendering/path_tracing/render.cpp)
target_include_directories(halo2_path_tracing PUBLIC ${PROJECT_SOURCE_DIR}/include PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(halo2_path_tracing PUBLIC halo2_cache Vulkan::Vulkan)
target_compile_definitions(halo2_path_tracing PRIVATE HALO2_PATH_TRACING_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
target_compile_options(halo2_path_tracing PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wno-missing-field-initializers;-ffp-contract=off>)
set_target_properties(halo2_path_tracing PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_sources(halo2_systems PRIVATE src/rendering/path_tracing/source_lights.cpp)
foreach(stage IN ITEMS vert frag)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/path_tracing/world.${stage}.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders/path_tracing
        COMMAND ${GLSLANG_VALIDATOR} -V -I${PROJECT_SOURCE_DIR}/shaders
            ${PROJECT_SOURCE_DIR}/shaders/path_tracing/world.${stage}
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/path_tracing/world.${stage}.spv
        DEPENDS shaders/path_tracing/world.${stage} shaders/path_tracing/scene.glsl shaders/path_tracing/emission.glsl
            shaders/path_tracing/lighting_filter.glsl shaders/path_tracing/surface.glsl
            shaders/path_tracing/color_guide.glsl shaders/path_tracing/surface_ray.glsl shaders/path_tracing/emitter_sampling.glsl
            shaders/path_tracing/source_indirect.glsl shaders/path_tracing/source_ambient.glsl
            shaders/source_alpha_test.glsl shaders/source_stipple.glsl shaders/world_depth_fragment.glsl VERBATIM)
endforeach()
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/path_tracing/lighting.frag.spv
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders/path_tracing
    COMMAND ${GLSLANG_VALIDATOR} -V -I${PROJECT_SOURCE_DIR}/shaders
        -DH2_PATH_TRACING_LIGHTING_WRITE=1 ${PROJECT_SOURCE_DIR}/shaders/path_tracing/world.frag
        -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/path_tracing/lighting.frag.spv
    DEPENDS shaders/path_tracing/world.frag shaders/path_tracing/scene.glsl shaders/path_tracing/emission.glsl
        shaders/path_tracing/lighting_filter.glsl shaders/path_tracing/surface.glsl shaders/source_alpha_test.glsl
        shaders/path_tracing/color_guide.glsl shaders/path_tracing/surface_ray.glsl shaders/path_tracing/emitter_sampling.glsl
        shaders/path_tracing/source_indirect.glsl shaders/path_tracing/source_ambient.glsl
        shaders/source_stipple.glsl shaders/world_depth_fragment.glsl VERBATIM)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/path_tracing/depth.frag.spv
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders/path_tracing
    COMMAND ${GLSLANG_VALIDATOR} -V -I${PROJECT_SOURCE_DIR}/shaders
        ${PROJECT_SOURCE_DIR}/shaders/path_tracing/depth.frag
        -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/path_tracing/depth.frag.spv
    DEPENDS shaders/path_tracing/depth.frag shaders/path_tracing/surface.glsl shaders/source_alpha_test.glsl
        shaders/source_stipple.glsl shaders/world_depth_fragment.glsl VERBATIM)
add_custom_target(path_tracing_shaders DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/path_tracing/depth.frag.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/path_tracing/lighting.frag.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/path_tracing/world.vert.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/path_tracing/world.frag.spv)
add_dependencies(halo2_path_tracing path_tracing_shaders)
target_link_libraries(halo2_renderer PUBLIC halo2_path_tracing)
if(BUILD_TESTING)
    add_executable(path_tracing_material_alias_tests tests/path_tracing/material_alias_tests.cpp)
    target_link_libraries(path_tracing_material_alias_tests PRIVATE halo2_path_tracing nlohmann_json::nlohmann_json)
    add_test(NAME path_tracing_material_alias COMMAND path_tracing_material_alias_tests
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-82114d7d2e034d48ba762109935f9ca3/halo2/h2_maps_win64_dx11/00a_introduction.map"
        "${CMAKE_CURRENT_BINARY_DIR}/path-tracing-material-alias.json")
    set_tests_properties(path_tracing_material_alias PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(path_tracing_lighting_storage_tests tests/path_tracing/lighting_storage_tests.cpp)
    target_include_directories(path_tracing_lighting_storage_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
    add_test(NAME path_tracing_lighting_storage COMMAND path_tracing_lighting_storage_tests)
    add_executable(path_tracing_vanilla_surface_tests tests/path_tracing/vanilla_surface_tests.cpp)
    target_include_directories(path_tracing_vanilla_surface_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_compile_options(path_tracing_vanilla_surface_tests PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-ffp-contract=off>)
    add_test(NAME path_tracing_vanilla_surface COMMAND path_tracing_vanilla_surface_tests)
    add_executable(path_tracing_vanilla_material_source_tests tests/path_tracing/vanilla_material_source_tests.cpp)
    target_link_libraries(path_tracing_vanilla_material_source_tests PRIVATE halo2_path_tracing nlohmann_json::nlohmann_json)
    add_test(NAME path_tracing_vanilla_opening_materials COMMAND path_tracing_vanilla_material_source_tests
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-82114d7d2e034d48ba762109935f9ca3/halo2/h2_maps_win64_dx11/00a_introduction.map"
        "${CMAKE_CURRENT_BINARY_DIR}/path-tracing-vanilla-opening-materials.json")
    set_tests_properties(path_tracing_vanilla_opening_materials PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(path_tracing_dynamic_detail_source_tests tests/path_tracing/dynamic_detail_source_tests.cpp)
    target_link_libraries(path_tracing_dynamic_detail_source_tests PRIVATE
        halo2_path_tracing halo2_vertex_program_reference nlohmann_json::nlohmann_json)
    target_compile_options(path_tracing_dynamic_detail_source_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-ffp-contract=off>)
    add_test(NAME path_tracing_source_dynamic_detail COMMAND path_tracing_dynamic_detail_source_tests
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-82114d7d2e034d48ba762109935f9ca3/halo2/h2_maps_win64_dx11/00a_introduction.map"
        "${CMAKE_CURRENT_BINARY_DIR}/path-tracing-source-dynamic-detail.json")
    set_tests_properties(path_tracing_source_dynamic_detail PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(path_tracing_source_ambient_tests tests/path_tracing/source_ambient_tests.cpp)
    target_link_libraries(path_tracing_source_ambient_tests PRIVATE
        halo2_path_tracing halo2_vertex_program_reference nlohmann_json::nlohmann_json)
    target_compile_options(path_tracing_source_ambient_tests PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-ffp-contract=off>)
    add_test(NAME path_tracing_source_ambient COMMAND path_tracing_source_ambient_tests
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-82114d7d2e034d48ba762109935f9ca3/halo2/h2_maps_win64_dx11/00a_introduction.map"
        "${CMAKE_CURRENT_BINARY_DIR}/path-tracing-source-ambient.json")
    set_tests_properties(path_tracing_source_ambient PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(path_tracing_color_guide_tests tests/path_tracing/color_guide_tests.cpp)
    target_link_libraries(path_tracing_color_guide_tests PRIVATE halo2_path_tracing nlohmann_json::nlohmann_json)
    target_compile_options(path_tracing_color_guide_tests PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-ffp-contract=off>)
    add_test(NAME path_tracing_source_color_guide COMMAND path_tracing_color_guide_tests
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/coagulation.map"
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/lockout.map"
        "${CMAKE_CURRENT_BINARY_DIR}/path-tracing-source-color-guide.json")
    set_tests_properties(path_tracing_source_color_guide PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_executable(path_tracing_sky_lighting_tests tests/path_tracing/sky_lighting_tests.cpp)
    target_link_libraries(path_tracing_sky_lighting_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME path_tracing_selected_sky COMMAND path_tracing_sky_lighting_tests
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/lockout.map"
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/coagulation.map"
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/lockout.map"
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/coagulation.map"
        "${CMAKE_CURRENT_BINARY_DIR}/path-tracing-selected-sky.json")
    set_tests_properties(path_tracing_selected_sky PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 10)
    add_executable(path_tracing_emission_source_tests tests/path_tracing/emission_source_tests.cpp)
    target_link_libraries(path_tracing_emission_source_tests PRIVATE halo2_path_tracing nlohmann_json::nlohmann_json)
    add_test(NAME path_tracing_source_emission COMMAND path_tracing_emission_source_tests
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/lockout.map"
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/00a_introduction.map"
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/lockout.map"
        "${CMAKE_CURRENT_BINARY_DIR}/path-tracing-source-emission.json")
    set_tests_properties(path_tracing_source_emission PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(path_tracing_surface_source_tests tests/path_tracing/surface_source_tests.cpp)
    target_link_libraries(path_tracing_surface_source_tests PRIVATE halo2_path_tracing nlohmann_json::nlohmann_json)
    target_compile_options(path_tracing_surface_source_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-ffp-contract=off>
        $<$<CXX_COMPILER_ID:MSVC>:/fp:strict>)
    add_test(NAME path_tracing_source_surface COMMAND path_tracing_surface_source_tests
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/coagulation.map"
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/coagulation.map"
        "${CMAKE_CURRENT_BINARY_DIR}/path-tracing-source-surface.json")
    set_tests_properties(path_tracing_source_surface PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_executable(path_tracing_opaque_surface_source_tests tests/path_tracing/opaque_surface_source_tests.cpp)
    target_link_libraries(path_tracing_opaque_surface_source_tests PRIVATE halo2_path_tracing nlohmann_json::nlohmann_json)
    target_compile_options(path_tracing_opaque_surface_source_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-ffp-contract=off>
        $<$<CXX_COMPILER_ID:MSVC>:/fp:strict>)
    add_test(NAME path_tracing_source_opaque_surface COMMAND path_tracing_opaque_surface_source_tests
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/coagulation.map"
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/coagulation.map"
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/lockout.map"
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/lockout.map"
        "${CMAKE_CURRENT_BINARY_DIR}/path-tracing-source-opaque-surface.json")
    set_tests_properties(path_tracing_source_opaque_surface PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_executable(path_tracing_panel_surface_source_tests tests/path_tracing/panel_surface_source_tests.cpp)
    target_link_libraries(path_tracing_panel_surface_source_tests PRIVATE halo2_path_tracing nlohmann_json::nlohmann_json)
    target_compile_options(path_tracing_panel_surface_source_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-ffp-contract=off>
        $<$<CXX_COMPILER_ID:MSVC>:/fp:strict>)
    add_test(NAME path_tracing_source_panel_surface COMMAND path_tracing_panel_surface_source_tests
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/lockout.map"
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/lockout.map"
        "${CMAKE_CURRENT_BINARY_DIR}/path-tracing-source-panel-surface.json")
    set_tests_properties(path_tracing_source_panel_surface PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_executable(path_tracing_scene_tests tests/path_tracing/scene_tests.cpp)
    target_link_libraries(path_tracing_scene_tests PRIVATE halo2_path_tracing)
    add_test(NAME path_tracing_scene COMMAND path_tracing_scene_tests)
    add_executable(path_tracing_geometry_tests tests/path_tracing/geometry_tests.cpp)
    target_link_libraries(path_tracing_geometry_tests PRIVATE halo2_path_tracing)
    add_test(NAME path_tracing_posed_geometry COMMAND path_tracing_geometry_tests)
    add_executable(path_tracing_gpu_tests tests/path_tracing/gpu_tests.cpp)
    target_link_libraries(path_tracing_gpu_tests PRIVATE halo2_path_tracing nlohmann_json::nlohmann_json)
    add_test(NAME path_tracing_gpu COMMAND path_tracing_gpu_tests ${CMAKE_CURRENT_BINARY_DIR}/shaders)
    # Includes cold driver compilation of the visibility, HDR-write and
    # resolve roles before the bounded 64x64 material/transport fixtures.
    # The portable intersection shader can spend over 45s compiling all cold
    # driver pipeline roles; retain a finite budget for that first execution.
    set_tests_properties(path_tracing_gpu PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
    option(HALO2_PATH_TRACING_NATIVE_DENOISE_TESTS
        "Run the2560x1388 no-upscaler Path Tracing noise regression" OFF)
    set(path_tracing_gpu_test_names path_tracing_gpu)
    add_test(NAME path_tracing_material_alias_gpu COMMAND path_tracing_gpu_tests
        ${CMAKE_CURRENT_BINARY_DIR}/shaders --material-alias)
    set_tests_properties(path_tracing_material_alias_gpu PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
    list(APPEND path_tracing_gpu_test_names path_tracing_material_alias_gpu)
    add_test(NAME path_tracing_material_alias_history COMMAND path_tracing_gpu_tests
        ${CMAKE_CURRENT_BINARY_DIR}/shaders --material-alias-history
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-82114d7d2e034d48ba762109935f9ca3/halo2/h2_maps_win64_dx11/00a_introduction.map")
    set_tests_properties(path_tracing_material_alias_history PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
    list(APPEND path_tracing_gpu_test_names path_tracing_material_alias_history)
    add_test(NAME path_tracing_foreground_reflection_turn COMMAND path_tracing_gpu_tests
        ${CMAKE_CURRENT_BINARY_DIR}/shaders --foreground-reflection-turn)
    set_tests_properties(path_tracing_foreground_reflection_turn PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
    list(APPEND path_tracing_gpu_test_names path_tracing_foreground_reflection_turn)
    add_test(NAME path_tracing_reflected_object_history COMMAND path_tracing_gpu_tests
        ${CMAKE_CURRENT_BINARY_DIR}/shaders --reflected-object-history)
    set_tests_properties(path_tracing_reflected_object_history PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
    list(APPEND path_tracing_gpu_test_names path_tracing_reflected_object_history)
    add_test(NAME path_tracing_unrelated_reflection_motion COMMAND path_tracing_gpu_tests
        ${CMAKE_CURRENT_BINARY_DIR}/shaders --unrelated-reflection-motion)
    set_tests_properties(path_tracing_unrelated_reflection_motion PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
    list(APPEND path_tracing_gpu_test_names path_tracing_unrelated_reflection_motion)
    add_test(NAME path_tracing_source_indirect COMMAND path_tracing_gpu_tests
        ${CMAKE_CURRENT_BINARY_DIR}/shaders --source-indirect)
    set_tests_properties(path_tracing_source_indirect PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
    list(APPEND path_tracing_gpu_test_names path_tracing_source_indirect)
    add_test(NAME path_tracing_opaque_detail COMMAND path_tracing_gpu_tests
        ${CMAKE_CURRENT_BINARY_DIR}/shaders --opaque-detail)
    set_tests_properties(path_tracing_opaque_detail PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
    list(APPEND path_tracing_gpu_test_names path_tracing_opaque_detail)
    add_test(NAME path_tracing_vanilla_surface_gpu COMMAND path_tracing_gpu_tests
        ${CMAKE_CURRENT_BINARY_DIR}/shaders --vanilla-surface)
    set_tests_properties(path_tracing_vanilla_surface_gpu PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
    list(APPEND path_tracing_gpu_test_names path_tracing_vanilla_surface_gpu)
    add_test(NAME path_tracing_panel_detail COMMAND path_tracing_gpu_tests
        ${CMAKE_CURRENT_BINARY_DIR}/shaders --panel-detail
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/lockout.map"
        "${CMAKE_CURRENT_BINARY_DIR}/path-tracing-panel-detail.json")
    set_tests_properties(path_tracing_panel_detail PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
    list(APPEND path_tracing_gpu_test_names path_tracing_panel_detail)
    add_test(NAME path_tracing_capital_ship COMMAND path_tracing_gpu_tests
        ${CMAKE_CURRENT_BINARY_DIR}/shaders --capital-ship
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-82114d7d2e034d48ba762109935f9ca3/halo2/h2_maps_win64_dx11/00a_introduction.map"
        "${CMAKE_CURRENT_BINARY_DIR}/path-tracing-capital-ship.json")
    set_tests_properties(path_tracing_capital_ship PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
    list(APPEND path_tracing_gpu_test_names path_tracing_capital_ship)
    add_test(NAME path_tracing_source_primary COMMAND path_tracing_gpu_tests
        ${CMAKE_CURRENT_BINARY_DIR}/shaders --source-primary)
    set_tests_properties(path_tracing_source_primary PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
    list(APPEND path_tracing_gpu_test_names path_tracing_source_primary)
    if(HALO2_PATH_TRACING_NATIVE_DENOISE_TESTS)
        add_test(NAME path_tracing_grazing_normal COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --grazing-normal)
        set_tests_properties(path_tracing_grazing_normal PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
        add_test(NAME path_tracing_grazing_secondary COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --grazing-secondary)
        set_tests_properties(path_tracing_grazing_secondary PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
        add_test(NAME path_tracing_jitter_reprojection COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --jitter-reprojection)
        add_test(NAME path_tracing_camera_reprojection COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --camera-reprojection)
        set_tests_properties(path_tracing_jitter_reprojection path_tracing_camera_reprojection PROPERTIES
            RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
        add_test(NAME path_tracing_foreground_reprojection COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --foreground-reprojection)
        set_tests_properties(path_tracing_foreground_reprojection PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
        add_test(NAME path_tracing_native_denoise COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --native-noise)
        add_test(NAME path_tracing_native_fence_denoise COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --native-fence-noise)
        add_test(NAME path_tracing_shadow_disocclusion COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --shadow-disocclusion)
        add_test(NAME path_tracing_multiple_light_shadow_disocclusion COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --multiple-light-shadow-disocclusion)
        add_test(NAME path_tracing_reflection_history COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --reflection-history)
        add_test(NAME path_tracing_reflection_tint_history COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --reflection-tint-history)
        set_tests_properties(path_tracing_reflection_tint_history PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
        list(APPEND path_tracing_gpu_test_names path_tracing_reflection_tint_history)
        add_test(NAME path_tracing_animated_light_denoise COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --animated-light-noise)
        add_test(NAME path_tracing_moving_light_sampling COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --moving-light-sampling)
        add_test(NAME path_tracing_alpha_shadow_history COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --alpha-shadow-history)
        set_tests_properties(path_tracing_alpha_shadow_history PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
        list(APPEND path_tracing_gpu_test_names path_tracing_alpha_shadow_history)
        set_tests_properties(path_tracing_moving_light_sampling PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
        list(APPEND path_tracing_gpu_test_names path_tracing_moving_light_sampling)
        add_test(NAME path_tracing_4k_fence_denoise COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --native-fence-noise 3840 2160)
        set_tests_properties(path_tracing_4k_fence_denoise PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 120)
        list(APPEND path_tracing_gpu_test_names path_tracing_4k_fence_denoise)
        add_test(NAME path_tracing_denoiser_negative_controls COMMAND ${Python3_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tools/validate_path_tracing_denoiser_regressions.py
            --gpu $<TARGET_FILE:path_tracing_gpu_tests> --shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders
            --compiler ${GLSLANG_VALIDATOR}
            --report ${CMAKE_CURRENT_BINARY_DIR}/path-tracing-denoiser-negative-controls.json)
        # Seven private shader variants may each require a cold driver compile.
        set_tests_properties(path_tracing_denoiser_negative_controls PROPERTIES RUN_SERIAL TRUE TIMEOUT 300)
        list(APPEND path_tracing_gpu_test_names path_tracing_denoiser_negative_controls)
        add_test(NAME path_tracing_native_shadow_disocclusion COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --native-shadow-disocclusion)
        add_test(NAME path_tracing_unrelated_motion_denoise COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --unrelated-motion-noise)
        add_test(NAME path_tracing_smooth_terminator COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --smooth-terminator
            "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/coagulation.map")
        set_tests_properties(path_tracing_smooth_terminator PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 120)
        add_test(NAME path_tracing_smooth_terminator_native COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --smooth-terminator
            "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/coagulation.map" 2560 1378)
        set_tests_properties(path_tracing_smooth_terminator_native PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 120)
        add_test(NAME path_tracing_actual_lockout_colors COMMAND path_tracing_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/shaders --actual-lockout-colors
            "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/lockout.map"
            "${CMAKE_CURRENT_BINARY_DIR}/path-tracing-actual-lockout-colors.json")
        set_tests_properties(path_tracing_actual_lockout_colors PROPERTIES RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 120)
        # Ideal control: one raw, three stationary filtered and four with a
        # tiny camera translation. Fenced WORLD control: one raw and sixteen
        # stationary filtered, with no intermediate capture/host barriers.
        set_tests_properties(path_tracing_native_denoise path_tracing_native_fence_denoise
            path_tracing_shadow_disocclusion path_tracing_multiple_light_shadow_disocclusion path_tracing_reflection_history path_tracing_animated_light_denoise path_tracing_native_shadow_disocclusion path_tracing_unrelated_motion_denoise PROPERTIES
            RUN_SERIAL TRUE SKIP_RETURN_CODE 77 TIMEOUT 90)
        list(APPEND path_tracing_gpu_test_names path_tracing_native_denoise path_tracing_native_fence_denoise
            path_tracing_shadow_disocclusion path_tracing_multiple_light_shadow_disocclusion path_tracing_reflection_history path_tracing_animated_light_denoise path_tracing_native_shadow_disocclusion path_tracing_unrelated_motion_denoise
            path_tracing_smooth_terminator path_tracing_smooth_terminator_native path_tracing_actual_lockout_colors
            path_tracing_grazing_normal path_tracing_grazing_secondary
            path_tracing_jitter_reprojection path_tracing_camera_reprojection path_tracing_foreground_reprojection)
    endif()
    if(EXISTS "${PROJECT_SOURCE_DIR}/.tools/validation/usr/lib/libVkLayer_khronos_validation.so")
        set_property(TEST ${path_tracing_gpu_test_names} APPEND PROPERTY ENVIRONMENT
            "VK_LAYER_PATH=${PROJECT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d"
            "LD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/.tools/validation/usr/lib:$ENV{LD_LIBRARY_PATH}")
    endif()
    add_executable(path_tracing_unlit_uv_tests tests/path_tracing/unlit_uv_tests.cpp)
    target_link_libraries(path_tracing_unlit_uv_tests PRIVATE
        halo2_cache halo2_vertex_program_reference nlohmann_json::nlohmann_json)
    target_compile_options(path_tracing_unlit_uv_tests PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-ffp-contract=off>
        $<$<CXX_COMPILER_ID:MSVC>:/fp:strict>)
    add_test(NAME path_tracing_unlit_uv COMMAND path_tracing_unlit_uv_tests
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/disc/maps/lockout.map"
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/lockout.map"
        "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/coagulation.map"
        "${CMAKE_CURRENT_BINARY_DIR}/path-tracing-unlit-uv.json")
    set_tests_properties(path_tracing_unlit_uv PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    # run_tests.py --retain-report path-tracing-unlit-uv.json keeps this small
    # source-program receipt before the owned build is removed.
    option(HALO2_PATH_TRACING_SELF_SHADOW_TESTS
        "Probe actual prepared Coagulation BSP source-normal direct-sun self-hits" OFF)
    if(HALO2_PATH_TRACING_SELF_SHADOW_TESTS)
        add_executable(path_tracing_self_shadow_tests tests/path_tracing/self_shadow_tests.cpp)
        target_link_libraries(path_tracing_self_shadow_tests PRIVATE halo2_path_tracing nlohmann_json::nlohmann_json)
        target_compile_options(path_tracing_self_shadow_tests PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-ffp-contract=off>
            $<$<CXX_COMPILER_ID:MSVC>:/fp:strict>)
        add_test(NAME path_tracing_source_self_shadow COMMAND path_tracing_self_shadow_tests
            "${HALO2_TEST_ASSET_SOURCE_ROOT}/assets/imported/mcc-sky-water-tu15-20261025-v7/halo2/h2_maps_win64_dx11/coagulation.map"
            "${CMAKE_CURRENT_BINARY_DIR}/path-tracing-source-self-shadow.json")
        set_tests_properties(path_tracing_source_self_shadow PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    endif()
endif()
