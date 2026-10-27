# Explicit production inventory. Reference/probe shaders in an incremental
# test build are never copied into a launcher runtime.
set(native_renderer_fixed_shaders
    bloom.vert.spv crt_oled.frag.spv display.frag.spv fog.vert.spv
    light_volume.vert.spv menu_world.geom.spv model_skin.comp.spv
    overlay.frag.spv overlay.vert.spv particle.geom.spv particle.vert.spv
    particle_directional.vert.spv particle_distortion.vert.spv
    particle_model.geom.spv particle_model.vert.spv particle_perpendicular.vert.spv
    patchy_noise.vert.spv pstencil/lightmap.frag.spv pstencil/lightmap.vert.spv
    path_tracing/world.vert.spv path_tracing/world.frag.spv path_tracing/depth.frag.spv
    path_tracing/lighting.frag.spv
    pstencil/volume.frag.spv pstencil/volume.vert.spv
    source_contrail.geom.spv source_contrail.vert.spv
    source_decal.geom.spv source_decal.vert.spv
    source_depth_color_writeback.comp.spv source_lens_draw.vert.spv
    source_lens_query.frag.spv source_lens_query.vert.spv
    source_shadow_filter.frag.spv source_shadow_filter.vert.spv
    source_weather_depth_alias.comp.spv source_weather_line.frag.spv
    source_weather_line.geom.spv source_weather_line.vert.spv
    source_weather_plate.frag.spv source_weather_plate.vert.spv
    source_weather_point.frag.spv source_weather_point.vert.spv
    ui.frag.spv ui.vert.spv ui_frame.frag.spv upscale_motion.frag.spv
    upscale_motion.vert.spv upscale_prepare.comp.spv
    vista_loading.frag.spv vista_loading.vert.spv water_copy.frag.spv
    world.vert.spv world_prt.vert.spv world_emblem.geom.spv)
set(native_renderer_production_shaders ${native_renderer_fixed_shaders})
foreach(shader IN LISTS NATIVE_WORLD_KERNEL_SPIRV_FILES NATIVE_FRAGMENT_SPIRV)
    file(RELATIVE_PATH relative "${CMAKE_CURRENT_BINARY_DIR}/shaders" "${shader}")
    list(APPEND native_renderer_production_shaders "${relative}")
endforeach()
list(REMOVE_DUPLICATES native_renderer_production_shaders)
list(SORT native_renderer_production_shaders)
string(REPLACE ";" "\n" native_renderer_production_shader_lines "${native_renderer_production_shaders}")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/production-shaders.txt"
    CONTENT "${native_renderer_production_shader_lines}\n")
