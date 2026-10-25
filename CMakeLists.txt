cmake_minimum_required(VERSION 3.24)
include(cmake/macos/Defaults.cmake)
project(Halo2Native VERSION 0.1.0 LANGUAGES CXX)

# Exported runtimes may outlive a disposable build. Their complete compiled
# shader bundle must be retained at this explicitly selected runtime path.
set(HALO2_RUNTIME_SHADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders" CACHE PATH
    "Runtime SPIR-V directory; generation remains in the private build tree")
# Asset decoding and scene preparation also run on the startup path. Keep
# fresh single-config builds optimized; explicit configurations are retained.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build configuration" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(HALO2_SANITIZERS "Enable address/undefined sanitizers" OFF)
if(APPLE)
    include(cmake/macos/Platform.cmake)
else()
    find_package(Vulkan REQUIRED)
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
if(APPLE)
    option(HALO2_MACOS_BUILD_SDL "Build pinned SDL for the requested Mac architectures" ON)
endif()
if(APPLE AND HALO2_MACOS_BUILD_SDL)
    include(cmake/macos/SDL.cmake)
    halo2_macos_sdl_dependency()
else()
    find_package(SDL2 REQUIRED CONFIG)
endif()
include(cmake/TestStorage.cmake)
option(HALO2_MACOS_BRINGUP_ONLY "Build just the macOS SDL/MoltenVK presentation and capability check" OFF)
if(HALO2_MACOS_BRINGUP_ONLY)
    if(NOT APPLE)
        message(FATAL_ERROR "HALO2_MACOS_BRINGUP_ONLY requires macOS")
    endif()
    include(cmake/macos/Bringup.cmake)
    return()
endif()
include(cmake/Glm.cmake)
find_package(Threads REQUIRED)
find_package(CURL 7.86 REQUIRED)
if(APPLE)
    option(HALO2_MACOS_BUILD_OPENSSL "Build pinned OpenSSL for the requested Mac architectures" ON)
endif()
if(APPLE AND HALO2_MACOS_BUILD_OPENSSL)
    include(cmake/macos/OpenSSL.cmake)
    halo2_macos_openssl_dependency()
else()
    find_package(OpenSSL REQUIRED COMPONENTS Crypto)
endif()
find_package(nlohmann_json 3.10 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PkgConfig REQUIRED)
if(APPLE)
    option(HALO2_MACOS_BUILD_FFMPEG "Build pinned playback libraries for the requested Mac architectures" ON)
endif()
if(APPLE AND HALO2_MACOS_BUILD_FFMPEG)
    include(cmake/macos/FFmpeg.cmake)
    halo2_macos_ffmpeg_dependency()
else()
    pkg_check_modules(AVCODEC REQUIRED IMPORTED_TARGET libavcodec libavformat libavutil libswresample libswscale)
endif()
if(APPLE)
    option(HALO2_MACOS_BUILD_OPUS "Build pinned Opus for the requested Mac architectures" ON)
endif()
if(APPLE AND HALO2_MACOS_BUILD_OPUS)
    include(cmake/macos/Opus.cmake)
    halo2_macos_opus_dependency()
else()
    pkg_check_modules(OPUS REQUIRED IMPORTED_TARGET opus)
endif()
option(HALO2_BUILD_MCC_LOADING_AUDIO "Build the genuine MCC loading-track setup decoder" ON)
option(HALO2_BUILD_MCC_LOADING_AUDIO_AUDIT "Compatibility alias enabling the MCC loading-track decoder" OFF)
if(HALO2_BUILD_MCC_LOADING_AUDIO OR HALO2_BUILD_MCC_LOADING_AUDIO_AUDIT)
    if(APPLE)
        option(HALO2_MACOS_BUILD_VORBIS "Build pinned Ogg/Vorbis for the requested Mac architectures" ON)
    endif()
    if(APPLE AND HALO2_MACOS_BUILD_VORBIS)
        include(cmake/macos/Vorbis.cmake)
        halo2_macos_vorbis_dependency()
    else()
        pkg_check_modules(VORBISFILE REQUIRED IMPORTED_TARGET vorbis ogg)
    endif()
    add_executable(halo2_mcc_loading_audio tools/convert_mcc_loading_audio.cpp
        external/ww2ogg/src/wwriff.cpp external/ww2ogg/src/codebook.cpp external/ww2ogg/src/crc.c)
    set_source_files_properties(external/ww2ogg/src/crc.c PROPERTIES LANGUAGE CXX)
    target_include_directories(halo2_mcc_loading_audio PRIVATE external/ww2ogg/src)
    target_link_libraries(halo2_mcc_loading_audio PRIVATE PkgConfig::VORBISFILE)
endif()
# The owned SDK generates its headers during the build. Its real Xbox codec
# enum and 64-sample decoding contract are checked by tests/macos/ffmpeg.
# Externally supplied headers must pass the existing configure-time check.
include(CheckCXXSourceCompiles)
if(NOT (APPLE AND HALO2_MACOS_BUILD_FFMPEG))
    set(CMAKE_REQUIRED_INCLUDES ${AVCODEC_INCLUDE_DIRS})
    check_cxx_source_compiles("#include <cstdint>\n#include <libavcodec/codec_id.h>\nint main() { return int(AV_CODEC_ID_ADPCM_IMA_XBOX); }" HALO2_XBOX_ADPCM)
    unset(CMAKE_REQUIRED_INCLUDES)
    if(NOT HALO2_XBOX_ADPCM)
        message(FATAL_ERROR "FFmpeg headers must provide AV_CODEC_ID_ADPCM_IMA_XBOX; generic IMA WAV is not compatible with Xbox audio.")
    endif()
endif()
find_program(GLSLANG_VALIDATOR glslangValidator REQUIRED)
include(cmake/SnapshotStorage.cmake)

# Private implementation headers use subsystem paths relative to src/.
# Public interfaces continue to be exported through include/halo2/.
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")

add_library(halo2_cache src/assets/cache.cpp src/rendering/material.cpp src/rendering/shader_animation.cpp src/rendering/bitmap_animation.cpp src/assets/appearance.cpp src/rendering/models/model_environment.cpp src/rendering/world/sky.cpp src/rendering/world/lightmap.cpp src/rendering/renderer/render_program.cpp src/rendering/renderer/render_view.cpp src/rendering/atmosphere.cpp src/rendering/fog_composite.cpp src/rendering/patchy_fog.cpp src/ui/menu_bloom.cpp src/ui/menu_atmosphere.cpp src/rendering/vertex_program.cpp src/rendering/texture.cpp src/rendering/texture_state.cpp src/rendering/cubemap.cpp src/rendering/bitmap_mips.cpp src/assets/content.cpp src/rendering/models/model.cpp src/rendering/models/model_pose.cpp src/core/transform.cpp src/ui/frontend_content.cpp src/ui/frontend_font.cpp src/ui/frontend_material.cpp)
target_sources(halo2_cache PRIVATE src/assets/tag_owner_index.cpp)
target_sources(halo2_cache PRIVATE src/ui/controller_prompt_artwork.cpp)
add_library(halo2_game_variant src/gameplay/multiplayer/source_game_variant.cpp)
target_include_directories(halo2_game_variant PUBLIC include)
target_compile_features(halo2_game_variant PUBLIC cxx_std_20)
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(halo2_game_variant PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_game_variant PUBLIC -fsanitize=address,undefined)
endif()
target_link_libraries(halo2_cache PRIVATE halo2_game_variant)
add_library(halo2_bomb_defusal src/gameplay/multiplayer/bomb_defusal.cpp)
target_include_directories(halo2_bomb_defusal PUBLIC include)
target_link_libraries(halo2_bomb_defusal PUBLIC halo2_game_variant)
target_link_libraries(halo2_cache PUBLIC halo2_bomb_defusal)
target_sources(halo2_cache PRIVATE src/platform/tu15_bump_palette.cpp)
add_library(halo2_renderer_palette src/rendering/renderer/renderer_palette.cpp)
target_sources(halo2_renderer_palette PRIVATE src/rendering/renderer/renderer_constant_mode.cpp)
target_sources(halo2_renderer_palette PRIVATE src/rendering/renderer/renderer_depth_mode.cpp)
target_sources(halo2_renderer_palette PRIVATE src/rendering/renderer/renderer_particle_program.cpp)
target_sources(halo2_renderer_palette PRIVATE src/rendering/renderer/renderer_particle_view.cpp)
target_sources(halo2_renderer_palette PRIVATE src/rendering/renderer/renderer_output_state.cpp)
target_sources(halo2_renderer_palette PRIVATE src/rendering/renderer/renderer_texture_coordinates.cpp)
target_sources(halo2_renderer_palette PRIVATE src/rendering/renderer/renderer_targets.cpp)
target_include_directories(halo2_renderer_palette PUBLIC include)
target_link_libraries(halo2_renderer_palette PUBLIC halo2_cache halo2_renderer_device_lease)
target_sources(halo2_cache PRIVATE src/gameplay/multiplayer/custom_slayer.cpp)
target_link_libraries(halo2_cache PUBLIC nlohmann_json::nlohmann_json OpenSSL::Crypto)
target_compile_definitions(halo2_cache PRIVATE HALO2_VARIANT_TAG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/tags/variants")
target_sources(halo2_cache PRIVATE src/rendering/effects/particle_movement.cpp)
target_sources(halo2_cache PRIVATE src/rendering/effects/particle_collision.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/effects/particle_movement.cpp src/rendering/effects/particle_collision.cpp src/gameplay/weapons/source_grenade_scene.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/effects/particle_movement.cpp src/rendering/effects/particle_collision.cpp src/gameplay/weapons/source_grenade_scene.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
endif()
target_sources(halo2_cache PRIVATE src/rendering/world/world_material.cpp src/rendering/world/bsp_atmosphere.cpp src/rendering/world/bsp_patchy_fog.cpp src/rendering/world/water.cpp src/gameplay/objects/scenery_lighting.cpp src/gameplay/objects/object_lighting.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_shader_texture.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_vertex_extern.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_shader_rgb.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_shader_state.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_shader_fog.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_scene_copy.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/source_scene_copy.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/source_scene_copy.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(MSVC)
    set_source_files_properties(src/rendering/source_shader_fog.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/source_shader_fog.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_cache PRIVATE src/rendering/source_fog_clear.cpp src/rendering/source_view_clear.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/source_fog_clear.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/source_fog_clear.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_cache PRIVATE src/gameplay/weapons/source_weapon_shader_input.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/weapons/source_weapon_material_input.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/effects/world_effects.cpp src/rendering/effects/particle_model.cpp src/audio/unit_damage_audio.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/effects/world_effects.cpp src/rendering/effects/particle_model.cpp src/audio/unit_damage_audio.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/gameplay/weapons/source_weapon_magazine.cpp)
target_sources(halo2_cache PRIVATE src/rendering/world/source_bsp_resident_plan.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/weapons/source_netgame_weapon.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_bitmap_residency.cpp src/rendering/source_bitmap_low_detail.cpp src/rendering/source_bitmap_cpu_selection.cpp src/rendering/source_bitmap_coverage.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_bitmap_initialization.cpp)
target_sources(halo2_cache PRIVATE src/platform/source_async_jobs.cpp src/rendering/source_bitmap_async_read.cpp src/rendering/source_bitmap_manager_transition.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_bitmap_reservation.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_bitmap_image_binding.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_bitmap_pending_header.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_bitmap_pending.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/source_bitmap_residency.cpp src/rendering/source_bitmap_cpu_selection.cpp src/rendering/source_bitmap_coverage.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/source_bitmap_residency.cpp src/rendering/source_bitmap_cpu_selection.cpp src/rendering/source_bitmap_coverage.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(MSVC)
    set_source_files_properties(src/gameplay/weapons/source_weapon_shader_input.cpp src/gameplay/weapons/source_weapon_magazine.cpp src/gameplay/weapons/source_netgame_weapon.cpp src/gameplay/weapons/source_netgame_weapon_scene.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/weapons/source_weapon_shader_input.cpp src/gameplay/weapons/source_weapon_magazine.cpp src/gameplay/weapons/source_netgame_weapon.cpp src/gameplay/weapons/source_netgame_weapon_scene.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/ui/frontend_text_style.cpp)
target_sources(halo2_cache PRIVATE src/ui/frontend_typewriter.cpp)
target_sources(halo2_cache PRIVATE src/ui/frontend_ui_assets.cpp src/ui/frontend_font_sampling.cpp)
target_sources(halo2_cache PRIVATE src/ui/gameplay_font_source.cpp)
target_sources(halo2_cache PRIVATE src/ui/hud/mcc_hud_reference.cpp)
target_sources(halo2_cache PRIVATE src/ui/hud/mcc_hud_bitmap_geometry.cpp)
target_sources(halo2_cache PRIVATE src/ui/hud/mcc_hud_shader_reference.cpp)
target_sources(halo2_cache PRIVATE src/ui/frontend_extension_catalog.cpp)
target_sources(halo2_cache PRIVATE src/ui/frontend_vista.cpp src/ui/vista/vista_lobby_metadata.cpp)
target_link_libraries(halo2_cache PRIVATE ZLIB::ZLIB Threads::Threads)
include(cmake/MccDecompression.cmake)
halo2_mcc_decompression(halo2_cache)
target_sources(halo2_cache PRIVATE src/ui/frontend_appearance.cpp)
target_sources(halo2_cache PRIVATE src/ui/frontend_emblem.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/units/player_armor_emblem.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_emblem.cpp)
target_sources(halo2_cache PRIVATE src/ui/frontend_model.cpp src/ui/frontend_model_camera.cpp)
target_sources(halo2_cache PRIVATE src/ui/frontend_model_lighting.cpp)
target_sources(halo2_cache PRIVATE src/ui/frontend_model_shader.cpp)
if(MSVC)
    set_source_files_properties(src/ui/frontend_model_shader.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/ui/frontend_model_shader.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/ui/frontend_model_locals.cpp src/ui/frontend_model_primary.cpp src/ui/frontend_model_ik.cpp src/ui/frontend_model_pose.cpp)
if(MSVC)
    set_source_files_properties(src/ui/frontend_model_locals.cpp src/ui/frontend_model_primary.cpp src/ui/frontend_model_ik.cpp src/ui/frontend_model_pose.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/ui/frontend_model_locals.cpp src/ui/frontend_model_primary.cpp src/ui/frontend_model_ik.cpp src/ui/frontend_model_pose.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(MSVC)
    set_source_files_properties(src/ui/frontend_model_camera.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/ui/frontend_model_camera.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(MSVC)
    set_source_files_properties(src/ui/frontend_text_style.cpp src/ui/frontend_painter.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/ui/frontend_text_style.cpp src/ui/frontend_painter.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(MSVC)
    set_source_files_properties(src/ui/popup_blur.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/ui/popup_blur.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/rendering/world/bsp_surface_lighting.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/world/bsp_surface_lighting.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/world/bsp_surface_lighting.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/rendering/world/bsp_collision_vector.cpp)
target_sources(halo2_cache PRIVATE src/runtime/source_instance_accelerator.cpp)
if(MSVC)
    set_source_files_properties(src/runtime/source_instance_accelerator.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/runtime/source_instance_accelerator.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/gameplay/objects/object_sky_lighting.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/object_light_contacts.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_light_attribution.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_light_streams.cpp)
target_sources(halo2_cache PRIVATE src/physics/source_vector_contact.cpp)
target_sources(halo2_cache PRIVATE src/rendering/renderer/source_render_contact.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_light_rays.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/objects/source_object_light_attribution.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/gameplay/objects/source_object_light_streams.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/physics/source_vector_contact.cpp src/rendering/renderer/source_render_contact.cpp src/gameplay/objects/source_object_light_rays.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/objects/source_object_light_attribution.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    set_source_files_properties(src/gameplay/objects/source_object_light_streams.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    set_source_files_properties(src/physics/source_vector_contact.cpp src/rendering/renderer/source_render_contact.cpp src/gameplay/objects/source_object_light_rays.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/gameplay/objects/object_light_spatial.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_spatial_query.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/objects/source_object_spatial_query.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/objects/source_object_spatial_query.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/rendering/models/model_selection.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/object_initialization.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/camera/source_camera.cpp src/rendering/scope_copy.cpp)
target_sources(halo2_cache PRIVATE src/rendering/shadows/source_shadow.cpp)
target_sources(halo2_cache PRIVATE src/rendering/shadows/source_shadow_plan.cpp)
target_sources(halo2_cache PRIVATE src/rendering/shadows/source_shadow_raw_draw.cpp src/rendering/shadows/source_shadow_raw_plan.cpp)
target_sources(halo2_cache PRIVATE src/rendering/models/source_model_fade.cpp)
target_sources(halo2_cache PRIVATE src/rendering/shadows/source_shadow_view.cpp)
target_sources(halo2_cache PRIVATE src/rendering/shadows/source_shadow_secondary.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_draw_unit_cull.cpp src/rendering/world/source_bsp_draw_collector.cpp)
target_sources(halo2_cache PRIVATE src/rendering/world/source_bsp_part_commands.cpp)
target_sources(halo2_cache PRIVATE src/rendering/effects/source_effect_light_geometry_frame.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_draw_unit.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_draw_collector.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_presentation.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/objects/source_object_presentation.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/objects/source_object_presentation.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/rendering/source_draw_resources.cpp)
target_sources(halo2_cache PRIVATE src/rendering/models/source_model_resources.cpp)
target_sources(halo2_cache PRIVATE src/rendering/models/source_model_admission.cpp)
target_sources(halo2_cache PRIVATE src/rendering/models/source_model_node_bank.cpp)
target_sources(halo2_cache PRIVATE src/rendering/models/source_model_build.cpp)
target_sources(halo2_cache PRIVATE src/rendering/models/source_model_lighting.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/models/source_model_admission.cpp src/rendering/models/source_model_node_bank.cpp src/rendering/models/source_model_lighting.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/models/source_model_admission.cpp src/rendering/models/source_model_node_bank.cpp src/rendering/models/source_model_lighting.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/gameplay/camera/source_sphere_rectangle.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/source_draw_unit.cpp src/gameplay/camera/source_sphere_rectangle.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/source_draw_unit.cpp src/gameplay/camera/source_sphere_rectangle.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/rendering/source_opaque_sort.cpp)
target_sources(halo2_cache PRIVATE src/rendering/effects/source_effect_light_opaque.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/world/source_bsp_part_commands.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/world/source_bsp_part_commands.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/rendering/source_prt_contact.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_bloom_context.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/world/bsp_atmosphere.cpp src/rendering/source_bloom_context.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/world/bsp_atmosphere.cpp src/rendering/source_bloom_context.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_light_mix.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_posed_target.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_light_uv.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_light_sample.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/objects/source_object_light_sample.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/objects/source_object_light_sample.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(MSVC)
    set_source_files_properties(src/gameplay/objects/source_object_posed_target.cpp src/gameplay/objects/source_object_light_uv.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/objects/source_object_posed_target.cpp src/gameplay/objects/source_object_light_uv.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(MSVC)
    set_source_files_properties(src/rendering/source_prt_contact.cpp src/gameplay/objects/source_object_light_mix.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/source_prt_contact.cpp src/gameplay/objects/source_object_light_mix.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/gameplay/multiplayer/source_local_player_priority.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/models/source_model_fade.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/shadows/source_shadow_view.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/source_draw_unit_cull.cpp src/rendering/world/source_bsp_draw_collector.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/models/source_model_fade.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    set_source_files_properties(src/rendering/shadows/source_shadow_view.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    set_source_files_properties(src/rendering/source_draw_unit_cull.cpp src/rendering/world/source_bsp_draw_collector.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/rendering/shadows/source_shadow_filter.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_vertex_declaration.cpp)
target_sources(halo2_cache PRIVATE src/ui/menu_world_source_geometry.cpp)
target_sources(halo2_cache PRIVATE src/rendering/world/mcc_bsp_vertex_layout.cpp)
target_sources(halo2_cache PRIVATE src/rendering/models/mcc_model_vertex_layout.cpp)
target_sources(halo2_cache PRIVATE src/rendering/models/mcc_model_node_map.cpp)
target_sources(halo2_cache PRIVATE src/rendering/models/mcc_skinned_model_vertex_layout.cpp)
target_sources(halo2_cache PRIVATE src/assets/mcc/mcc_resource_reader.cpp)
target_sources(halo2_cache PRIVATE src/ui/menu_world_source_binding.cpp)
target_sources(halo2_cache PRIVATE src/ui/menu_world_source_pass.cpp)
target_sources(halo2_cache PRIVATE src/ui/menu_world_vertex_source.cpp)
target_sources(halo2_cache PRIVATE src/ui/menu_world_vertex_catalog.cpp)
target_sources(halo2_cache PRIVATE src/ui/menu_world_fragment.cpp)
target_sources(halo2_cache PRIVATE src/ui/menu_world_raster.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_transparent_group_parts.cpp)
target_sources(halo2_cache PRIVATE src/ui/menu_world_entry_stencil.cpp)
if(MSVC)
    set_source_files_properties(src/ui/menu_world_source_geometry.cpp src/ui/menu_world_source_binding.cpp src/ui/menu_world_fragment.cpp
        src/rendering/world/mcc_bsp_vertex_layout.cpp src/rendering/models/mcc_model_vertex_layout.cpp src/rendering/models/mcc_skinned_model_vertex_layout.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/ui/menu_world_source_geometry.cpp src/ui/menu_world_source_binding.cpp src/ui/menu_world_fragment.cpp
        src/rendering/world/mcc_bsp_vertex_layout.cpp src/rendering/models/mcc_model_vertex_layout.cpp src/rendering/models/mcc_skinned_model_vertex_layout.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/rendering/models/source_cpu_skin_inputs.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/source_vertex_declaration.cpp src/rendering/models/source_cpu_skin_inputs.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/source_vertex_declaration.cpp src/rendering/models/source_cpu_skin_inputs.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/gameplay/animation/source_device.cpp src/rendering/source_prt.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_device_motion.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_machine_door.cpp src/rendering/models/source_model_regions.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_device_event.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_device_world_definition.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/objects/source_device_motion.cpp src/gameplay/objects/source_machine_door.cpp src/rendering/models/source_model_regions.cpp src/gameplay/objects/native_machine_door_world.cpp src/gameplay/objects/source_device_world.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/objects/source_device_motion.cpp src/gameplay/objects/source_machine_door.cpp src/rendering/models/source_model_regions.cpp src/gameplay/objects/native_machine_door_world.cpp src/gameplay/objects/source_device_world.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_prop.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/prop_sandbox_definition.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_prop_initial_state.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_scenery_initial_state.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_scenery_idle_phase.cpp)
target_sources(halo2_cache PRIVATE src/core/source_euler.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/camera/source_view_projection.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/objects/source_scenery_idle_phase.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/core/source_euler.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/gameplay/camera/source_view_projection.cpp src/rendering/renderer/render_view.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/objects/source_scenery_idle_phase.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/core/source_euler.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/gameplay/camera/source_view_projection.cpp src/rendering/renderer/render_view.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
endif()
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_colors.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_geometry_lod.cpp src/rendering/world/source_visibility.cpp)
target_sources(halo2_cache PRIVATE src/rendering/world/source_portal_clip.cpp)
target_sources(halo2_cache PRIVATE src/rendering/world/source_frustum_camera.cpp)
target_sources(halo2_cache PRIVATE src/rendering/world/source_portal_frustum.cpp)
target_sources(halo2_cache PRIVATE src/rendering/world/source_portal_visibility.cpp)
target_sources(halo2_cache PRIVATE src/rendering/world/source_bsp_admission.cpp)
target_sources(halo2_cache PRIVATE src/rendering/world/source_bsp_accelerator.cpp src/rendering/world/source_bsp_ranges.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/world/source_bsp_accelerator.cpp src/rendering/world/source_bsp_ranges.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/world/source_bsp_accelerator.cpp src/rendering/world/source_bsp_ranges.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/rendering/world/source_bsp_shader_scene.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_admission.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_admission_memory.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_render_queue.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_render_entry.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_geometry_resource.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_geometry_resource_request.cpp)
target_link_libraries(halo2_cache PUBLIC SDL2::SDL2)
if(MSVC)
    set_source_files_properties(src/gameplay/objects/source_object_render_entry.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/objects/source_object_render_entry.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_render_flags.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_location.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_primitive.cpp)
target_sources(halo2_cache PRIVATE src/rendering/world/source_portal_mask.cpp)
target_sources(halo2_cache PRIVATE src/ui/mainmenu_portal_state.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_transparent_sort.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_transparent_queue.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_transparent_parts.cpp)
target_sources(halo2_cache PRIVATE src/rendering/models/source_model_palette.cpp)
target_sources(halo2_cache PRIVATE src/rendering/models/source_model_pose.cpp)
target_sources(halo2_cache PRIVATE src/rendering/shadows/source_shadow_submission.cpp)
target_sources(halo2_cache PRIVATE src/rendering/shadows/source_shadow_lighting.cpp)
target_sources(halo2_cache PRIVATE src/rendering/renderer/source_render_cache.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_spatial_pool.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_spatial_scan.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_object_vector_contact.cpp)
target_sources(halo2_cache PRIVATE src/ui/popup_blur.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_shader_lod.cpp src/rendering/source_shader_lod_definition.cpp)
target_sources(halo2_cache PRIVATE src/rendering/models/source_model_shader.cpp)
target_sources(halo2_cache PRIVATE src/rendering/models/source_dynamic_model_shaders.cpp)
target_sources(halo2_cache PRIVATE src/rendering/models/source_shield_model_shaders.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_stipple.cpp)
target_sources(halo2_cache PRIVATE src/rendering/renderer/source_render_policy.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/source_geometry_lod.cpp src/rendering/world/source_visibility.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/world/source_portal_clip.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/world/source_frustum_camera.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/world/source_portal_frustum.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/world/source_portal_visibility.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/world/source_bsp_admission.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/ui/mainmenu_bsp_frame.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/gameplay/objects/source_object_admission.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/gameplay/objects/source_object_location.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/gameplay/objects/source_object_spatial_pool.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/world/source_portal_mask.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/source_transparent_sort.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/source_transparent_queue.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/source_transparent_parts.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/models/source_model_palette.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/models/source_model_pose.cpp src/rendering/models/model_pose.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/shadows/source_shadow_submission.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/source_shader_lod.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/models/source_model_shader.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/source_geometry_lod.cpp src/rendering/world/source_visibility.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/world/source_portal_clip.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/world/source_frustum_camera.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/world/source_portal_frustum.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/world/source_portal_visibility.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/world/source_bsp_admission.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/ui/mainmenu_bsp_frame.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/gameplay/objects/source_object_admission.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/gameplay/objects/source_object_location.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/gameplay/objects/source_object_spatial_pool.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/world/source_portal_mask.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/source_transparent_sort.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/source_transparent_queue.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    set_source_files_properties(src/rendering/source_transparent_parts.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/models/source_model_palette.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/models/source_model_pose.cpp src/rendering/models/model_pose.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/shadows/source_shadow_submission.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/source_shader_lod.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/models/source_model_shader.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
endif()
target_sources(halo2_cache PRIVATE src/ui/menu_screen_effect.cpp)
target_sources(halo2_cache PRIVATE src/campaign/cinematic_crossfade.cpp)
target_sources(halo2_cache PRIVATE src/ui/menu_noise_texture.cpp)
target_sources(halo2_cache PRIVATE src/rendering/effects/light_volume.cpp)
target_sources(halo2_cache PRIVATE src/rendering/effects/particle_sprite.cpp)
target_sources(halo2_cache PRIVATE src/rendering/effects/particle_model.cpp)
target_sources(halo2_cache PRIVATE src/rendering/effects/particle_values.cpp)
target_sources(halo2_cache PRIVATE src/rendering/effects/particle_render_inputs.cpp)
target_sources(halo2_cache PRIVATE src/rendering/effects/particle_birth.cpp)
target_sources(halo2_cache PRIVATE src/rendering/effects/particle_emitter.cpp)
target_sources(halo2_cache PRIVATE src/rendering/effects/particle_creation.cpp)
target_sources(halo2_cache PRIVATE src/rendering/effects/particle_vertex.cpp)
target_sources(halo2_cache PRIVATE src/rendering/effects/particle_material.cpp)
target_sources(halo2_cache PRIVATE src/rendering/effects/source_particle_sampler.cpp)
target_sources(halo2_cache PRIVATE src/rendering/renderer/material_effects.cpp)
target_sources(halo2_cache PRIVATE src/rendering/effects/particle_view.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/animation/first_person_ik.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/units/reference_player.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/animation/action_timing.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/animation/fp_registration.cpp)
add_executable(mcc_first_person_binding_tests EXCLUDE_FROM_ALL tests/mcc_first_person_binding_tests.cpp)
target_sources(mcc_first_person_binding_tests PRIVATE src/gameplay/weapons/source_weapon_animation_query_symbols_tu15.cpp)
target_link_libraries(mcc_first_person_binding_tests PRIVATE halo2_cache)
target_sources(halo2_cache PRIVATE src/gameplay/weapons/fp_clock.cpp)
target_sources(halo2_cache PRIVATE src/ui/frontend_emblem.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/pickup_visibility.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_variation.cpp)

target_sources(halo2_cache PRIVATE src/gameplay/objects/item_point.cpp)
target_link_libraries(halo2_cache PUBLIC halo2_glm)
target_sources(halo2_cache PRIVATE src/ui/hud/hud_content.cpp)
target_sources(halo2_cache PRIVATE src/ui/hud/motion_sensor.cpp src/runtime/native_parallel.cpp)
target_sources(halo2_cache PRIVATE src/assets/original_data.cpp src/platform/native_tu15_operands.generated.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/animation/unit_animation_metadata_store.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_metadata_store.cpp)
target_sources(halo2_cache PRIVATE src/assets/source_tag_metadata_store.cpp)
include(cmake/Tags.cmake)
target_sources(halo2_cache PRIVATE src/platform/source_mutable_tag_store.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/weapons/fp_sound_request.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/weapons/fp_sound_submit.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(src/gameplay/weapons/fp_sound_submit.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
elseif(MSVC)
    set_source_files_properties(src/gameplay/weapons/fp_sound_submit.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
endif()
target_sources(halo2_cache PRIVATE src/audio/sound_object_request.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_driver_parameters.cpp)
add_library(halo2_sound_dsp_encoder_stereo_tu15 STATIC src/audio/dsp/sound_dsp_encoder_stereo_tu15.cpp)
target_include_directories(halo2_sound_dsp_encoder_stereo_tu15 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(halo2_cache PUBLIC halo2_sound_dsp_encoder_stereo_tu15)
add_library(halo2_sound_gp_image_tu15 STATIC src/audio/dsp/sound_gp_image_tu15.cpp)
target_include_directories(halo2_sound_gp_image_tu15 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(halo2_sound_gp_image_tu15 PUBLIC OpenSSL::Crypto)
target_link_libraries(halo2_cache PUBLIC halo2_sound_gp_image_tu15)
include(cmake/SoundGpCpuTu15.cmake)
if(MSVC)
    set_source_files_properties(src/audio/sound_object_request.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/audio/sound_driver_parameters.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/audio/sound_object_request.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
    set_source_files_properties(src/audio/sound_driver_parameters.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_cache PRIVATE src/gameplay/multiplayer/multiplayer_balance.cpp)
target_sources(halo2_cache PRIVATE src/ui/vista/vista_cache.cpp)
add_library(halo2_mcc_sound_tag_layout STATIC src/audio/mcc_sound_tag_layout.cpp)
target_include_directories(halo2_mcc_sound_tag_layout PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(halo2_cache PUBLIC halo2_mcc_sound_tag_layout)
target_sources(halo2_cache PRIVATE src/assets/mcc_cache.cpp src/assets/mcc_cache_decode.cpp src/gameplay/animation/mcc_animation.cpp src/physics/mcc_physics.cpp src/audio/mcc_audio.cpp src/rendering/mcc_native_shaders.cpp src/runtime/mcc_native_library.cpp src/assets/source_cache_resources.cpp src/runtime/mcc_runtime_delivery.cpp src/ui/mcc_loading_inputs.cpp src/assets/mcc/mcc_ordinary_pointer.cpp)
target_sources(halo2_cache PRIVATE src/assets/mcc/mcc_native_map_owner.cpp src/assets/mcc_prepared_map_owner.cpp)
target_sources(halo2_cache PRIVATE src/campaign/scripting/mcc_script_layout.cpp src/campaign/scripting/native_mcc_script_builtin.cpp)
target_sources(halo2_cache PRIVATE src/assets/mcc/mcc_gameplay_scalar.cpp)
target_sources(halo2_cache PRIVATE src/assets/mcc/mcc_gameplay_reference.cpp)
target_sources(halo2_cache PRIVATE src/assets/mcc/mcc_map_gameplay_vanilla.cpp)
target_sources(halo2_cache PRIVATE src/rendering/world/mcc_bsp_collision_reference.cpp)
target_sources(halo2_cache PRIVATE src/rendering/world/mcc_bsp_instance_bounds.cpp)
target_sources(halo2_cache PRIVATE src/audio/mcc_sound_runtime_delivery.cpp)
target_sources(halo2_cache PRIVATE src/assets/prepared_original_sound_bank.cpp)
target_sources(halo2_cache PRIVATE src/audio/mcc_sound_geometry.cpp)
target_sources(halo2_cache PRIVATE src/audio/mcc_sound_vanilla_reference.cpp)
target_sources(halo2_cache PRIVATE src/audio/mcc_sound_timing_reference.cpp)
target_sources(halo2_cache PRIVATE src/audio/mcc_sound_stored_time_reference.cpp)
target_sources(halo2_cache PRIVATE src/audio/mcc_sound_permutation_time_reference.cpp)
target_sources(halo2_cache PRIVATE src/audio/mcc_sound_class_reference.cpp)
target_sources(halo2_cache PRIVATE src/audio/mcc_sound_class_apply.cpp)
target_sources(halo2_cache PRIVATE src/assets/mcc/mcc_gameplay_unit_scalar.cpp)
target_sources(halo2_cache PRIVATE src/assets/mcc/mcc_destruction_model_scalar.cpp)
target_sources(halo2_cache PRIVATE src/assets/mcc/mcc_destruction_model_contract.cpp)
target_sources(halo2_cache PRIVATE src/assets/mcc/mcc_destruction_model_reference.cpp)
target_sources(halo2_cache PRIVATE src/campaign/mcc_cairo_vanilla.cpp)
target_sources(halo2_cache PRIVATE src/campaign/mcc_campaign_vanilla.cpp)
target_link_libraries(halo2_cache PRIVATE PkgConfig::OPUS)
add_executable(inspect_vista_cache tools/inspect_vista_cache.cpp)
target_link_libraries(inspect_vista_cache PRIVATE halo2_cache)
add_executable(export_dust2_hud EXCLUDE_FROM_ALL tools/export_dust2_hud.cpp)
target_link_libraries(export_dust2_hud PRIVATE halo2_cache)
add_executable(export_dust2_props EXCLUDE_FROM_ALL tools/export_dust2_props.cpp)
target_link_libraries(export_dust2_props PRIVATE halo2_cache)
add_executable(inspect_mcc_cache tools/inspect_mcc_cache.cpp)
target_sources(inspect_mcc_cache PRIVATE src/audio/audio.cpp src/audio/vista_audio_decode.cpp
    src/audio/vista_audio_framing.cpp src/audio/sound_fade.cpp src/audio/sound_classes.cpp)
target_link_libraries(inspect_mcc_cache PRIVATE halo2_cache PkgConfig::AVCODEC)
add_executable(inspect_mcc_physics tools/inspect_mcc_physics.cpp)
target_link_libraries(inspect_mcc_physics PRIVATE halo2_cache)
add_executable(inspect_mcc_runtime_delivery tools/inspect_mcc_runtime_delivery.cpp)
target_link_libraries(inspect_mcc_runtime_delivery PRIVATE halo2_cache nlohmann_json::nlohmann_json)
add_executable(verify_mcc_physics_constructor tools/verify_mcc_physics_constructor.cpp)
target_link_libraries(verify_mcc_physics_constructor PRIVATE halo2_physics_world_runtime nlohmann_json::nlohmann_json)
add_executable(inspect_bitmap tools/inspect_bitmap.cpp)
target_link_libraries(inspect_bitmap PRIVATE halo2_cache nlohmann_json::nlohmann_json)
if(BUILD_TESTING)
    add_executable(vista_cache_tests tests/vista_cache_tests.cpp)
    target_link_libraries(vista_cache_tests PRIVATE halo2_cache)
    add_test(NAME vista_cache_ownership COMMAND vista_cache_tests)
    add_executable(vista_asset_adapter_tests tests/vista_asset_adapter_tests.cpp)
    target_link_libraries(vista_asset_adapter_tests PRIVATE halo2_systems halo2_cache halo2_systems)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/lockout.map" AND
       EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map" AND UNIX)
        add_test(NAME vista_original_asset_bindings COMMAND vista_asset_adapter_tests
            "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    endif()
endif()
target_sources(halo2_cache PRIVATE src/gameplay/multiplayer/multiplayer_netgame_postprocess.cpp)
target_sources(halo2_cache PRIVATE src/rendering/weather/source_weather_assets.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_control.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/source_control_target.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/units/source_unit_center.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/source_unit_center.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(MSVC)
    set_source_files_properties(src/gameplay/objects/source_control_target.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/objects/source_control_target.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/rendering/weather/source_weather_initialization.cpp)
target_sources(halo2_cache PRIVATE src/rendering/weather/source_weather_plate_simulation.cpp)
target_sources(halo2_cache PRIVATE src/rendering/weather/source_weather_exclusion.cpp)
target_sources(halo2_cache PRIVATE src/rendering/models/model_collision.cpp)
target_include_directories(halo2_cache PUBLIC include)
if(MSVC)
    set_source_files_properties(src/rendering/weather/source_weather_assets.cpp src/rendering/weather/source_weather_initialization.cpp src/rendering/weather/source_weather_plate_simulation.cpp src/rendering/weather/source_weather_exclusion.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/gameplay/animation/action_timing.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/gameplay/weapons/fp_clock.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/world/terrain_outer.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/gameplay/objects/pickup_visibility.cpp src/ui/hud/motion_sensor.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/audio/sound_variation.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/gameplay/objects/item_point.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/effects/particle_sprite.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/effects/particle_values.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/effects/particle_render_inputs.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/effects/particle_birth.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/effects/particle_emitter.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/rendering/effects/particle_creation.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    target_compile_options(halo2_cache PRIVATE /W4)
else()
    set_source_files_properties(src/rendering/weather/source_weather_assets.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/weather/source_weather_initialization.cpp src/rendering/weather/source_weather_plate_simulation.cpp src/rendering/weather/source_weather_exclusion.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    set_source_files_properties(src/gameplay/animation/action_timing.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/gameplay/weapons/fp_clock.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/gameplay/objects/pickup_visibility.cpp src/ui/hud/motion_sensor.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/audio/sound_variation.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/gameplay/objects/item_point.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    target_compile_options(halo2_cache PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_library(halo2_native_synchronization src/platform/native_synchronization.cpp)
target_sources(halo2_native_synchronization PRIVATE src/platform/native_threads.cpp)
target_sources(halo2_native_synchronization PRIVATE src/runtime/native_execution_gate.cpp)
target_include_directories(halo2_native_synchronization PUBLIC include)
target_link_libraries(halo2_native_synchronization PUBLIC Threads::Threads)
if(MSVC)
    target_compile_options(halo2_native_synchronization PRIVATE /W4)
else()
    target_compile_options(halo2_native_synchronization PRIVATE -Wall -Wextra -Wpedantic)
endif()
target_link_libraries(halo2_cache PUBLIC halo2_native_synchronization)
add_library(halo2_sound_platform src/audio/sdl_sound_platform.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_startup.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_source_lifecycle.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_cache_startup.cpp)
target_sources(halo2_cache PRIVATE src/audio/native_sound_storage.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_map_activation.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_driver_startup.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_driver_owners.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_driver_cleanup.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_driver_listener.cpp src/audio/sound_submit.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_promotion.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_create.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_admission.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_spatial.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_driver_spatial.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_sdk_spatial.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_sdk_device.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_sdk_commit.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_sdk_setters.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_sdk_construction.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_sdk_voice_initialization.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_sdk_hardware_voice.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_sdk_voice_dispatch.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_sdk_sample_pages.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_sdk_voice_attachment.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_sdk_notifications.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_sdk_voice_retirement.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_cache_request.cpp src/audio/sound_driver_stream.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_driver_master_gain.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_capabilities.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_looping.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/objects/object_construction_tables.cpp)
target_sources(halo2_cache PRIVATE src/audio/sound_cache_lifecycle.cpp src/audio/sound_driver_update.cpp)
if(MSVC)
    set_source_files_properties(src/audio/sound_driver_listener.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/audio/sound_promotion.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/audio/sound_create.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/audio/sound_admission.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/audio/sound_spatial.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/audio/sound_driver_spatial.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/audio/sound_sdk_spatial.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/audio/sound_sdk_setters.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/audio/sound_sdk_voice_initialization.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/audio/sound_driver_master_gain.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/audio/sound_looping.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/audio/sound_driver_listener.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    set_source_files_properties(src/audio/sound_promotion.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    set_source_files_properties(src/audio/sound_driver_master_gain.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
    set_source_files_properties(src/audio/sound_looping.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
    set_source_files_properties(src/audio/sound_create.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math;-fno-associative-math")
    set_source_files_properties(src/audio/sound_admission.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math;-fno-associative-math")
    set_source_files_properties(src/audio/sound_spatial.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math;-fno-associative-math")
    set_source_files_properties(src/audio/sound_driver_spatial.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math;-fno-associative-math")
    set_source_files_properties(src/audio/sound_sdk_spatial.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math;-fno-associative-math")
    set_source_files_properties(src/audio/sound_sdk_setters.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math;-fno-associative-math")
    set_source_files_properties(src/audio/sound_sdk_voice_initialization.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math;-fno-associative-math")
endif()
target_include_directories(halo2_sound_platform PUBLIC include)
target_link_libraries(halo2_sound_platform PUBLIC SDL2::SDL2)
if(MSVC)
    target_compile_options(halo2_sound_platform PRIVATE /W4 /fp:strict)
else()
    target_compile_options(halo2_sound_platform PRIVATE -Wall -Wextra -Wpedantic -fno-fast-math -ffp-contract=off)
endif()
add_library(halo2_systems src/physics/collision/collision.cpp src/campaign/ai/navigation.cpp src/core/kernels.cpp src/campaign/scripting/scripts.cpp src/audio/audio.cpp src/audio/sound_classes.cpp src/assets/residency.cpp src/platform/media/movie.cpp src/gameplay/camera/camera_motion.cpp src/runtime/scene.cpp src/gameplay/simulation.cpp src/gameplay/multiplayer/multiplayer.cpp src/network/multiplayer_network.cpp src/ui/frontend_state.cpp src/ui/frontend_painter.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/multiplayer_defusal.cpp)
target_sources(halo2_systems PRIVATE src/core/presentation_effects.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/source_unit_center.cpp)
target_sources(halo2_systems PRIVATE src/campaign/ai/source_character_shaders.cpp)
target_sources(halo2_systems PRIVATE src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp)
if(NOT MSVC)
    set_property(SOURCE src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp APPEND PROPERTY
        COMPILE_OPTIONS -fno-fast-math -ffp-contract=off -frounding-math)
endif()
if(BUILD_TESTING)
    add_executable(source_scene_device_constructor_tu15_tests tests/source_scene_device_constructor_tu15_tests.cpp)
    target_link_libraries(source_scene_device_constructor_tu15_tests PRIVATE halo2_systems)
    add_test(NAME original_tu15_scene_device_constructor COMMAND source_scene_device_constructor_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_scene_device_constructor.original.json)
    set_tests_properties(original_tu15_scene_device_constructor PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(heavy_vehicle_choices_tu15_tests tests/heavy_vehicle_choices_tu15_tests.cpp)
    target_link_libraries(heavy_vehicle_choices_tu15_tests PRIVATE halo2_systems)
    add_test(NAME original_tu15_heavy_vehicle_choices COMMAND heavy_vehicle_choices_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_heavy_vehicle_choices.original.json)
    set_tests_properties(original_tu15_heavy_vehicle_choices PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(light_vehicle_choices_tu15_tests tests/light_vehicle_choices_tu15_tests.cpp)
    target_link_libraries(light_vehicle_choices_tu15_tests PRIVATE halo2_systems)
    add_test(NAME original_tu15_light_vehicle_choices COMMAND light_vehicle_choices_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_light_vehicle_choices.original.json)
    set_tests_properties(original_tu15_light_vehicle_choices PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(turret_vehicle_choices_tu15_tests tests/turret_vehicle_choices_tu15_tests.cpp)
    target_link_libraries(turret_vehicle_choices_tu15_tests PRIVATE halo2_systems)
    add_test(NAME original_tu15_turret_vehicle_choices COMMAND turret_vehicle_choices_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/turret_vehicle_choices_tu15_original.json)
    set_tests_properties(original_tu15_turret_vehicle_choices PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 900)
    add_executable(live_player_scheduler_tu15_tests tools/replay_live_player_scheduler_tu15.cpp)
    target_link_libraries(live_player_scheduler_tu15_tests PRIVATE halo2_systems)
    set(HALO2_TU15_PLAYER_SCHEDULER_CAPTURE "" CACHE PATH "Actual live14FEC0 input capture")
    set(HALO2_TU15_PLAYER_SCHEDULER_ORIGINAL "" CACHE PATH "Unchanged original hardware output for that capture")
    add_test(NAME original_tu15_live_player_scheduler COMMAND live_player_scheduler_tu15_tests
        "${HALO2_TU15_PLAYER_SCHEDULER_CAPTURE}" "${HALO2_TU15_PLAYER_SCHEDULER_ORIGINAL}"
        "${CMAKE_CURRENT_BINARY_DIR}/original-tu15-live-player-scheduler.json")
    set_tests_properties(original_tu15_live_player_scheduler PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()
if(BUILD_TESTING)
    add_executable(campaign_character_shader_probe tools/campaign_character_shader_probe.cpp)
    target_link_libraries(campaign_character_shader_probe PRIVATE halo2_systems halo2_menu_world_storage)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map")
        add_test(NAME original_campaign_character_shader_inventory COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/campaign_character_shader_inventory_tests.py)
        add_executable(campaign_scene_material_tests tests/campaign_scene_material_tests.cpp src/campaign/source_campaign_model.cpp)
        target_link_libraries(campaign_scene_material_tests PRIVATE halo2_cache)
        add_test(NAME native_campaign_scene_materials COMMAND campaign_scene_material_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map "${HALO2_TU15_GAMEPLAY_XBE}")
        set_tests_properties(native_campaign_scene_materials PROPERTIES SKIP_RETURN_CODE 77)
    endif()
    add_executable(vehicle_shading_capture tools/capture_vehicle_shading.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
    target_link_libraries(vehicle_shading_capture PRIVATE halo2_renderer halo2_systems SDL2::SDL2)
    target_compile_definitions(vehicle_shading_capture PRIVATE HALO2_VEHICLE_SHADING_REFERENCE=1)
    add_executable(live_vehicle_model_capture tools/capture_vehicle_shading.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
    target_link_libraries(live_vehicle_model_capture PRIVATE halo2_renderer halo2_systems SDL2::SDL2)
    target_compile_definitions(live_vehicle_model_capture PRIVATE HALO2_LIVE_VEHICLE_MODEL_CAPTURE=1)
    add_executable(live_prop_model_capture tools/capture_prop_model_light.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
    target_link_libraries(live_prop_model_capture PRIVATE halo2_renderer halo2_systems SDL2::SDL2)
    add_executable(live_unit_model_capture tools/capture_unit_model_light.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
    target_link_libraries(live_unit_model_capture PRIVATE halo2_renderer halo2_systems SDL2::SDL2)
    add_executable(vehicle_shader_alias_tests tests/vehicle_shader_alias_tests.cpp)
    target_link_libraries(vehicle_shader_alias_tests PRIVATE halo2_cache)
    add_test(NAME original_vehicle_shader_aliases COMMAND vehicle_shader_alias_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps)
    set_tests_properties(original_vehicle_shader_aliases PROPERTIES SKIP_RETURN_CODE 77)
endif()
if(MSVC)
    set_source_files_properties(src/campaign/ai/source_character_shaders.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/campaign/ai/source_character_shaders.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/rendering/renderer/render_interpolation.cpp)
target_sources(halo2_cache PRIVATE src/rendering/models/model_scale_resets.cpp src/campaign/cinematic_scale_resets.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/multiplayer_presentation.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/weapons/native_weapon_inventory.cpp src/gameplay/multiplayer/multiplayer_inventory.cpp)
target_sources(halo2_systems PRIVATE src/ui/hud/native_action_prompt.cpp src/gameplay/weapons/native_weapon_interaction.cpp src/campaign/campaign_weapon_interaction.cpp src/gameplay/units/source_unit_interaction_target.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(src/gameplay/units/source_unit_interaction_target.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_systems PRIVATE src/gameplay/units/damage_feedback.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/shield_overlay.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/units/damage_feedback.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/gameplay/units/shield_overlay.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/damage_feedback.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
    set_source_files_properties(src/gameplay/units/shield_overlay.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_systems PRIVATE src/rendering/effects/effect_light.cpp src/rendering/effects/effect_light_admission.cpp src/rendering/effects/effect_light_spatial.cpp src/gameplay/camera/source_sphere_position.cpp src/rendering/effects/source_light_spawn_contacts.cpp src/rendering/effects/effect_light_registration.cpp)
target_sources(halo2_systems PRIVATE src/rendering/effects/effect_light_lifetime.cpp)
target_sources(halo2_systems PRIVATE src/rendering/effects/effect_light_view.cpp)
target_sources(halo2_systems PRIVATE src/rendering/effects/effect_light_receiver.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/effects/effect_light_receiver.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/effects/effect_light_receiver.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
endif()
target_sources(halo2_systems PRIVATE src/rendering/effects/effect_part_control.cpp)
target_sources(halo2_cache PRIVATE src/rendering/effects/effect_light_draw.cpp)
target_sources(halo2_systems PRIVATE src/rendering/world/source_bsp_effect_lights.cpp)
target_sources(halo2_systems PRIVATE src/rendering/shadows/source_shadow_scene.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/world/source_bsp_effect_lights.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/world/source_bsp_effect_lights.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/rendering/effects/effect_light_scissor.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/effects/effect_light_scissor.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/effects/effect_light_scissor.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
endif()
if(MSVC)
    set_source_files_properties(src/rendering/effects/effect_light.cpp src/rendering/effects/effect_light_admission.cpp src/gameplay/camera/source_sphere_position.cpp src/rendering/effects/source_light_spawn_contacts.cpp src/rendering/effects/effect_light_registration.cpp src/rendering/effects/effect_light_lifetime.cpp src/rendering/effects/effect_light_view.cpp src/rendering/effects/effect_part_control.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/effects/effect_light.cpp src/rendering/effects/effect_light_admission.cpp src/gameplay/camera/source_sphere_position.cpp src/rendering/effects/source_light_spawn_contacts.cpp src/rendering/effects/effect_light_registration.cpp src/rendering/effects/effect_light_lifetime.cpp src/rendering/effects/effect_light_view.cpp src/rendering/effects/effect_part_control.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
endif()
if(BUILD_TESTING)
    add_executable(effect_light_admission_tests tests/effect_light_admission_tests.cpp)
    add_executable(effect_light_spatial_tests tests/effect_light_spatial_tests.cpp)
    add_executable(source_sphere_position_tests tests/source_sphere_position_tests.cpp)
    add_executable(effect_light_registration_tests tests/effect_light_registration_tests.cpp)
    add_executable(effect_light_lifetime_tests tests/effect_light_lifetime_tests.cpp)
    add_executable(effect_light_view_tests tests/effect_light_view_tests.cpp)
    target_link_libraries(effect_light_view_tests PRIVATE halo2_systems)
    add_test(NAME original_effect_light_view_history COMMAND effect_light_view_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_view.original.bin)
    add_executable(effect_light_receiver_tests tests/effect_light_receiver_tests.cpp)
    target_link_libraries(effect_light_receiver_tests PRIVATE halo2_systems)
    add_test(NAME original_effect_light_receiver COMMAND effect_light_receiver_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_receiver.original.bin)
    add_executable(effect_part_control_tests tests/effect_part_control_tests.cpp)
    target_link_libraries(effect_part_control_tests PRIVATE halo2_systems)
    add_test(NAME original_effect_part_control COMMAND effect_part_control_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_part_control.original.bin)
    target_link_libraries(effect_light_lifetime_tests PRIVATE halo2_systems)
    add_test(NAME original_effect_light_lifetime COMMAND effect_light_lifetime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_lifetime.original.bin)
    target_link_libraries(effect_light_registration_tests PRIVATE halo2_systems)
    add_test(NAME original_effect_light_registration COMMAND effect_light_registration_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_registration.original.bin)
    add_test(NAME original_effect_light_registration_terrain COMMAND effect_light_registration_tests --terrain
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_registration.original.bin)
    target_link_libraries(source_sphere_position_tests PRIVATE halo2_systems)
    add_test(NAME original_light_spawn_correction COMMAND source_sphere_position_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/light_spawn_correction.original.bin)
    add_test(NAME original_light_spawn_contacts COMMAND source_sphere_position_tests --terrain
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/light_spawn_correction.original.bin)
    target_link_libraries(effect_light_spatial_tests PRIVATE halo2_systems)
    add_test(NAME original_effect_light_spatial_pool COMMAND effect_light_spatial_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_spatial_pool.original.bin)
    target_link_libraries(effect_light_admission_tests PRIVATE halo2_systems)
    add_test(NAME original_effect_light_render_admission COMMAND effect_light_admission_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_admission.original.bin)
    add_test(NAME original_effect_light_candidates COMMAND effect_light_admission_tests --candidates
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_candidates.original.bin)
    add_test(NAME original_effect_light_visibility_visit COMMAND effect_light_admission_tests --visibility
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_visibility.original.bin)
    add_test(NAME original_effect_light_visibility_collection COMMAND effect_light_admission_tests --collection
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_collection.original.bin)
    add_executable(effect_light_tests tests/effect_light_tests.cpp)
    target_link_libraries(effect_light_tests PRIVATE halo2_systems)
    add_test(NAME original_effect_light_sphere COMMAND effect_light_tests --sphere
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_sphere.original.bin)
    add_test(NAME original_effect_light_view_uniforms COMMAND effect_light_tests --view-uniforms
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_view_uniforms.original.bin)
    add_test(NAME original_effect_light_attenuation COMMAND effect_light_tests --attenuation
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_attenuation.xemu.rgba)
    add_test(NAME original_effect_light_view_fades COMMAND effect_light_tests --view-fades
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_view_fades.original.bin)
    add_test(NAME original_effect_light_uniforms COMMAND effect_light_tests --uniforms
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_uniforms.original.bin)
    add_test(NAME original_effect_light_update COMMAND effect_light_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light.original.bin)
endif()
target_sources(halo2_systems PRIVATE src/ui/frontend_session.cpp)
target_sources(halo2_cache PRIVATE src/ui/frontend_custom_maps.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/multiplayer_world.cpp)
target_sources(halo2_systems PRIVATE src/ui/vista/native_vista_flow.cpp src/ui/vista/native_vista_pcr.cpp src/ui/vista/vista_lobby_input.cpp src/ui/vista/vista_lobby_leave_task.cpp src/ui/vista/vista_profile_catalog.cpp)
if(BUILD_TESTING)
  add_executable(native_vista_flow_tests tests/native_vista_flow_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp src/ui/frontend.cpp)
  target_link_libraries(native_vista_flow_tests PRIVATE halo2_renderer)
  add_test(NAME native_vista_offline_roundtrip COMMAND native_vista_flow_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
    "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map"
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe)
  set_tests_properties(native_vista_offline_roundtrip PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
  add_test(NAME native_vista_two_player_roundtrip COMMAND native_vista_flow_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
    "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map"
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe --two-players)
  set_tests_properties(native_vista_two_player_roundtrip PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
add_library(halo2_discord_identity src/network/services/discord_login.cpp src/network/services/steam_login.cpp)
target_include_directories(halo2_discord_identity PUBLIC include)
target_link_libraries(halo2_discord_identity PRIVATE halo2_online_protocol)
add_dependencies(halo2_discord_identity halo2_online_protocol_rust_build)
target_link_libraries(halo2_discord_identity PRIVATE CURL::libcurl OpenSSL::Crypto nlohmann_json::nlohmann_json Threads::Threads)
target_compile_options(halo2_discord_identity PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
if(WIN32)
    target_link_libraries(halo2_discord_identity PRIVATE ws2_32)
endif()
target_link_libraries(halo2_systems PUBLIC halo2_discord_identity)
set(HALO2_DISCORD_SOCIAL_SDK_ROOT "" CACHE PATH "Official Discord Social SDK directory (optional)")
add_library(halo2_discord_inbox src/network/services/discord_activity_inbox.cpp)
target_include_directories(halo2_discord_inbox PUBLIC include)
add_library(halo2_discord_social src/network/services/discord_social.cpp src/network/services/discord_voice_blocks.cpp)
target_include_directories(halo2_discord_social PUBLIC include)
target_link_libraries(halo2_discord_social PRIVATE halo2_discord_inbox)
target_link_libraries(halo2_discord_social PRIVATE halo2_discord_identity CURL::libcurl nlohmann_json::nlohmann_json Threads::Threads)
if(HALO2_DISCORD_SOCIAL_SDK_ROOT)
    find_path(HALO2_DISCORD_SOCIAL_INCLUDE discordpp.h PATHS "${HALO2_DISCORD_SOCIAL_SDK_ROOT}/include" NO_DEFAULT_PATH REQUIRED)
    find_library(HALO2_DISCORD_SOCIAL_LIBRARY discord_partner_sdk PATHS "${HALO2_DISCORD_SOCIAL_SDK_ROOT}/lib/release" NO_DEFAULT_PATH REQUIRED)
    target_include_directories(halo2_discord_social SYSTEM PRIVATE "${HALO2_DISCORD_SOCIAL_INCLUDE}")
    target_compile_definitions(halo2_discord_social PRIVATE HALO2_DISCORD_SOCIAL_SDK=1)
    target_link_libraries(halo2_discord_social PRIVATE "${HALO2_DISCORD_SOCIAL_LIBRARY}")
endif()
# Keep Linux native deliveries independent of the private vendor extraction.
# Other platform packaging is validated on its host before being enabled here.
function(halo2_bundle_discord_social target)
    if(HALO2_DISCORD_SOCIAL_SDK_ROOT AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set_property(TARGET ${target} APPEND PROPERTY BUILD_RPATH "$ORIGIN")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HALO2_DISCORD_SOCIAL_LIBRARY}" "$<TARGET_FILE_DIR:${target}>/libdiscord_partner_sdk.so"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${HALO2_DISCORD_SOCIAL_SDK_ROOT}/License-Notices.txt" "$<TARGET_FILE_DIR:${target}>/Discord-Social-SDK-Notices.txt"
            VERBATIM)
    endif()
endfunction()
target_sources(halo2_systems PRIVATE src/ui/frontend_friends.cpp)
target_link_libraries(halo2_systems PUBLIC halo2_discord_social)
add_library(halo2_online_directory src/network/online_directory.cpp)
target_include_directories(halo2_online_directory PUBLIC include)
target_link_libraries(halo2_online_directory PRIVATE CURL::libcurl nlohmann_json::nlohmann_json Threads::Threads)
target_compile_options(halo2_online_directory PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
target_sources(halo2_systems PRIVATE src/ui/frontend_directory.cpp)
target_link_libraries(halo2_systems PUBLIC halo2_online_directory)
add_library(halo2_online_matchmaking src/network/online_matchmaking.cpp src/network/online_match_session.cpp src/network/online_search_qos.cpp)
include(cmake/OnlineProtocolRust.cmake)
target_include_directories(halo2_discord_identity PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}" "${CMAKE_CURRENT_SOURCE_DIR}/rust/online_protocol_bridge")
include(cmake/SecurityTransport.cmake)
target_link_libraries(halo2_online_matchmaking PRIVATE halo2_online_protocol)
target_include_directories(halo2_online_matchmaking PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
add_dependencies(halo2_online_matchmaking halo2_online_protocol_rust_build)
target_link_libraries(halo2_systems PRIVATE halo2_online_protocol)
include(cmake/OnlinePeers.cmake)
include(cmake/PeerSnapshotCandidate.cmake)
target_include_directories(halo2_online_matchmaking PUBLIC include)
target_link_libraries(halo2_online_matchmaking PRIVATE halo2_online_peers halo2_authority_transfer halo2_source_admissions halo2_peer_mesh_quality halo2_peer_host_agreement halo2_peer_common_cut halo2_peer_snapshot_candidate halo2_discord_identity CURL::libcurl OpenSSL::Crypto nlohmann_json::nlohmann_json Threads::Threads)
target_link_libraries(halo2_systems PUBLIC halo2_peer_snapshot_candidate)
target_compile_options(halo2_online_matchmaking PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
add_executable(online_search_qos_tests tests/online_search_qos_tests.cpp)
target_link_libraries(online_search_qos_tests PRIVATE halo2_online_matchmaking)
target_sources(halo2_systems PRIVATE src/ui/frontend_matchmaking.cpp src/network/online_postgame.cpp)
target_link_libraries(halo2_systems PUBLIC halo2_online_matchmaking)
add_library(halo2_online_account src/network/online_account.cpp src/network/services/discord_party_activity.cpp)
target_include_directories(halo2_online_account PUBLIC include)
target_include_directories(halo2_online_account PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
add_dependencies(halo2_online_account halo2_online_protocol_rust_build)
target_link_libraries(halo2_online_account PRIVATE halo2_online_protocol)
target_link_libraries(halo2_online_account PRIVATE halo2_discord_identity CURL::libcurl OpenSSL::Crypto nlohmann_json::nlohmann_json Threads::Threads)
target_compile_options(halo2_online_account PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
target_link_libraries(halo2_discord_social PRIVATE halo2_online_account)
target_sources(halo2_systems PRIVATE src/ui/frontend_clans.cpp src/ui/frontend_party.cpp)
target_sources(halo2_systems PRIVATE src/ui/frontend_couch.cpp)
target_link_libraries(halo2_systems PUBLIC halo2_online_account)
add_library(halo2_online_voice src/network/online_voice.cpp src/gameplay/game_voice.cpp)
target_include_directories(halo2_online_voice PUBLIC include)
target_link_libraries(halo2_online_voice PRIVATE halo2_online_account SDL2::SDL2 CURL::libcurl PkgConfig::OPUS Threads::Threads)
target_compile_options(halo2_online_voice PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>)
target_link_libraries(halo2_systems PUBLIC halo2_online_voice)
target_link_libraries(halo2_online_matchmaking PRIVATE halo2_online_voice halo2_game_variant)
add_executable(game_voice_tests tests/game_voice_tests.cpp)
target_link_libraries(game_voice_tests PRIVATE halo2_online_voice)
if(BUILD_TESTING)
    add_executable(game_voice_handoff_tests tests/game_voice_handoff_tests.cpp)
    target_link_libraries(game_voice_handoff_tests PRIVATE halo2_online_voice)
    add_test(NAME game_voice_handoff COMMAND game_voice_handoff_tests)
    set_tests_properties(game_voice_handoff PROPERTIES TIMEOUT 20)
endif()
add_executable(online_login_probe tools/online_login_probe.cpp)
target_link_libraries(online_login_probe PRIVATE halo2_online_account)
add_executable(online_match_launch_tests tests/online_match_launch_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp src/ui/frontend.cpp)
target_link_libraries(online_match_launch_tests PRIVATE halo2_renderer)
target_compile_definitions(online_match_launch_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(online_match_launch_tests halo2_shaders)
add_executable(online_couch_launch_tests tests/online_couch_launch_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp src/ui/frontend.cpp)
target_link_libraries(online_couch_launch_tests PRIVATE halo2_renderer)
target_compile_definitions(online_couch_launch_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(online_couch_launch_tests halo2_shaders)

target_sources(halo2_cache PRIVATE src/rendering/models/model_markers.cpp)
target_sources(halo2_systems PRIVATE src/rendering/effects/weapon_effect.cpp)
target_sources(halo2_systems PRIVATE src/rendering/effects/muzzle_effects.cpp)
target_sources(halo2_systems PRIVATE src/rendering/effects/world_effects.cpp src/campaign/scripting/source_script_effect.cpp
    src/campaign/scripting/source_script_effect_runtime_tu15.cpp src/campaign/source_campaign_cinematic_runtime_tu15.cpp
    src/rendering/renderer/source_renderer_cpu_interface_tu15.cpp)
target_sources(halo2_systems PRIVATE src/rendering/effects/source_native_light_snapshot_tu15.cpp)
target_sources(halo2_systems PRIVATE src/campaign/source_campaign_camera_runtime_tu15.cpp)
target_sources(halo2_systems PRIVATE src/campaign/source_campaign_screen_runtime_tu15.cpp)
target_sources(halo2_systems PRIVATE src/campaign/source_campaign_dof_recipe_tu15.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/objects/source_object_functions_runtime_tu15.cpp)
target_sources(halo2_systems PRIVATE src/campaign/source_unit_cinematic_runtime_tu15.cpp)
target_sources(halo2_systems PRIVATE src/campaign/ai/source_unit_speech_runtime_tu15.cpp)
target_sources(halo2_systems PRIVATE src/campaign/source_campaign_model.cpp)
target_sources(halo2_systems PRIVATE src/campaign/source_campaign_weapons.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/weapons/source_weapon_firing_material_owner.cpp)
target_sources(halo2_systems PRIVATE src/campaign/source_campaign_view_tu15.cpp)
target_sources(halo2_systems PRIVATE src/campaign/source_campaign_lighting_tu15.cpp)
target_sources(halo2_systems PRIVATE src/campaign/scripting/source_scripted_sound_point_tu15.cpp)
target_sources(halo2_systems PRIVATE src/audio/unit_damage_audio.cpp)
if(BUILD_TESTING)
    add_executable(particle_model_tests tests/particle_model_tests.cpp)
    target_link_libraries(particle_model_tests PRIVATE halo2_cache)
endif()
target_sources(halo2_systems PRIVATE src/gameplay/objects/prop_sandbox.cpp src/gameplay/objects/source_device_world.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/objects/native_machine_door_world.cpp)
target_sources(halo2_systems PRIVATE src/ui/menu_music.cpp src/audio/sound_fade.cpp)
target_sources(halo2_systems PRIVATE src/audio/source_loop_track_transition_tu15.cpp)
target_sources(halo2_systems PRIVATE
    src/audio/source_loop_track_control_tu15.cpp src/audio/source_loop_track_update_tu15.cpp
    src/audio/source_loop_detail_control_tu15.cpp src/audio/source_loop_phased_output_tu15.cpp)
if(MSVC)
    set_source_files_properties(src/audio/source_loop_track_control_tu15.cpp src/audio/source_loop_detail_control_tu15.cpp src/audio/source_loop_phased_output_tu15.cpp
        PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/audio/source_loop_track_control_tu15.cpp src/audio/source_loop_detail_control_tu15.cpp src/audio/source_loop_phased_output_tu15.cpp PROPERTIES COMPILE_OPTIONS
        "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING)
    add_executable(source_loop_track_transition_tu15_tests
        tests/source_loop_track_transition_tu15_tests.cpp
        src/audio/source_loop_track_transition_tu15.cpp)
    target_include_directories(source_loop_track_transition_tu15_tests PRIVATE include)
    target_link_libraries(source_loop_track_transition_tu15_tests PRIVATE
        OpenSSL::Crypto nlohmann_json::nlohmann_json)
    add_test(NAME original_loop_track_transition_tu15
        COMMAND source_loop_track_transition_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/loop_track_transition_tu15.json")
    add_executable(secondary_trigger_intent_tu15_tests tests/secondary_trigger_intent_tu15_tests.cpp)
    target_include_directories(secondary_trigger_intent_tu15_tests PRIVATE include)
    target_compile_features(secondary_trigger_intent_tu15_tests PRIVATE cxx_std_20)
    target_link_libraries(secondary_trigger_intent_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME original_secondary_trigger_intent_tu15 COMMAND secondary_trigger_intent_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/secondary_trigger_intent_tu15.json")
    add_executable(scenery_audio_angle_tu15_tests tests/scenery_audio_angle_tu15_tests.cpp
        src/audio/source_object_loop_audio_tu15_generated.cpp)
    target_include_directories(scenery_audio_angle_tu15_tests PRIVATE src include)
    target_compile_features(scenery_audio_angle_tu15_tests PRIVATE cxx_std_20)
    target_link_libraries(scenery_audio_angle_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm)
    add_test(NAME original_scenery_audio_angle_tu15 COMMAND scenery_audio_angle_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/scenery_audio_angle_tu15.json")
    add_executable(source_object_audio_views_tu15_tests tests/source_object_audio_views_tu15_tests.cpp)
    target_link_libraries(source_object_audio_views_tu15_tests PRIVATE
        halo2_systems OpenSSL::Crypto nlohmann_json::nlohmann_json)
    add_test(NAME original_object_audio_native_views_tu15
        COMMAND source_object_audio_views_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/object_audio_views_tu15.json")
    add_executable(source_loop_track_control_tu15_tests
        tests/source_loop_track_control_tu15_tests.cpp src/audio/source_loop_track_control_tu15.cpp)
    target_include_directories(source_loop_track_control_tu15_tests PRIVATE include)
    target_link_libraries(source_loop_track_control_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME original_loop_track_control_tu15 COMMAND source_loop_track_control_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/loop_track_control_tu15.json")
    add_executable(source_loop_track_update_tu15_tests
        tests/source_loop_track_update_tu15_tests.cpp src/audio/source_loop_track_update_tu15.cpp
        src/audio/source_loop_track_transition_tu15.cpp)
    target_include_directories(source_loop_track_update_tu15_tests PRIVATE include)
    target_link_libraries(source_loop_track_update_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm)
    add_test(NAME original_loop_track_update_prefix_tu15 COMMAND source_loop_track_update_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/loop_track_update_prefix_tu15.json")
    add_executable(source_loop_track_decision_tu15_tests
        tests/source_loop_track_decision_tu15_tests.cpp src/audio/source_loop_track_update_tu15.cpp
        src/audio/source_loop_track_transition_tu15.cpp)
    target_include_directories(source_loop_track_decision_tu15_tests PRIVATE include)
    target_link_libraries(source_loop_track_decision_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm)
    add_test(NAME original_loop_track_decisions_tu15 COMMAND source_loop_track_decision_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/loop_track_update_tu15.json")
    add_executable(source_loop_detail_control_tu15_tests
        tests/source_loop_detail_control_tu15_tests.cpp src/audio/source_loop_detail_control_tu15.cpp)
    target_include_directories(source_loop_detail_control_tu15_tests PRIVATE include)
    target_link_libraries(source_loop_detail_control_tu15_tests PRIVATE nlohmann_json::nlohmann_json halo2_glm)
    add_test(NAME original_loop_detail_control_tu15 COMMAND source_loop_detail_control_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/loop_detail_control_tu15.json")
    set_tests_properties(original_loop_detail_control_tu15 PROPERTIES SKIP_RETURN_CODE 77)
endif()
target_sources(halo2_systems PRIVATE src/audio/multiplayer_audio.cpp)
target_sources(halo2_systems PRIVATE src/input/controller_settings.cpp src/ui/hud/action_prompt_bindings.cpp src/input/rumble.cpp src/gameplay/units/player_training.cpp src/input/training_controls.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/player_screen_flash.cpp src/gameplay/units/player_screen_flash_content.cpp)
target_sources(halo2_systems PRIVATE src/ui/hud/source_hud_messages.cpp)
if(MSVC)
    set_source_files_properties(src/ui/hud/source_hud_messages.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/ui/hud/source_hud_messages.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(MSVC)
    set_source_files_properties(src/gameplay/units/player_screen_flash.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/player_screen_flash.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_systems PRIVATE src/ui/frontend_lobby.cpp)
target_sources(halo2_systems PRIVATE
    src/ui/vista/vista_lobby_countdown.cpp
    src/ui/vista/vista_session_countdown.cpp
    src/ui/vista/vista_session_commit.cpp
    src/ui/vista/vista_session_readiness.cpp
    src/ui/vista/vista_frontend_readiness.cpp
    src/ui/vista/vista_lifecycle.cpp
    src/ui/vista/vista_menu_return_observer.cpp
    src/ui/vista/vista_slayer_publication.cpp
    src/ui/vista/vista_session_birth.cpp
    src/ui/vista/vista_local_user.cpp
    src/ui/vista/vista_controller_profile.cpp
    src/gameplay/multiplayer/multiplayer_outcome.cpp
    src/audio/vista_announcer_queue.cpp
    src/audio/vista_audio_framing.cpp
    src/audio/vista_audio_decode.cpp
    src/ui/vista/vista_loading.cpp)
if(NOT MSVC)
    set_source_files_properties(src/ui/vista/vista_loading.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
add_executable(decode_vista_audio tools/decode_vista_audio.cpp)
target_link_libraries(decode_vista_audio PRIVATE halo2_systems)
target_sources(halo2_systems PRIVATE src/gameplay/animation/animation_presentation.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/weapons/fp_replication.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/weapons/unit_grenade.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/weapons/native_grenade.cpp)
if(NOT MSVC)
    set_source_files_properties(src/gameplay/weapons/native_grenade.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_systems PRIVATE src/gameplay/animation/unit_animation.cpp src/gameplay/animation/unit_animation_pose.cpp)
target_sources(halo2_systems PRIVATE src/campaign/cinematic_animation.cpp)
target_sources(halo2_systems PRIVATE src/campaign/campaign_cinematic_animation.cpp)
target_sources(halo2_systems PRIVATE src/campaign/campaign_frame_clock.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/animation/unit_animation.cpp src/gameplay/animation/unit_animation_pose.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/animation/unit_animation.cpp src/gameplay/animation/unit_animation_pose.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(MSVC)
    set_source_files_properties(src/gameplay/weapons/unit_grenade.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/weapons/unit_grenade.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(MSVC)
    set_source_files_properties(src/gameplay/animation/animation_presentation.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/animation/animation_presentation.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
endif()
if(MSVC)
    set_source_files_properties(src/ui/menu_music.cpp src/audio/sound_fade.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/ui/menu_music.cpp src/audio/sound_fade.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
endif()
target_link_libraries(halo2_systems PUBLIC halo2_cache halo2_sound_platform SDL2::SDL2 PkgConfig::AVCODEC)
target_sources(halo2_systems PRIVATE src/app/desktop_session.cpp)
target_sources(halo2_systems PRIVATE src/input/multiplayer_kbm.cpp)
target_sources(halo2_systems PRIVATE src/input/pc_settings.cpp src/ui/frontend_pc_settings.cpp)
target_sources(halo2_systems PRIVATE src/ui/hud/full_scoreboard.cpp)
target_sources(halo2_cache PRIVATE src/ui/hud/full_scoreboard_assets.cpp src/ui/hud/full_scoreboard_painter.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/unit_locomotion.cpp)
target_sources(halo2_systems PRIVATE src/ui/hud/multiplayer_hud.cpp src/ui/hud/native_action_prompt_hud.cpp)
target_sources(halo2_systems PRIVATE src/ui/hud/source_hud_score.cpp src/ui/hud/source_hud_objectives.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/unit_input.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/unit_inventory.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/objects/object_schedule.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/player_command.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/objects/item_motion.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/objects/item_transform_math.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/objects/item_bounds.cpp src/gameplay/objects/item_nodes.cpp)
target_sources(halo2_systems PRIVATE src/physics/runtime/physics_contact_record.cpp)
target_sources(halo2_systems PRIVATE src/physics/runtime/physics_contact_lifecycle.cpp)
target_sources(halo2_systems PRIVATE src/physics/physics_body_state.cpp)
target_sources(halo2_systems PRIVATE src/physics/physics_body_factory.cpp)
target_sources(halo2_systems PRIVATE src/physics/physics_body_slots.cpp)
target_sources(halo2_systems PRIVATE src/physics/physics_shape_wrapper.cpp)
target_sources(halo2_systems PRIVATE src/physics/physics_body_grouping.cpp)
target_sources(halo2_systems PRIVATE src/physics/physics_body_disposal.cpp)
target_sources(halo2_systems PRIVATE src/physics/physics_world_membership.cpp)
target_sources(halo2_systems PRIVATE src/physics/collision/physics_broadphase.cpp)
target_sources(halo2_systems PRIVATE src/physics/collision/physics_pair_manager.cpp)
target_sources(halo2_systems PRIVATE src/physics/physics_agent_scheduler.cpp)
target_sources(halo2_systems PRIVATE src/physics/physics_current_broadphase.cpp)
target_sources(halo2_systems PRIVATE src/physics/physics_bsp_children.cpp)
target_sources(halo2_systems PRIVATE src/physics/physics_child_agents.cpp)
target_sources(halo2_systems PRIVATE src/physics/collision/physics_narrowphase_owners.cpp)
target_sources(halo2_systems PRIVATE src/physics/collision/physics_gjk.cpp)
target_sources(halo2_systems PRIVATE src/physics/runtime/physics_contact_batches.cpp)
target_sources(halo2_systems PRIVATE src/physics/physics_constraint_membership.cpp)
target_sources(halo2_systems PRIVATE src/physics/runtime/physics_contact_dispatch.cpp)
target_sources(halo2_systems PRIVATE src/physics/physics_motion_step.cpp src/physics/physics_world_step.cpp src/physics/physics_constraint_step.cpp)
if(MSVC)
    set_source_files_properties(src/physics/physics_motion_step.cpp src/physics/physics_world_step.cpp src/physics/physics_constraint_step.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/physics/physics_motion_step.cpp src/physics/physics_world_step.cpp src/physics/physics_constraint_step.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(MSVC)
    set_source_files_properties(src/physics/runtime/physics_contact_dispatch.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/physics/runtime/physics_contact_batches.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/physics/collision/physics_gjk.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/physics/collision/physics_narrowphase_owners.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/physics/physics_child_agents.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/physics/collision/physics_broadphase.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/physics/physics_shape_wrapper.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/physics/physics_body_grouping.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/physics/runtime/physics_contact_dispatch.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    set_source_files_properties(src/physics/runtime/physics_contact_batches.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    set_source_files_properties(src/physics/collision/physics_gjk.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    set_source_files_properties(src/physics/collision/physics_narrowphase_owners.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    set_source_files_properties(src/physics/physics_child_agents.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    set_source_files_properties(src/physics/collision/physics_broadphase.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    set_source_files_properties(src/physics/physics_shape_wrapper.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    set_source_files_properties(src/physics/physics_body_grouping.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_systems PRIVATE src/gameplay/objects/spatial_registry.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/weapons/weapon_error.cpp src/gameplay/weapons/smg.cpp src/gameplay/objects/inventory.cpp src/gameplay/multiplayer/spawn.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/unit_zoom.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/unit_scope_camera.cpp src/ui/hud/full_scoreboard_providers.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/weapons/unit_weapon_switch.cpp)
target_sources(halo2_systems PRIVATE src/ui/hud/multiplayer_scoreboard.cpp)
target_sources(halo2_cache PRIVATE src/ui/hud/hud_bitmap_geometry.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/weapons/unit_weapon_selection.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/unit_constructors.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/animation/unit_animation_resolution.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/weapons/unit_grenade_begin.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/objects/object_creation_request.cpp)
target_sources(halo2_systems PRIVATE src/campaign/source_scenario_request.cpp src/campaign/source_scenario_request_binding.cpp)
if(MSVC)
    set_source_files_properties(src/campaign/source_scenario_request_binding.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/campaign/source_scenario_request_binding.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_systems PRIVATE src/gameplay/objects/object_type_construction.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/unit_action_dispatch.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/objects/object_model_resources.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/objects/object_physics_construction.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/objects/raw_datum_pool.cpp src/gameplay/objects/raw_datum_pool_startup.cpp src/gameplay/objects/raw_datum_pool_creation.cpp src/platform/source_static_allocation.cpp src/runtime/source_runtime_host_storage.cpp src/gameplay/units/biped_component_storage.cpp src/physics/physics_component_construction.cpp)
target_sources(halo2_systems PRIVATE src/runtime/source_runtime_host_virtual_memory.cpp)
target_sources(halo2_systems PRIVATE src/physics/physics_component_rebuild.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(src/physics/physics_component_construction.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
elseif(MSVC)
    set_source_files_properties(src/physics/physics_component_construction.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
endif()
target_sources(halo2_systems PRIVATE src/gameplay/objects/object_construction.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/objects/object_storage_release.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/source_player_unit.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_round_reset.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/objects/object_preparation_pressure.cpp)
target_sources(halo2_systems PRIVATE src/platform/source_main_heap_storage.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/unit_constructor_tail.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/unit_local_transition.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/animation/unit_channel_request.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/animation/unit_subchannel_caller.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/unit_alternate_request.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/animation/unit_pose_requests.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/animation/animation_residency_capacity.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/animation/animation_async_constructor.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/animation/animation_async_worker.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/animation/animation_variant.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/animation/animation_variant.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/animation/animation_variant.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_cache PRIVATE src/gameplay/animation/unit_animation_selection.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/animation/unit_animation_selection.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/animation/unit_animation_selection.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(MSVC)
    set_source_files_properties(src/gameplay/units/unit_zoom.cpp src/gameplay/units/unit_scope_camera.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/ui/hud/full_scoreboard.cpp src/ui/hud/full_scoreboard_painter.cpp src/ui/hud/multiplayer_scoreboard.cpp src/ui/hud/hud_bitmap_geometry.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/unit_zoom.cpp src/gameplay/units/unit_scope_camera.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    set_source_files_properties(src/ui/hud/full_scoreboard.cpp src/ui/hud/full_scoreboard_painter.cpp src/ui/hud/multiplayer_scoreboard.cpp src/ui/hud/hud_bitmap_geometry.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_systems PRIVATE src/physics/collision/posed_collision.cpp)
target_sources(halo2_systems PRIVATE src/rendering/world/terrain_outer.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/slayer.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_game_end.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_result_statistics.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_result_capture.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_result_event.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_score_event.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_king_visual.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_king_boundary.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_king_draw.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/multiplayer/source_king_visual.cpp src/gameplay/multiplayer/source_king_boundary.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/multiplayer/source_king_visual.cpp src/gameplay/multiplayer/source_king_boundary.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_result_retention.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/source_player_profile.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/source_player_lives.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/source_player_departure.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_king.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_ctf.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_king_replication.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_king_wire.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_engine_entity.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_engine_entry.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_engine_entry_decode.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/multiplayer/source_king.cpp src/gameplay/multiplayer/source_ctf.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/multiplayer/source_king.cpp src/gameplay/multiplayer/source_ctf.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_round_end.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_round_lifecycle.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/native_player_profile.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_game_over.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/source_session_game_over.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/multiplayer/source_game_over.cpp src/audio/sound_classes.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/multiplayer/source_game_over.cpp src/audio/sound_classes.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_systems PRIVATE src/gameplay/units/damage_history.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/damage_amount.cpp src/gameplay/units/source_player_damage.cpp)
target_sources(halo2_systems PRIVATE src/rendering/source_camouflage.cpp)
target_sources(halo2_cache PRIVATE src/rendering/source_camouflage_context.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/units/source_player_damage.cpp src/rendering/source_camouflage.cpp src/rendering/source_camouflage_context.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/source_player_damage.cpp src/rendering/source_camouflage.cpp src/rendering/source_camouflage_context.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_systems PRIVATE src/gameplay/units/unit_damage.cpp)
target_sources(halo2_systems PRIVATE src/core/datum_pool.cpp src/gameplay/objects/native_object_lifetimes.cpp)
include(cmake/SourcePoolContinuation.cmake)
include(cmake/MultiplayerObjectContinuation.cmake)
include(cmake/SceneContinuation.cmake)
include(cmake/WorldComponentsContinuation.cmake)
include(cmake/ScriptPresentationContinuation.cmake)
include(cmake/GrenadeContinuation.cmake)
include(cmake/SimulationContinuation.cmake)
target_sources(halo2_systems PRIVATE src/gameplay/objects/source_object_body_storage.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/lethal_attribution.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/objects/object_garbage.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/objects/owner_counter.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/objects/object_light_query.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/objects/object_light_query.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/objects/object_light_query.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_systems PRIVATE src/campaign/scripting/source_script.cpp src/campaign/scripting/source_script_lists.cpp)
target_sources(halo2_systems PRIVATE src/campaign/ai/source_ai_actor.cpp src/campaign/ai/source_ai_references.cpp
    src/campaign/ai/source_ai_import.cpp src/gameplay/units/source_unit_control.cpp src/campaign/ai/source_ai_actor_binding.cpp
    src/campaign/ai/source_ai_actor_lifecycle.cpp src/campaign/ai/source_ai_actor_slot_table_base.cpp
    src/campaign/ai/source_ai_actor_control.cpp src/campaign/ai/source_ai_actor_membership.cpp)
target_sources(halo2_systems PRIVATE src/rendering/weather/source_weather.cpp)
target_sources(halo2_systems PRIVATE src/rendering/weather/source_weather_world.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/source_game_time.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/source_game_time.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/source_game_time.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_systems PRIVATE src/rendering/weather/source_weather_wind.cpp)
target_sources(halo2_systems PRIVATE src/ui/menu_projection.cpp)
target_sources(halo2_systems PRIVATE src/ui/mainmenu_bsp_frame.cpp)
target_sources(halo2_systems PRIVATE src/ui/mainmenu_bsp_ranges.cpp)
if(MSVC)
    set_source_files_properties(src/ui/mainmenu_bsp_ranges.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/ui/mainmenu_bsp_ranges.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
target_sources(halo2_systems PRIVATE src/ui/mainmenu_world_source.cpp)
target_sources(halo2_systems PRIVATE src/ui/menu_world_depth.cpp)
if(MSVC)
    set_source_files_properties(src/ui/menu_world_depth.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/ui/menu_world_depth.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(WIN32)
    target_link_libraries(halo2_systems PUBLIC ws2_32)
endif()
target_compile_options(halo2_systems PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wno-missing-field-initializers>)
add_library(halo2_kernels SHARED src/rendering/atmosphere.cpp src/rendering/fog_composite.cpp src/rendering/patchy_fog.cpp src/ui/menu_bloom.cpp src/rendering/texture_state.cpp src/rendering/renderer/render_view.cpp src/core/kernels.cpp src/rendering/bitmap_mips.cpp src/rendering/renderer/render_program.cpp src/core/transform.cpp src/gameplay/camera/camera_motion.cpp)
target_sources(halo2_kernels PRIVATE src/ui/popup_blur.cpp)
target_sources(halo2_kernels PRIVATE src/ui/frontend_model_camera.cpp)
target_sources(halo2_kernels PRIVATE src/core/source_euler.cpp)
target_include_directories(halo2_kernels PRIVATE include)
add_library(halo2_weapon_kernels SHARED src/gameplay/weapons/weapon_error.cpp src/gameplay/weapons/smg.cpp src/gameplay/objects/inventory.cpp src/gameplay/multiplayer/spawn.cpp src/assets/original_data.cpp src/platform/native_tu15_operands.generated.cpp)
target_include_directories(halo2_weapon_kernels PRIVATE include)
if(MSVC)
    set_source_files_properties(src/rendering/weather/source_weather.cpp src/rendering/weather/source_weather_wind.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/gameplay/weapons/weapon_error.cpp src/gameplay/weapons/smg.cpp src/gameplay/multiplayer/spawn.cpp src/gameplay/objects/inventory.cpp src/physics/collision/posed_collision.cpp src/gameplay/multiplayer/slayer.cpp src/gameplay/units/damage_history.cpp src/gameplay/units/damage_amount.cpp src/gameplay/units/unit_damage.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/gameplay/animation/source_device.cpp src/rendering/source_prt.cpp src/gameplay/objects/source_prop.cpp src/gameplay/objects/source_machine_scene.cpp src/gameplay/objects/source_prop_scene.cpp src/gameplay/objects/source_object_geometry.cpp src/gameplay/objects/source_scenery_scene.cpp src/gameplay/objects/source_object_colors.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    target_compile_options(halo2_weapon_kernels PRIVATE /fp:strict)
else()
    target_compile_options(halo2_weapon_kernels PRIVATE -ffp-contract=off)
endif()
if(NOT MSVC)
    set_source_files_properties(src/campaign/campaign_frame_clock.cpp src/campaign/cinematic_crossfade.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    set_source_files_properties(src/rendering/weather/source_weather.cpp src/rendering/weather/source_weather_wind.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/audio/sound_classes.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/world/bsp_collision_vector.cpp src/gameplay/objects/object_lighting.cpp src/gameplay/camera/source_camera.cpp src/rendering/scope_copy.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/shadows/source_shadow.cpp src/gameplay/objects/source_object_colors.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/gameplay/animation/source_device.cpp src/rendering/source_prt.cpp src/gameplay/objects/source_prop.cpp src/gameplay/objects/source_machine_scene.cpp src/gameplay/objects/source_prop_scene.cpp src/gameplay/objects/source_object_geometry.cpp src/gameplay/objects/source_scenery_scene.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/gameplay/objects/object_sky_lighting.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/gameplay/multiplayer/multiplayer_netgame_postprocess.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/gameplay/objects/object_light_spatial.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/models/model_selection.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/gameplay/animation/first_person_ik.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/effects/light_volume.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/effects/particle_sprite.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/effects/particle_values.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/effects/particle_render_inputs.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/effects/particle_birth.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/effects/particle_emitter.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/effects/particle_creation.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    set_source_files_properties(src/rendering/renderer/render_view.cpp src/rendering/atmosphere.cpp src/rendering/fog_composite.cpp src/rendering/patchy_fog.cpp src/ui/menu_bloom.cpp src/core/transform.cpp src/rendering/shader_animation.cpp PROPERTIES COMPILE_OPTIONS -ffp-contract=off)
    target_compile_options(halo2_kernels PRIVATE -ffp-contract=off)
    target_compile_options(halo2_systems PRIVATE -ffp-contract=off)
endif()
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(halo2_discord_identity PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_discord_identity PUBLIC -fsanitize=address,undefined)
    target_compile_options(halo2_native_synchronization PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_native_synchronization PUBLIC -fsanitize=address,undefined)
    target_compile_options(halo2_sound_platform PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_sound_platform PUBLIC -fsanitize=address,undefined)
    target_compile_options(halo2_cache PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_cache PUBLIC -fsanitize=address,undefined)
    target_compile_options(halo2_weapon_kernels PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_weapon_kernels PUBLIC -fsanitize=address,undefined)
endif()

foreach(shader upscale_prepare.comp upscale_motion.vert upscale_motion.frag world.vert world_prt.vert world.frag light_volume.vert light_volume.frag particle.frag fog.vert fog.frag water_copy.frag scene_copy.frag scope_copy.frag bloom.vert bloom.frag cinematic_crossfade.frag crt_oled.frag display.frag patchy_noise.vert patchy_noise.frag patchy_composite.frag bsp_patchy_noise.frag bsp_patchy_composite.frag overlay.vert overlay.frag ui.vert ui.frag ui_frame.frag hud.frag vista_loading.vert vista_loading.frag)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader}.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${GLSLANG_VALIDATOR} -V ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader} -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader}.spv
        DEPENDS shaders/${shader} shaders/pstencil/specular_mask.glsl shaders/world_combiner_specialization.glsl shaders/world_vertex.glsl shaders/source_vertex_inputs.glsl shaders/source_cpu_skin_outputs.glsl shaders/fog_color.glsl shaders/patchy_color.glsl shaders/patchy_constants.glsl shaders/bloom_color.glsl shaders/combiner.glsl shaders/vertex_program.glsl shaders/texture_program.glsl shaders/render_view.glsl shaders/particle_depth.glsl shaders/world_depth_fragment.glsl shaders/source_depth_storage.glsl shaders/source_stipple.glsl shaders/source_alpha_test.glsl VERBATIM)
    list(APPEND SHADER_OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader}.spv)
endforeach()
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/world_emblem.geom.spv
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
    COMMAND ${GLSLANG_VALIDATOR} -V ${CMAKE_CURRENT_SOURCE_DIR}/shaders/world_emblem.geom
        -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/world_emblem.geom.spv
    DEPENDS shaders/world_emblem.geom VERBATIM)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/world_emblem.frag.spv
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
    COMMAND ${GLSLANG_VALIDATOR} -V -DH2_EMBLEM_W_DEPTH=1
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/world.frag
        -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/world_emblem.frag.spv
    DEPENDS shaders/world.frag shaders/world_combiner_specialization.glsl
        shaders/combiner.glsl shaders/texture_program.glsl shaders/source_alpha_test.glsl
        shaders/source_stipple.glsl shaders/world_depth_fragment.glsl shaders/source_depth_storage.glsl VERBATIM)
list(APPEND SHADER_OUTPUTS ${CMAKE_CURRENT_BINARY_DIR}/shaders/world_emblem.geom.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/world_emblem.frag.spv)
add_custom_target(halo2_shaders DEPENDS ${SHADER_OUTPUTS})
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world.geom.spv
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
    COMMAND ${GLSLANG_VALIDATOR} -V --target-env vulkan1.3 -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/menu_world.geom
        -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world.geom.spv
    DEPENDS shaders/menu_world.geom shaders/world_depth_geometry.glsl VERBATIM)
add_custom_target(menu_world_geometry_shader
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world.geom.spv)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world.frag.spv
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
    COMMAND ${GLSLANG_VALIDATOR} -V --target-env vulkan1.3 -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/menu_world.frag
        -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world.frag.spv
    DEPENDS shaders/menu_world.frag shaders/menu_world_fragment.glsl
        shaders/source_alpha_test.glsl shaders/combiner.glsl shaders/texture_program.glsl
        shaders/world_depth_fragment.glsl shaders/source_depth_storage.glsl VERBATIM)
add_custom_target(menu_world_fragment_shader
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world.frag.spv)
# Port shader sources are part of the engine. Normal builds must not require
# private Xbox/Vista caches or derive shader programs from MCC donor assets.
set(NATIVE_SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders/native-programs)
find_program(SPIRV_VALIDATOR spirv-val REQUIRED)
if(NOT CMAKE_CROSSCOMPILING)
    # Explicit offline reference maintenance; never a normal shader dependency.
    add_executable(generate_menu_world_vertex_catalog tools/generate_menu_world_vertex_catalog.cpp)
    target_link_libraries(generate_menu_world_vertex_catalog PRIVATE halo2_cache)
endif()
set(MENU_WORLD_VERTEX_CATALOG_DIR ${CMAKE_CURRENT_BINARY_DIR}/menu-world-vertex-catalog)
set(MENU_WORLD_VERTEX_CATALOG_HEADER ${MENU_WORLD_VERTEX_CATALOG_DIR}/menu_world_vertex_catalog.generated.hpp)
file(GLOB native_menu_vertex_sources CONFIGURE_DEPENDS ${NATIVE_SHADER_SOURCE_DIR}/menu/*.vert)
file(GLOB pstencil_projected_vertex_sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/pstencil/native-programs/*.vert)
add_custom_command(OUTPUT ${MENU_WORLD_VERTEX_CATALOG_HEADER} ${MENU_WORLD_VERTEX_CATALOG_DIR}/compiled.json
    COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tools/compile_menu_world_vertex_catalog.py
        --triangle-vertices
        --sources ${NATIVE_SHADER_SOURCE_DIR}/menu/sources.json
        --additional-sources ${CMAKE_CURRENT_SOURCE_DIR}/shaders/pstencil/native-programs/sources.json
        --compiler ${GLSLANG_VALIDATOR} --validator ${SPIRV_VALIDATOR}
        --header ${MENU_WORLD_VERTEX_CATALOG_HEADER}
        --manifest ${MENU_WORLD_VERTEX_CATALOG_DIR}/compiled.json
    DEPENDS ${NATIVE_SHADER_SOURCE_DIR}/menu/sources.json ${native_menu_vertex_sources}
        shaders/pstencil/native-programs/sources.json ${pstencil_projected_vertex_sources}
        tools/compile_menu_world_vertex_catalog.py tools/world_triangle_vertex.py shaders/source_world_vertex_fetch.glsl shaders/world_depth_geometry.glsl ${GLSLANG_VALIDATOR} ${SPIRV_VALIDATOR}
    VERBATIM)
add_custom_target(menu_world_vertex_catalog
    DEPENDS ${MENU_WORLD_VERTEX_CATALOG_HEADER} ${MENU_WORLD_VERTEX_CATALOG_DIR}/compiled.json)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_program.glsl
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_program.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_program.json
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_particle_graphics_program.py
        --preserved-sources ${NATIVE_SHADER_SOURCE_DIR}/particles.json
        --output ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_program.glsl
    DEPENDS tools/generate_particle_graphics_program.py tools/generate_particle_vertex_probe.py
        tools/compile_menu_world_vertex_catalog.py ${NATIVE_SHADER_SOURCE_DIR}/particles.json
        ${NATIVE_SHADER_SOURCE_DIR}/particle_program.glsl VERBATIM)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_directional_program.glsl
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_directional_program.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_directional_program.json
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_particle_graphics_program.py
        --preserved-sources ${NATIVE_SHADER_SOURCE_DIR}/particles.json
        --classification 8 --output ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_directional_program.glsl
    DEPENDS tools/generate_particle_graphics_program.py tools/generate_particle_vertex_probe.py
        tools/compile_menu_world_vertex_catalog.py ${NATIVE_SHADER_SOURCE_DIR}/particles.json
        ${NATIVE_SHADER_SOURCE_DIR}/particle_directional_program.glsl VERBATIM)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_directional.vert.spv
    COMMAND ${GLSLANG_VALIDATOR} -V -DPARTICLE_DIRECTIONAL=1
        -I${CMAKE_CURRENT_BINARY_DIR}/shaders -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/particle.vert
        -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_directional.vert.spv
    DEPENDS shaders/particle.vert shaders/particle_vertex_output.glsl
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_directional_program.glsl VERBATIM)
foreach(particle_stage vert geom)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle.${particle_stage}.spv
        COMMAND ${GLSLANG_VALIDATOR} -V -I${CMAKE_CURRENT_BINARY_DIR}/shaders -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders/particle.${particle_stage}
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle.${particle_stage}.spv
        DEPENDS shaders/particle.${particle_stage} shaders/particle_vertex_output.glsl
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_program.glsl VERBATIM)
endforeach()
foreach(particle_family perpendicular model)
    if(particle_family STREQUAL "model")
        set(particle_class 1)
        set(particle_macro PARTICLE_MODEL)
    else()
        set(particle_class 9)
        set(particle_macro PARTICLE_PERPENDICULAR)
    endif()
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_${particle_family}_program.glsl
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_${particle_family}_program.hpp
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_${particle_family}_program.json
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_particle_graphics_program.py
            --preserved-sources ${NATIVE_SHADER_SOURCE_DIR}/particles.json --classification ${particle_class}
            --output ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_${particle_family}_program.glsl
        DEPENDS tools/generate_particle_graphics_program.py tools/generate_particle_vertex_probe.py
            tools/compile_menu_world_vertex_catalog.py ${NATIVE_SHADER_SOURCE_DIR}/particles.json
            ${NATIVE_SHADER_SOURCE_DIR}/particle_${particle_family}_program.glsl VERBATIM)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_${particle_family}.vert.spv
        COMMAND ${GLSLANG_VALIDATOR} -V -D${particle_macro}=1
            -I${CMAKE_CURRENT_BINARY_DIR}/shaders -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders/particle.vert
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_${particle_family}.vert.spv
        DEPENDS shaders/particle.vert shaders/particle_vertex_output.glsl
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_${particle_family}_program.glsl VERBATIM)
endforeach()
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_distortion_program.glsl
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_distortion_program.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_distortion_program.json
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_particle_graphics_program.py
        --preserved-sources ${NATIVE_SHADER_SOURCE_DIR}/particles.json --classification 7 --variant distortion
        --output ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_distortion_program.glsl
    DEPENDS tools/generate_particle_graphics_program.py tools/compile_menu_world_vertex_catalog.py
        ${NATIVE_SHADER_SOURCE_DIR}/particles.json ${NATIVE_SHADER_SOURCE_DIR}/particle_distortion_program.glsl VERBATIM)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_distortion.vert.spv
    COMMAND ${GLSLANG_VALIDATOR} -V -DPARTICLE_DISTORTION=1
        -I${CMAKE_CURRENT_BINARY_DIR}/shaders -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/particle.vert
        -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_distortion.vert.spv
    DEPENDS shaders/particle.vert shaders/particle_vertex_output.glsl
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_distortion_program.glsl VERBATIM)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_model.geom.spv
    COMMAND ${GLSLANG_VALIDATOR} -V -DPARTICLE_MODEL=1
        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/particle.geom
        -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_model.geom.spv
    DEPENDS shaders/particle.geom VERBATIM)
add_custom_target(particle_graphics_shaders DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_distortion.vert.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_perpendicular.vert.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_model.vert.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_model.geom.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_directional.vert.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle.vert.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle.geom.spv)
add_library(halo2_particle_renderer src/rendering/effects/particle_renderer.cpp)
target_link_libraries(halo2_particle_renderer PUBLIC halo2_cache Vulkan::Vulkan)
target_include_directories(halo2_particle_renderer PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/shaders)
add_dependencies(halo2_particle_renderer particle_graphics_shaders halo2_shaders)
target_compile_options(halo2_particle_renderer PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wno-missing-field-initializers>)
foreach(shadow_filter_shader source_shadow_filter.vert source_shadow_filter.frag)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/${shadow_filter_shader}.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${GLSLANG_VALIDATOR} -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shadow_filter_shader}
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/${shadow_filter_shader}.spv
        DEPENDS shaders/${shadow_filter_shader} shaders/combiner.glsl shaders/texture_program.glsl VERBATIM)
endforeach()
add_custom_target(source_shadow_filter_shaders DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/source_shadow_filter.vert.spv
    ${CMAKE_CURRENT_BINARY_DIR}/shaders/source_shadow_filter.frag.spv)
add_library(halo2_shadow_render src/rendering/shadows/source_shadow_render.cpp src/rendering/shadows/source_shadow_draw_storage.cpp
    src/rendering/world/source_world_draw_storage.cpp)
target_link_libraries(halo2_shadow_render PUBLIC halo2_cache Vulkan::Vulkan)
target_compile_definitions(halo2_shadow_render PRIVATE HALO2_SHADOW_FILTER_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(halo2_shadow_render source_shadow_filter_shaders)
add_library(halo2_menu_world_storage src/ui/menu_world_source_storage.cpp src/ui/menu_world_triangle_storage.cpp)
target_link_libraries(halo2_menu_world_storage PUBLIC halo2_cache Vulkan::Vulkan)
add_library(halo2_renderer_device_lease src/rendering/renderer/renderer_device_lease.cpp)
target_include_directories(halo2_renderer_device_lease PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(halo2_renderer src/rendering/renderer/renderer.cpp src/rendering/world/bsp_postprocess.cpp src/rendering/scope_render.cpp src/campaign/cinematic_crossfade_render.cpp src/campaign/cinematic_camera_interpolation.cpp)
target_sources(halo2_renderer PRIVATE src/rendering/source_scene_copy_gpu.cpp)
target_link_libraries(halo2_renderer PUBLIC halo2_renderer_device_lease)
target_link_libraries(halo2_renderer PUBLIC halo2_renderer_palette)
target_sources(halo2_renderer PRIVATE src/rendering/renderer/renderer_device_lease.cpp)
target_sources(halo2_renderer PRIVATE src/app/desktop_loading.cpp)
target_sources(halo2_renderer PRIVATE src/ui/mcc_loading_presentation.cpp src/audio/mcc_loading_converter_metadata.cpp)
target_sources(halo2_renderer PRIVATE src/rendering/renderer/resident_draw_storage.cpp src/gameplay/weapons/source_grenade_scene.cpp)
target_sources(halo2_renderer PRIVATE src/gameplay/weapons/native_weapon_projectile_scene.cpp)
add_executable(rocket_ghost_projectile_scene_tests tests/rocket_ghost_projectile_scene_tests.cpp)
target_link_libraries(rocket_ghost_projectile_scene_tests PRIVATE halo2_renderer)
set(HALO2_ROCKET_GHOST_MCC_MAP "" CACHE FILEPATH "Prepared MCC Coagulation map for projectile presentation regressions")
add_test(NAME native_rocket_ghost_presentation COMMAND rocket_ghost_projectile_scene_tests "${HALO2_ROCKET_GHOST_MCC_MAP}")
set_tests_properties(native_rocket_ghost_presentation PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 90)
target_sources(halo2_systems PRIVATE src/gameplay/weapons/native_weapon_projectile.cpp)
set_source_files_properties(src/gameplay/weapons/native_weapon_projectile.cpp PROPERTIES COMPILE_OPTIONS
    "$<$<CXX_COMPILER_ID:GNU,Clang>:-fno-fast-math;-ffp-contract=off;-frounding-math>;$<$<CXX_COMPILER_ID:MSVC>:/fp:strict>")
add_executable(weapon_projectile_basis_tests tests/weapon_projectile_basis_tests.cpp)
target_link_libraries(weapon_projectile_basis_tests PRIVATE halo2_systems)
add_test(NAME original_weapon_projectile_basis COMMAND weapon_projectile_basis_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_projectile_basis_original.tsv)
add_executable(weapon_projectile_color_tests tests/weapon_projectile_color_tests.cpp)
target_link_libraries(weapon_projectile_color_tests PRIVATE halo2_systems)
add_test(NAME retail_weapon_projectile_color COMMAND weapon_projectile_color_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_projectile_color_original.tsv)
add_executable(source_grenade_scene_tests tests/source_grenade_scene_tests.cpp)
target_link_libraries(source_grenade_scene_tests PRIVATE halo2_renderer)
add_executable(grenade_presentation_tests tests/grenade_presentation_tests.cpp)
target_link_libraries(grenade_presentation_tests PRIVATE halo2_renderer)
add_test(NAME native_grenade_presentation COMMAND grenade_presentation_tests)
add_executable(world_presentation_tests tests/world_presentation_tests.cpp)
target_link_libraries(world_presentation_tests PRIVATE halo2_systems)
add_test(NAME native_world_presentation COMMAND world_presentation_tests)
add_executable(vehicle_camera_presentation_tests tests/vehicle_camera_presentation_tests.cpp)
target_link_libraries(vehicle_camera_presentation_tests PRIVATE halo2_systems)
add_test(NAME native_vehicle_camera_presentation COMMAND vehicle_camera_presentation_tests)
add_executable(campaign_pose_interpolation_tests tests/campaign_pose_interpolation_tests.cpp)
target_link_libraries(campaign_pose_interpolation_tests PRIVATE halo2_systems)
add_test(NAME native_campaign_pose_interpolation COMMAND campaign_pose_interpolation_tests)
add_executable(campaign_scale_reset_tests tests/campaign_scale_reset_tests.cpp)
target_link_libraries(campaign_scale_reset_tests PRIVATE halo2_cache)
add_test(NAME native_campaign_scale_reset COMMAND campaign_scale_reset_tests)
add_test(NAME native_campaign_scale_reset_assets COMMAND campaign_scale_reset_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map")
add_executable(texture_residency_tests tests/texture_residency_tests.cpp)
target_include_directories(texture_residency_tests PRIVATE include)
target_compile_features(texture_residency_tests PRIVATE cxx_std_20)
target_link_libraries(texture_residency_tests PRIVATE Threads::Threads)
add_test(NAME native_texture_residency COMMAND texture_residency_tests)
add_executable(startup_texture_fingerprints_tests tests/startup_texture_fingerprints_tests.cpp)
target_include_directories(startup_texture_fingerprints_tests PRIVATE src)
target_link_libraries(startup_texture_fingerprints_tests PRIVATE Threads::Threads)
target_compile_features(startup_texture_fingerprints_tests PRIVATE cxx_std_20)
add_test(NAME startup_texture_fingerprints_contract COMMAND startup_texture_fingerprints_tests)
add_executable(renderer_pipeline_cache_tests tests/renderer_pipeline_cache_tests.cpp)
target_include_directories(renderer_pipeline_cache_tests PRIVATE src ${Vulkan_INCLUDE_DIRS})
target_compile_features(renderer_pipeline_cache_tests PRIVATE cxx_std_20)
add_test(NAME renderer_pipeline_cache_contract COMMAND renderer_pipeline_cache_tests)
add_executable(renderer_image_residency_tests tests/renderer_image_residency_tests.cpp)
target_include_directories(renderer_image_residency_tests PRIVATE include)
target_compile_features(renderer_image_residency_tests PRIVATE cxx_std_20)
add_test(NAME native_renderer_image_residency COMMAND renderer_image_residency_tests)
add_executable(signed_texture_residency_tests tests/signed_texture_residency_tests.cpp)
target_link_libraries(signed_texture_residency_tests PRIVATE halo2_cache)
add_test(NAME native_signed_texture_residency COMMAND signed_texture_residency_tests)
add_executable(material_pass_residency_tests tests/material_pass_residency_tests.cpp)
target_include_directories(material_pass_residency_tests PRIVATE include)
target_compile_features(material_pass_residency_tests PRIVATE cxx_std_20)
target_link_libraries(material_pass_residency_tests PRIVATE Threads::Threads)
add_test(NAME native_material_pass_residency COMMAND material_pass_residency_tests)
add_executable(campaign_text_layout_tests tests/campaign_text_layout_tests.cpp)
target_include_directories(campaign_text_layout_tests PRIVATE include)
target_compile_features(campaign_text_layout_tests PRIVATE cxx_std_20)
add_test(NAME native_campaign_text_layout COMMAND campaign_text_layout_tests "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_text_control_tu15.tsv"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_opening_titles_mcc.tsv"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_opening_titles_xbox.tsv")
add_executable(source_model_transparent_order_tests tests/source_model_transparent_order_tests.cpp src/rendering/source_transparent_sort.cpp)
target_include_directories(source_model_transparent_order_tests PRIVATE include)
target_compile_features(source_model_transparent_order_tests PRIVATE cxx_std_20)
add_test(NAME native_model_transparent_order_tu15 COMMAND source_model_transparent_order_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mcc_opening_space_queue.tsv"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mcc_opening_space_units.tsv")
add_executable(bitmap_residency_tests tests/bitmap_residency_tests.cpp)
target_link_libraries(bitmap_residency_tests PRIVATE halo2_cache Threads::Threads)
add_test(NAME native_bitmap_residency COMMAND bitmap_residency_tests "${HALO2_TU15_GAMEPLAY_XBE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map"
    "${CMAKE_CURRENT_BINARY_DIR}/bitmap-residency-report.json")
set_tests_properties(native_bitmap_residency PROPERTIES SKIP_RETURN_CODE 77)
if(UNIX)
    add_test(NAME native_translation_partition_linkage COMMAND "${Python3_EXECUTABLE}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_translation_partitions.py)
endif()
add_executable(resident_object_draw_gpu_tests tests/resident_object_draw_gpu_tests.cpp)
target_link_libraries(resident_object_draw_gpu_tests PRIVATE halo2_renderer)
add_executable(source_prt_owner_gpu_tests tests/source_prt_owner_gpu_tests.cpp)
target_link_libraries(source_prt_owner_gpu_tests PRIVATE halo2_renderer)
add_executable(source_prt_fallback_tu15_tests tests/source_prt_fallback_tu15_tests.cpp)
target_link_libraries(source_prt_fallback_tu15_tests PRIVATE halo2_cache)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/01b_spacestation.map" AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_prt_fallback.original.json")
    add_test(NAME original_tu15_prt_fallback COMMAND source_prt_fallback_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/01b_spacestation.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_prt_fallback.original.json)
endif()
add_executable(multiplayer_death_momentum_tests tests/multiplayer_death_momentum_tests.cpp)
target_link_libraries(multiplayer_death_momentum_tests PRIVATE halo2_systems)
add_executable(multiplayer_death_cycle_tests tests/multiplayer_death_cycle_tests.cpp)
target_link_libraries(multiplayer_death_cycle_tests PRIVATE halo2_systems)
add_test(NAME original_multiplayer_death_cycle COMMAND multiplayer_death_cycle_tests
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
set_tests_properties(original_multiplayer_death_cycle PROPERTIES SKIP_RETURN_CODE 77)
add_executable(multiplayer_kill_trigger_momentum_tests tests/multiplayer_kill_trigger_momentum_tests.cpp)
target_link_libraries(multiplayer_kill_trigger_momentum_tests PRIVATE halo2_systems)
target_sources(halo2_systems PRIVATE src/physics/physics_body_matrix.cpp)
target_sources(halo2_systems PRIVATE src/physics/physics_node_delivery.cpp)
if(MSVC)
    set_source_files_properties(src/physics/physics_node_delivery.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/physics/physics_node_delivery.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
add_executable(physics_node_delivery_tests tests/physics_node_delivery_tests.cpp)
target_link_libraries(physics_node_delivery_tests PRIVATE halo2_systems)
if(BUILD_TESTING)
    add_test(NAME original_physics_node_delivery COMMAND physics_node_delivery_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/physical_nodes_original.tsv)
    set_tests_properties(original_physics_node_delivery PROPERTIES SKIP_RETURN_CODE 77)
endif()
target_sources(halo2_systems PRIVATE src/gameplay/units/death_camera.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/native_death_camera.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/vehicles/native_vehicle_camera.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/vehicles/native_vehicle_camera_continuation.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/units/death_camera.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/death_camera.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_systems PRIVATE src/gameplay/units/death_camera_binding.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/death_camera_lifecycle.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/units/death_camera_lifecycle.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/death_camera_lifecycle.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING)
    add_executable(death_camera_lifecycle_tests tests/death_camera_lifecycle_tests.cpp)
    target_link_libraries(death_camera_lifecycle_tests PRIVATE halo2_systems)
    add_test(NAME original_death_camera_lifecycle COMMAND death_camera_lifecycle_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_camera_lifecycle_original.tsv)
    set_tests_properties(original_death_camera_lifecycle PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_death_camera_attached COMMAND death_camera_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_camera_attached_original.tsv)
    set_tests_properties(original_death_camera_attached PROPERTIES SKIP_RETURN_CODE 77)
endif()
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_body_velocity.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_component_rebuild.cpp src/gameplay/units/biped_ragdoll_runtime.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_runtime_scratch.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_prephysics.cpp src/gameplay/units/biped_phase_runtime.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_postphysics_runtime.cpp src/gameplay/units/biped_postphysics_runtime_generated.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_postphysics_phase.cpp src/gameplay/units/biped_postphysics_phase_generated.cpp)
target_sources(halo2_systems PRIVATE src/physics/collision/source_segment_collision.cpp src/physics/collision/source_segment_collision_generated.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/death_world_scheduler.cpp src/gameplay/units/death_world_scheduler_generated.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/death_world_physics_runtime_generated.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/death_object_phases.cpp src/gameplay/units/death_object_phases_generated.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/death_startup_runtime.cpp src/gameplay/units/death_startup_runtime_generated.cpp src/runtime/source_runtime_host_physical_memory.cpp src/gameplay/units/death_startup_native.cpp)
target_sources(halo2_systems PRIVATE src/runtime/source_runtime_cache_tags.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/multiplayer/multiplayer_object_runtime.cpp)
add_library(halo2_raw_datum_pool_identity STATIC src/gameplay/objects/raw_datum_pool_identity.cpp)
target_include_directories(halo2_raw_datum_pool_identity PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(halo2_raw_datum_pool_identity PUBLIC cxx_std_20)
if(HALO2_SANITIZERS AND NOT MSVC)
    target_compile_options(halo2_raw_datum_pool_identity PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(halo2_raw_datum_pool_identity PUBLIC -fsanitize=address,undefined)
endif()
target_link_libraries(halo2_systems PUBLIC halo2_raw_datum_pool_identity)
target_sources(halo2_systems PRIVATE src/rendering/source_geometry_resource_acquire.cpp)
target_sources(halo2_systems PRIVATE src/physics/source_physical_cache.cpp)
target_sources(halo2_systems PRIVATE src/rendering/source_geometry_resource_retirement.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/units/death_startup_runtime_generated.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/death_startup_runtime_generated.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()

if(MSVC)
    set_source_files_properties(src/gameplay/units/death_object_phases.cpp src/gameplay/units/death_object_phases_generated.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/death_object_phases.cpp src/gameplay/units/death_object_phases_generated.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(MSVC)
    set_source_files_properties(src/gameplay/units/death_world_scheduler.cpp src/gameplay/units/death_world_scheduler_generated.cpp src/gameplay/units/death_world_physics_runtime_generated.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/death_world_scheduler.cpp src/gameplay/units/death_world_scheduler_generated.cpp src/gameplay/units/death_world_physics_runtime_generated.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_systems PRIVATE src/rendering/source_point_collision_runtime.cpp src/rendering/source_point_collision_runtime_generated.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/source_point_collision_runtime.cpp src/rendering/source_point_collision_runtime_generated.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/source_point_collision_runtime.cpp src/rendering/source_point_collision_runtime_generated.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(MSVC)
    set_source_files_properties(src/physics/collision/source_segment_collision.cpp src/physics/collision/source_segment_collision_generated.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/physics/collision/source_segment_collision.cpp src/physics/collision/source_segment_collision_generated.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(MSVC)
    set_source_files_properties(src/gameplay/units/biped_postphysics_runtime.cpp src/gameplay/units/biped_postphysics_runtime_generated.cpp src/gameplay/units/biped_postphysics_phase.cpp src/gameplay/units/biped_postphysics_phase_generated.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/biped_postphysics_runtime.cpp src/gameplay/units/biped_postphysics_runtime_generated.cpp src/gameplay/units/biped_postphysics_phase.cpp src/gameplay/units/biped_postphysics_phase_generated.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_movement.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_physical_step.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/animation/unit_animation_runtime.cpp src/gameplay/animation/unit_animation_channels.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/animation/unit_animation_selection_runtime.cpp src/gameplay/animation/unit_animation_selection_call.cpp)
target_sources(halo2_systems PRIVATE src/runtime/source_runtime_memory.cpp src/rendering/effects/source_attached_effect.cpp)
target_link_libraries(halo2_systems PRIVATE halo2_snapshot_storage)
target_sources(halo2_systems PRIVATE src/gameplay/weapons/unit_pickup_runtime.cpp src/gameplay/units/player_inventory_admission.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/weapons/unit_weapon_routing.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/unit_object_runtime.cpp src/gameplay/units/unit_object_runtime_generated.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/units/unit_object_runtime.cpp src/gameplay/units/unit_object_runtime_generated.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/unit_object_runtime.cpp src/gameplay/units/unit_object_runtime_generated.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(MSVC)
    set_source_files_properties(src/gameplay/weapons/unit_weapon_routing.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/weapons/unit_weapon_routing.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(MSVC)
    set_source_files_properties(src/gameplay/weapons/unit_pickup_runtime.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/weapons/unit_pickup_runtime.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_systems PRIVATE src/runtime/source_runtime_image.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/objects/object_body_heap.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/objects/object_current_nodes.cpp)
if(BUILD_TESTING)
    add_executable(biped_prephysics_tests tests/biped_prephysics_tests.cpp)
    target_link_libraries(biped_prephysics_tests PRIVATE halo2_systems)
    add_test(NAME original_biped_prephysics COMMAND biped_prephysics_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_prephysics_original.tsv)
    set_tests_properties(original_biped_prephysics PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_biped_prephysics_admission COMMAND biped_prephysics_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_prephysics_admission_original.tsv)
    set_tests_properties(original_biped_prephysics_admission PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_biped_prephysics_input COMMAND biped_prephysics_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_prephysics_input_original.tsv)
    set_tests_properties(original_biped_prephysics_input PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_biped_prephysics_producers COMMAND biped_prephysics_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_prephysics_producers_original.tsv)
    set_tests_properties(original_biped_prephysics_producers PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_biped_prephysics_velocity COMMAND biped_prephysics_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_prephysics_velocity_original.tsv)
    set_tests_properties(original_biped_prephysics_velocity PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_biped_prephysics_world COMMAND biped_prephysics_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_prephysics_world_original.tsv)
    add_test(NAME original_biped_prephysics_world_point COMMAND biped_prephysics_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_prephysics_world_original.tsv native-point)
    set_tests_properties(original_biped_prephysics_world original_biped_prephysics_world_point PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(biped_orientation_tests tests/biped_orientation_tests.cpp)
    target_link_libraries(biped_orientation_tests PRIVATE halo2_systems)
    add_test(NAME original_biped_orientation COMMAND biped_orientation_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_orientation_original.tsv)
    set_tests_properties(original_biped_orientation PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(corpse_current_nodes_tests tests/corpse_current_nodes_tests.cpp)
    target_link_libraries(corpse_current_nodes_tests PRIVATE halo2_systems)
    add_test(NAME original_corpse_current_nodes COMMAND corpse_current_nodes_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/corpse_current_nodes_original.tsv)
    set_tests_properties(original_corpse_current_nodes PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_corpse_current_matrices COMMAND corpse_current_nodes_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/corpse_current_matrices_original.tsv)
    set_tests_properties(original_corpse_current_matrices PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(biped_movement_tests tests/biped_movement_tests.cpp)
    target_link_libraries(biped_movement_tests PRIVATE halo2_systems)
    add_test(NAME original_biped_movement COMMAND biped_movement_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_movement_original.tsv)
    set_tests_properties(original_biped_movement PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(biped_movement_helpers_tests tests/biped_movement_helpers_tests.cpp)
    target_link_libraries(biped_movement_helpers_tests PRIVATE halo2_systems)
    add_test(NAME original_biped_movement_helpers COMMAND biped_movement_helpers_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_movement_helpers_original.tsv)
    set_tests_properties(original_biped_movement_helpers PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(biped_physical_step_tests tests/biped_physical_step_tests.cpp)
    target_link_libraries(biped_physical_step_tests PRIVATE halo2_systems)
    add_test(NAME original_biped_physical_step COMMAND biped_physical_step_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_physical_step_original.tsv)
    set_tests_properties(original_biped_physical_step PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(biped_physical_movement_tests tests/biped_physical_movement_tests.cpp)
    target_link_libraries(biped_physical_movement_tests PRIVATE halo2_systems)
    add_test(NAME original_biped_physical_movement COMMAND biped_physical_movement_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_physical_movement_original.tsv)
    set_tests_properties(original_biped_physical_movement PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(biped_dead_step_tests tests/biped_dead_step_tests.cpp)
    target_link_libraries(biped_dead_step_tests PRIVATE halo2_systems)
    add_test(NAME original_biped_dead_step COMMAND biped_dead_step_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_dead_step_original.tsv)
    set_tests_properties(original_biped_dead_step PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(raw_datum_startup_tests tests/raw_datum_startup_tests.cpp)
    target_link_libraries(raw_datum_startup_tests PRIVATE halo2_systems)
    add_test(NAME original_raw_datum_startup COMMAND raw_datum_startup_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/raw_datum_startup_original.tsv)
    add_executable(raw_datum_creation_tests tests/raw_datum_creation_tests.cpp)
    target_link_libraries(raw_datum_creation_tests PRIVATE halo2_systems)
    add_test(NAME original_raw_datum_creation COMMAND raw_datum_creation_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/raw_datum_creation_original.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    set_tests_properties(original_raw_datum_creation PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(biped_component_storage_tests tests/biped_component_storage_tests.cpp)
    target_link_libraries(biped_component_storage_tests PRIVATE halo2_systems)
    add_test(NAME original_biped_component_storage COMMAND biped_component_storage_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_component_storage_original.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    set_tests_properties(original_biped_component_storage PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_main_heap_storage_tests tests/source_main_heap_storage_tests.cpp)
    target_link_libraries(source_main_heap_storage_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_source_main_heap_storage COMMAND source_main_heap_storage_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/main-heap-measured/fixture.json
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/main-heap-hardware-measured/fixture.json)
    set_tests_properties(original_source_main_heap_storage PROPERTIES SKIP_RETURN_CODE 77)

    add_executable(unit_animation_transition_tests tests/unit_animation_transition_tests.cpp)
    target_link_libraries(unit_animation_transition_tests PRIVATE halo2_systems)
    add_test(NAME original_death_animation_transition COMMAND unit_animation_transition_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_animation_transition_original.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)

    add_executable(unit_animation_selection_runtime_tests tests/unit_animation_selection_runtime_tests.cpp)
    target_link_libraries(unit_animation_selection_runtime_tests PRIVATE halo2_systems)
    add_test(NAME original_death_point_containment_runtime COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_point_containment_runtime_original.tsv 16 point)
    add_test(NAME original_death_point_containment_adapter COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_point_containment_runtime_original.tsv 16 point-adapted)
    set_tests_properties(original_death_point_containment_runtime original_death_point_containment_adapter PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_death_world_scheduler_runtime COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_world_scheduler_runtime_original.tsv 13 world)
    add_test(NAME original_death_world_scheduler_adapter COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_world_scheduler_runtime_original.tsv 13 world-adapted)
    set_tests_properties(original_death_world_scheduler_runtime original_death_world_scheduler_adapter PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(death_startup_native_tests tests/death_startup_native_tests.cpp)
    target_link_libraries(death_startup_native_tests PRIVATE halo2_systems OpenSSL::Crypto ZLIB::ZLIB)
    add_test(NAME original_death_startup_native COMMAND death_startup_native_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_startup_runtime_original.json
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    set_tests_properties(original_death_startup_native PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    add_test(NAME original_source_render_cache_startup_native COMMAND death_startup_native_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_render_cache_startup_original.json
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    set_tests_properties(original_source_render_cache_startup_native PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    add_test(NAME original_death_world_startup_native COMMAND death_startup_native_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_cache_startup_runtime_original.json
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
    set_tests_properties(original_death_world_startup_native PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    add_test(NAME original_source_model_resource_startup_native COMMAND death_startup_native_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_model_resource_startup_original.json
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
    set_tests_properties(original_source_model_resource_startup_native PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    add_executable(multiplayer_object_runtime_tests tests/multiplayer_object_runtime_tests.cpp)
    target_link_libraries(multiplayer_object_runtime_tests PRIVATE halo2_systems)
    add_executable(death_startup_divide_tests tests/death_startup_divide_tests.cpp)
    target_link_libraries(death_startup_divide_tests PRIVATE halo2_systems)
    add_test(NAME original_death_startup_unsigned_division COMMAND death_startup_divide_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261012/fatal-rng-research/divide-reference/fixture.json
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    set_tests_properties(original_death_startup_unsigned_division PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    add_test(NAME native_multiplayer_object_world COMMAND multiplayer_object_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
    set_tests_properties(native_multiplayer_object_world PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_test(NAME native_multiplayer_source_species_resources COMMAND multiplayer_object_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map --resources)
    set_tests_properties(native_multiplayer_source_species_resources PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_test(NAME native_multiplayer_source_biped_birth COMMAND multiplayer_object_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map --create)
    add_test(NAME native_multiplayer_source_match_binding COMMAND multiplayer_object_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map --match)
    set_tests_properties(native_multiplayer_source_biped_birth native_multiplayer_source_match_binding
        PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    add_test(NAME original_death_object_phases_runtime COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_object_phases_runtime_original.tsv 65 object-phases)
    set_tests_properties(original_death_object_phases_runtime PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_death_object_phases_continuous COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_object_phases_runtime_original.tsv 65 object-continuous)
    set_tests_properties(original_death_object_phases_continuous PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_death_object_phases_defined_inputs COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_object_phases_runtime_original.tsv 65 object-continuous-adapted)
    set_tests_properties(original_death_object_phases_defined_inputs PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_death_object_phases_cycle COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_object_phases_runtime_original.tsv 65 object-cycle-adapted)
    set_tests_properties(original_death_object_phases_cycle PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_fatal_session_gate_tests tests/source_fatal_session_gate_tests.cpp)
    target_link_libraries(source_fatal_session_gate_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_source_fatal_session_gate COMMAND source_fatal_session_gate_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_fatal_session_gate_original.json)
    set_tests_properties(original_source_fatal_session_gate PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    add_test(NAME original_death_segment_collision_runtime COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_segment_collision_runtime_original.tsv 684 segment)
    add_test(NAME original_death_segment_collision_adapter COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_segment_collision_runtime_original.tsv 684 segment-adapted)
    add_test(NAME original_death_camera_collision_adapter COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_segment_collision_runtime_original.tsv 684 segment-camera-adapted)
    set_tests_properties(original_death_segment_collision_runtime original_death_segment_collision_adapter
        original_death_camera_collision_adapter PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_biped_postphysics_runtime COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_postphysics_runtime_original.tsv 13 postphysics)
    add_test(NAME original_biped_postphysics_adapter COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_postphysics_runtime_original.tsv 13 postphysics-adapted)
    set_tests_properties(original_biped_postphysics_runtime original_biped_postphysics_adapter PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_biped_postphysics_phase_runtime COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_postphysics_phase_runtime_original.tsv 13 postphysics-phase)
    add_test(NAME original_biped_postphysics_phase_adapter COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_postphysics_phase_runtime_original.tsv 13 postphysics-phase-adapted)
    set_tests_properties(original_biped_postphysics_phase_runtime original_biped_postphysics_phase_adapter PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_death_animation_selection_runtime COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_animation_selection_runtime_original.tsv)
    set_tests_properties(original_death_animation_selection_runtime PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_death_animation_pose_runtime COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_animation_pose_runtime_original.tsv 56)
    set_tests_properties(original_death_animation_pose_runtime PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_death_animation_pose_adapters COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_animation_pose_runtime_original.tsv 56 adapted)
    set_tests_properties(original_death_animation_pose_adapters PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_biped_input_runtime COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_input_runtime_original.tsv 13)
    add_test(NAME original_biped_input_adapters COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_input_runtime_original.tsv 13 adapted)
    set_tests_properties(original_biped_input_runtime original_biped_input_adapters PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_biped_phase_runtime COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_phase_runtime_original.tsv 68)
    add_test(NAME original_biped_phase_adapters COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_phase_runtime_original.tsv 68 adapted)
    set_tests_properties(original_biped_phase_runtime original_biped_phase_adapters PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_death_animation_events_runtime COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_animation_events_runtime_original.tsv 4)
    add_test(NAME original_death_animation_events_adapters COMMAND unit_animation_selection_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_animation_events_runtime_original.tsv 4 adapted)
    set_tests_properties(original_death_animation_events_runtime original_death_animation_events_adapters PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_corpse_current_pose COMMAND corpse_current_nodes_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/corpse_current_pose_original.tsv)
    set_tests_properties(original_corpse_current_pose PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_animation_phase_tests tests/unit_animation_phase_tests.cpp)
    target_link_libraries(unit_animation_phase_tests PRIVATE halo2_systems)
    add_test(NAME original_death_animation_phase COMMAND unit_animation_phase_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_animation_phase_original.tsv)

    add_executable(unit_animation_channels_tests tests/unit_animation_channels_tests.cpp)
    target_link_libraries(unit_animation_channels_tests PRIVATE halo2_systems)
    add_test(NAME original_death_animation_channels COMMAND unit_animation_channels_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_animation_channels_original.tsv)

    add_executable(unit_animation_weights_tests tests/unit_animation_weights_tests.cpp)
    target_link_libraries(unit_animation_weights_tests PRIVATE halo2_systems)
    add_test(NAME original_death_animation_weights COMMAND unit_animation_weights_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_animation_weights_original.tsv)

    add_executable(unit_animation_request_tests tests/unit_animation_request_tests.cpp)
    target_link_libraries(unit_animation_request_tests PRIVATE halo2_systems)
    add_test(NAME original_death_animation_request COMMAND unit_animation_request_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_animation_request_original.tsv)
    set_tests_properties(original_death_animation_request PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(campaign_lens_restore_memory_tu15_tests tests/campaign_lens_restore_memory_tu15_tests.cpp src/runtime/source_runtime_memory.cpp)
    target_include_directories(campaign_lens_restore_memory_tu15_tests PRIVATE include)
    target_link_libraries(campaign_lens_restore_memory_tu15_tests PRIVATE halo2_snapshot_storage Threads::Threads)
    add_test(NAME campaign_lens_restore_memory_tu15 COMMAND campaign_lens_restore_memory_tu15_tests)
    add_executable(campaign_lens_restore_runtime_tu15_tests tests/campaign_lens_restore_runtime_tu15_tests.cpp)
    target_link_libraries(campaign_lens_restore_runtime_tu15_tests PRIVATE halo2_systems)
    add_executable(source_runtime_memory_tests tests/source_runtime_memory_tests.cpp)
    target_link_libraries(source_runtime_memory_tests PRIVATE halo2_systems)
    add_test(NAME source_runtime_retained_memory COMMAND source_runtime_memory_tests)
    add_executable(native_parallel_tests tests/native_parallel_tests.cpp src/runtime/native_parallel.cpp)
    target_include_directories(native_parallel_tests PRIVATE include)
    target_link_libraries(native_parallel_tests PRIVATE Threads::Threads)
    set_source_files_properties(tests/native_parallel_tests.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    add_test(NAME native_parallel_fork_join COMMAND native_parallel_tests)
    add_executable(source_runtime_immutable_tests tests/source_runtime_immutable_tests.cpp src/runtime/source_runtime_memory.cpp)
    add_executable(native_tu15_physics_branch_ownership_tests
        tests/native_tu15_physics_branch_ownership_tests.cpp
        src/physics/native_tu15_physics_branch_data.cpp src/runtime/source_runtime_memory.cpp)
    target_include_directories(native_tu15_physics_branch_ownership_tests PRIVATE include)
    target_link_libraries(native_tu15_physics_branch_ownership_tests PRIVATE halo2_snapshot_storage Threads::Threads)
    add_test(NAME native_tu15_physics_branch_ownership COMMAND native_tu15_physics_branch_ownership_tests)
    add_executable(native_tu15_ctf_static_data_tests
        tests/native_tu15_ctf_static_data_tests.cpp
        src/gameplay/multiplayer/native_tu15_ctf_static_data.cpp src/runtime/source_runtime_memory.cpp)
    target_include_directories(native_tu15_ctf_static_data_tests PRIVATE include)
    target_link_libraries(native_tu15_ctf_static_data_tests PRIVATE
        halo2_snapshot_storage Threads::Threads nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_ctf_static_data COMMAND native_tu15_ctf_static_data_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/ctf_static_original_tu15.json)
    add_executable(native_tu15_player_result_data_tests
        tests/native_tu15_player_result_data_tests.cpp
        src/gameplay/units/native_tu15_player_result_data.cpp src/runtime/source_runtime_memory.cpp)
    target_include_directories(native_tu15_player_result_data_tests PRIVATE include)
    target_link_libraries(native_tu15_player_result_data_tests PRIVATE
        halo2_snapshot_storage Threads::Threads nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_player_result_data COMMAND native_tu15_player_result_data_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/player_result_original_tu15.json)
    target_include_directories(source_runtime_immutable_tests PRIVATE include)
    target_link_libraries(source_runtime_immutable_tests PRIVATE halo2_snapshot_storage Threads::Threads)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_test(NAME source_runtime_immutable_comparison COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
            --output-root "@HALO2_TEST_SCRATCH@/memory-budget/source-immutable-comparison" --timeout 60
            -- $<TARGET_FILE:source_runtime_immutable_tests>)
        set_tests_properties(source_runtime_immutable_comparison PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
    endif()
    add_executable(source_runtime_snapshot_memory_tests tests/source_runtime_snapshot_memory_tests.cpp
        src/runtime/source_runtime_memory.cpp src/runtime/source_runtime_host_physical_memory.cpp)
    target_include_directories(source_runtime_snapshot_memory_tests PRIVATE include)
    target_link_libraries(source_runtime_snapshot_memory_tests PRIVATE halo2_snapshot_storage Threads::Threads)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        foreach(snapshot_case checkpoint restore copy contracts storage adoption)
            add_test(NAME source_runtime_snapshot_${snapshot_case} COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/memory-budget/source-snapshot-${snapshot_case}" --timeout 180
                -- $<TARGET_FILE:source_runtime_snapshot_memory_tests> ${snapshot_case})
            set_tests_properties(source_runtime_snapshot_${snapshot_case} PROPERTIES RUN_SERIAL TRUE TIMEOUT 210)
        endforeach()
    endif()
    add_executable(source_runtime_shared_backing_tests tests/source_runtime_shared_backing_tests.cpp src/runtime/source_runtime_memory.cpp)
    target_link_libraries(source_runtime_shared_backing_tests PRIVATE halo2_snapshot_storage)
    target_include_directories(source_runtime_shared_backing_tests PRIVATE include)
    target_link_libraries(source_runtime_shared_backing_tests PRIVATE Threads::Threads)
    add_executable(source_runtime_policy_hold_tests tests/source_runtime_policy_hold_tests.cpp
        src/runtime/source_runtime_memory.cpp src/runtime/source_execution_inputs.cpp)
    target_include_directories(source_runtime_policy_hold_tests PRIVATE include)
    target_link_libraries(source_runtime_policy_hold_tests PRIVATE halo2_snapshot_storage Threads::Threads)
    add_test(NAME source_runtime_policy_publication COMMAND source_runtime_policy_hold_tests)
    set_tests_properties(source_runtime_policy_publication PROPERTIES TIMEOUT 20)
    add_executable(source_runtime_write_dispatch_tests tests/source_runtime_write_dispatch_tests.cpp
        src/runtime/source_runtime_memory.cpp)
    target_include_directories(source_runtime_write_dispatch_tests PRIVATE include)
    target_link_libraries(source_runtime_write_dispatch_tests PRIVATE halo2_snapshot_storage Threads::Threads)
    add_test(NAME source_runtime_write_dispatch COMMAND source_runtime_write_dispatch_tests)
    add_test(NAME source_runtime_write_dispatch_reference COMMAND source_runtime_write_dispatch_tests)
    set_tests_properties(source_runtime_write_dispatch_reference PROPERTIES ENVIRONMENT "HALO2_REFERENCE_WRITE_DISPATCH=1")
    add_executable(texture_pixel_fingerprint_tests tests/texture_pixel_fingerprint_tests.cpp)
    target_include_directories(texture_pixel_fingerprint_tests PRIVATE include src)
    target_link_libraries(texture_pixel_fingerprint_tests PRIVATE Threads::Threads)
    add_test(NAME native_texture_pixel_fingerprints COMMAND texture_pixel_fingerprint_tests)
    add_executable(source_physical_backing_tests tests/source_physical_backing_tests.cpp
        src/runtime/source_runtime_memory.cpp src/runtime/source_runtime_host_physical_memory.cpp)
    target_link_libraries(source_physical_backing_tests PRIVATE halo2_snapshot_storage)
    target_include_directories(source_physical_backing_tests PRIVATE include)
    target_link_libraries(source_physical_backing_tests PRIVATE Threads::Threads)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_test(NAME source_runtime_shared_backing COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
            --output-root "@HALO2_TEST_SCRATCH@/memory-budget/source-shared-backing" --timeout 60
            -- $<TARGET_FILE:source_runtime_shared_backing_tests>)
        set_tests_properties(source_runtime_shared_backing PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
        foreach(physical_case large protection)
            add_test(NAME source_physical_backing_${physical_case} COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/memory-budget/source-physical-${physical_case}" --timeout 60
                -- $<TARGET_FILE:source_physical_backing_tests> ${physical_case})
            set_tests_properties(source_physical_backing_${physical_case} PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
        endforeach()
    else()
        add_test(NAME source_runtime_shared_backing COMMAND source_runtime_shared_backing_tests)
        set_tests_properties(source_runtime_shared_backing PROPERTIES RUN_SERIAL TRUE)
    endif()
    add_executable(source_runtime_transfer_tests tests/source_runtime_transfer_tests.cpp src/runtime/source_runtime_memory.cpp)
    target_link_libraries(source_runtime_transfer_tests PRIVATE halo2_snapshot_storage)
    target_include_directories(source_runtime_transfer_tests PRIVATE include)
    target_link_libraries(source_runtime_transfer_tests PRIVATE Threads::Threads)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        foreach(transfer_case read write boundaries)
            add_test(NAME source_runtime_transfer_${transfer_case} COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/memory-budget/source-transfer-${transfer_case}" --timeout 60
                -- $<TARGET_FILE:source_runtime_transfer_tests> ${transfer_case})
            set_tests_properties(source_runtime_transfer_${transfer_case} PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
        endforeach()
    else()
        add_test(NAME source_runtime_transfer_boundaries COMMAND source_runtime_transfer_tests boundaries)
        set_tests_properties(source_runtime_transfer_boundaries PROPERTIES RUN_SERIAL TRUE)
    endif()
    add_executable(source_runtime_host_virtual_memory_tests tests/source_runtime_host_virtual_memory_tests.cpp
        src/runtime/source_runtime_host_virtual_memory.cpp src/runtime/source_runtime_memory.cpp)
    target_link_libraries(source_runtime_host_virtual_memory_tests PRIVATE halo2_snapshot_storage)
    target_include_directories(source_runtime_host_virtual_memory_tests PRIVATE include)
    add_test(NAME source_runtime_host_virtual_memory COMMAND source_runtime_host_virtual_memory_tests)
    add_executable(source_host_resource_continuation_tests tests/source_host_resource_continuation_tests.cpp
        src/runtime/source_runtime_memory.cpp src/runtime/source_runtime_host_virtual_memory.cpp
        src/runtime/source_runtime_host_physical_memory.cpp src/gameplay/objects/object_body_heap.cpp
        src/runtime/source_runtime_host_storage.cpp src/physics/physics_pool_host_storage.cpp)
    target_link_libraries(source_host_resource_continuation_tests PRIVATE halo2_snapshot_storage)
    target_include_directories(source_host_resource_continuation_tests PRIVATE include)
    add_test(NAME source_host_resource_continuation COMMAND source_host_resource_continuation_tests)
    set_tests_properties(source_host_resource_continuation PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_runtime_image_tests tests/source_runtime_image_tests.cpp)
    target_link_libraries(source_runtime_image_tests PRIVATE halo2_systems)
    add_test(NAME original_runtime_image COMMAND source_runtime_image_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    set_tests_properties(original_runtime_image PROPERTIES SKIP_RETURN_CODE 77)
endif()
if(MSVC)
    set_source_files_properties(src/gameplay/units/biped_prephysics.cpp src/gameplay/units/biped_movement.cpp src/gameplay/units/biped_physical_step.cpp src/gameplay/animation/unit_animation_runtime.cpp src/gameplay/animation/unit_animation_channels.cpp src/gameplay/animation/unit_animation_selection_runtime.cpp src/gameplay/animation/unit_animation_selection_call.cpp src/gameplay/objects/object_current_nodes.cpp src/gameplay/objects/item_transform_math.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/biped_prephysics.cpp src/gameplay/units/biped_movement.cpp src/gameplay/units/biped_physical_step.cpp src/gameplay/animation/unit_animation_runtime.cpp src/gameplay/animation/unit_animation_channels.cpp src/gameplay/animation/unit_animation_selection_runtime.cpp src/gameplay/animation/unit_animation_selection_call.cpp src/gameplay/objects/object_current_nodes.cpp src/gameplay/objects/item_transform_math.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_controller.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_world_removal.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_broadphase_removal.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_contact_retirement.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_world_insertion.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_pair_agent.cpp)
target_sources(halo2_systems PRIVATE src/physics/physics_constraint_retirement.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/units/biped_controller.cpp src/gameplay/units/biped_body_velocity.cpp src/gameplay/units/biped_world_insertion.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/biped_controller.cpp src/gameplay/units/biped_body_velocity.cpp src/gameplay/units/biped_world_insertion.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING)
    add_test(NAME original_biped_controller COMMAND biped_admission_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_controller_original.tsv)
    set_tests_properties(original_biped_controller PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_biped_world_removal COMMAND biped_admission_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_world_removal_original.tsv)
    set_tests_properties(original_biped_world_removal PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_biped_world_insertion COMMAND biped_admission_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_world_insertion_original.tsv)
    set_tests_properties(original_biped_world_insertion PROPERTIES SKIP_RETURN_CODE 77)
endif()
if(BUILD_TESTING)
    add_test(NAME original_natural_corpse_nodes COMMAND physics_node_delivery_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/natural_corpse_nodes_original.tsv)
    set_tests_properties(original_natural_corpse_nodes PROPERTIES SKIP_RETURN_CODE 77)
endif()
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_physics_rebuild.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/camera/observer_motion.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/camera/observer_world.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/units/death_camera.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/gameplay/camera/observer_motion.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
    set_source_files_properties(src/gameplay/camera/observer_world.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/death_camera.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
    set_source_files_properties(src/gameplay/camera/observer_motion.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
    set_source_files_properties(src/gameplay/camera/observer_world.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
add_executable(death_camera_tests tests/death_camera_tests.cpp)
target_link_libraries(death_camera_tests PRIVATE halo2_systems)
add_executable(biped_physics_rebuild_tests tests/biped_physics_rebuild_tests.cpp)
target_link_libraries(biped_physics_rebuild_tests PRIVATE halo2_systems)
if(BUILD_TESTING)
    add_test(NAME original_biped_physics_rebuild COMMAND biped_physics_rebuild_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_physics_rebuild_original.tsv)
    set_tests_properties(original_biped_physics_rebuild PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_death_camera COMMAND death_camera_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_camera_original.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_observer_branches_original.tsv)
    set_tests_properties(original_death_camera PROPERTIES SKIP_RETURN_CODE 77)
endif()
target_sources(halo2_systems PRIVATE src/physics/source_lift_scalar.cpp)
target_sources(halo2_systems PRIVATE src/physics/source_lift_membership.cpp)
add_executable(source_lift_membership_tests tests/source_lift_membership_tests.cpp)
target_link_libraries(source_lift_membership_tests PRIVATE halo2_systems)
if(BUILD_TESTING)
    add_test(NAME original_source_lift_membership COMMAND source_lift_membership_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/lift-membership-20261010/measured-v1/fixture.json)
    set_tests_properties(original_source_lift_membership PROPERTIES SKIP_RETURN_CODE 77)
endif()
target_sources(halo2_systems PRIVATE src/gameplay/world/source_teleporter.cpp src/physics/source_lift_contact.cpp src/gameplay/vehicles/source_vehicle_construction.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/world/source_teleporter.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/world/source_teleporter.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
add_executable(source_teleporter_tests tests/source_teleporter_tests.cpp)
target_link_libraries(source_teleporter_tests PRIVATE halo2_systems)
add_executable(source_lift_contact_tests tests/source_lift_contact_tests.cpp)
target_link_libraries(source_lift_contact_tests PRIVATE halo2_systems)
add_executable(source_vehicle_construction_tests tests/source_vehicle_construction_tests.cpp)
target_link_libraries(source_vehicle_construction_tests PRIVATE halo2_systems)
# Historical host-only traversal diagnostic. The MCC tests below exercise
# the complete TU1.5 world/query/pose/event/view owners in normal match ticks.
add_executable(teleporter_match_tests EXCLUDE_FROM_ALL tests/teleporter_match_tests.cpp)
target_link_libraries(teleporter_match_tests PRIVATE halo2_systems)
add_executable(teleporter_mcc_tu15_tests EXCLUDE_FROM_ALL tests/teleporter_mcc_tu15_tests.cpp)
target_link_libraries(teleporter_mcc_tu15_tests PRIVATE halo2_systems halo2_authority_codec)
add_executable(relic_teleporter_gate_tu15_tests EXCLUDE_FROM_ALL tests/relic_teleporter_gate_tu15_tests.cpp)
target_link_libraries(relic_teleporter_gate_tu15_tests PRIVATE halo2_systems)
add_executable(teleporter_blocked_notice_tu15_tests EXCLUDE_FROM_ALL tests/teleporter_blocked_notice_tu15_tests.cpp)
target_link_libraries(teleporter_blocked_notice_tu15_tests PRIVATE halo2_systems)
add_executable(source_hud_messages_tu15_tests EXCLUDE_FROM_ALL tests/source_hud_messages_tu15_tests.cpp)
target_link_libraries(source_hud_messages_tu15_tests PRIVATE halo2_systems)
add_executable(mcc_teleporter_gameplay_tests tests/mcc_teleporter_gameplay_tests.cpp)
target_link_libraries(mcc_teleporter_gameplay_tests PRIVATE halo2_cache)
if(BUILD_TESTING)
    set(HALO2_MCC_TELEPORTER_MAP "" CACHE FILEPATH "Prepared MCC map for live TU1.5 teleporter acceptance")
    add_test(NAME mcc_teleporter_original_authoring COMMAND mcc_teleporter_gameplay_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mcc_teleporter_gameplay.json")
    add_test(NAME native_mcc_teleporter_traversal COMMAND teleporter_mcc_tu15_tests "${HALO2_MCC_TELEPORTER_MAP}")
    add_test(NAME native_mcc_teleporter_blocked_notice COMMAND teleporter_blocked_notice_tu15_tests "${HALO2_MCC_TELEPORTER_MAP}")
    add_test(NAME original_tu15_hud_messages COMMAND source_hud_messages_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hud_messages_tu15.original.json")
    set_tests_properties(native_mcc_teleporter_blocked_notice PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    set_tests_properties(native_mcc_teleporter_traversal PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME native_mcc_teleporter_approach COMMAND teleporter_mcc_tu15_tests "${HALO2_MCC_TELEPORTER_MAP}")
    set_tests_properties(native_mcc_teleporter_approach PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180
        ENVIRONMENT "HALO2_TELEPORTER_APPROACH=1")
    set(HALO2_MCC_RELIC_MAP "" CACHE FILEPATH "Prepared MCC Relic map for original switch/teleporter acceptance")
    add_test(NAME native_mcc_relic_teleporter_gate COMMAND relic_teleporter_gate_tu15_tests "${HALO2_MCC_RELIC_MAP}")
    set_tests_properties(native_mcc_relic_teleporter_gate PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
endif()
add_executable(lockout_lift_match_tests EXCLUDE_FROM_ALL tests/lockout_lift_match_tests.cpp)
target_link_libraries(lockout_lift_match_tests PRIVATE halo2_systems)
if(BUILD_TESTING)
    add_test(NAME original_source_teleporter_control COMMAND source_teleporter_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/teleporter-control-20261010/measured-v4/fixture.json)
    add_test(NAME original_source_lift_contact COMMAND source_lift_contact_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/lift-contact-20261010/measured-v1/fixture.txt)
    add_test(NAME original_source_vehicle_construction COMMAND source_vehicle_construction_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/vehicle-construction-20261010/measured-v1/fixture.json)
    set_tests_properties(original_source_teleporter_control original_source_lift_contact original_source_vehicle_construction
        PROPERTIES SKIP_RETURN_CODE 77)
endif()
target_sources(halo2_systems PRIVATE src/gameplay/weapons/projectile_material_response.cpp)
target_sources(halo2_systems PRIVATE src/rendering/models/source_model_effect_lights.cpp)
if(BUILD_TESTING)
    add_executable(campaign_environment_material_tests tests/campaign_environment_material_tests.cpp)
    target_link_libraries(campaign_environment_material_tests PRIVATE halo2_cache)
    add_test(NAME original_campaign_environment_materials COMMAND campaign_environment_material_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/01b_spacestation.map)
    set_tests_properties(original_campaign_environment_materials PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_skinned_geometry_tests tests/source_skinned_geometry_tests.cpp)
    target_link_libraries(source_skinned_geometry_tests PRIVATE halo2_cache)
    add_test(NAME original_tu15_skinned_geometry COMMAND source_skinned_geometry_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_skinned_vertex_declarations.original.json)
    add_test(NAME original_tu15_campaign_skinned_geometry COMMAND source_skinned_geometry_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/01b_spacestation.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_skinned_vertex_declarations.original.json
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_cairo_model_compression.original.json)
    set_tests_properties(original_tu15_skinned_geometry original_tu15_campaign_skinned_geometry
        PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_model_effect_light_tests tests/source_model_effect_light_tests.cpp)
    target_link_libraries(source_model_effect_light_tests PRIVATE halo2_cache)
    add_test(NAME original_tu15_model_effect_light_ranges COMMAND source_model_effect_light_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_full_light_selection.original.json
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_full_light_ranges.original.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_full_light_commands.original.json)
    set_tests_properties(original_tu15_model_effect_light_ranges PROPERTIES SKIP_RETURN_CODE 77)
endif()
target_sources(halo2_cache PRIVATE src/physics/collision/static_scenery_collision.cpp)
if(MSVC)
    set_source_files_properties(src/physics/collision/static_scenery_collision.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/physics/collision/static_scenery_collision.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
add_executable(static_scenery_collision_tests tests/static_scenery_collision_tests.cpp)
target_link_libraries(static_scenery_collision_tests PRIVATE halo2_systems)
add_executable(particle_movement_alias_tests tests/particle_movement_alias_tests.cpp)
target_link_libraries(particle_movement_alias_tests PRIVATE halo2_systems)
if(MSVC)
    set_source_files_properties(src/gameplay/weapons/projectile_material_response.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/weapons/projectile_material_response.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
add_executable(projectile_material_response_tests tests/projectile_material_response_tests.cpp)
target_link_libraries(projectile_material_response_tests PRIVATE halo2_systems)
add_executable(plasma_grenade_lifecycle_tests tests/plasma_grenade_lifecycle_tests.cpp)
target_link_libraries(plasma_grenade_lifecycle_tests PRIVATE halo2_systems SDL2::SDL2)
if(MSVC)
    set_source_files_properties(src/physics/source_lift_scalar.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/physics/source_lift_scalar.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
add_executable(source_lift_scalar_tests tests/source_lift_scalar_tests.cpp)
target_link_libraries(source_lift_scalar_tests PRIVATE halo2_systems)
add_executable(physics_body_matrix_tests tests/physics_body_matrix_tests.cpp)
target_link_libraries(physics_body_matrix_tests PRIVATE halo2_systems)
add_executable(particle_movement_tests tests/particle_movement_tests.cpp)
target_link_libraries(particle_movement_tests PRIVATE halo2_cache)
if(BUILD_TESTING)
    add_test(NAME original_static_scenery_collision COMMAND static_scenery_collision_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/static-scenery-collision-20261010/cold-nodes.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    set_tests_properties(original_static_scenery_collision PROPERTIES SKIP_RETURN_CODE 77
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME authored_particle_movement_alias COMMAND particle_movement_alias_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/midship.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    set_tests_properties(authored_particle_movement_alias PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_projectile_material_response COMMAND projectile_material_response_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/plasma-contact-20261010/measured-v2/materials.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/plasma-contact-20261010/measured-v2/choices.bin)
    set_tests_properties(original_projectile_material_response PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_plasma_grenade_lifecycle COMMAND plasma_grenade_lifecycle_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/midship.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    set_tests_properties(original_plasma_grenade_lifecycle PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_source_lift_scalar COMMAND source_lift_scalar_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/lift-scalar-native-20261010/measured-v2/fixture.txt)
    set_tests_properties(original_source_lift_scalar PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_physics_body_matrix COMMAND physics_body_matrix_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spartan-body-matrix-20261010/fixture.txt)
    set_tests_properties(original_physics_body_matrix PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_multiplayer_death_momentum COMMAND multiplayer_death_momentum_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
    set_tests_properties(original_multiplayer_death_momentum PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_multiplayer_kill_trigger_momentum COMMAND multiplayer_kill_trigger_momentum_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    set_tests_properties(original_multiplayer_kill_trigger_momentum PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_particle_physics_movement COMMAND particle_movement_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/particle-physics-movement-20261010/measured-v2/fixtures.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/particle-physics-movement-20261010/measured-v2/context.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/particle-physics-movement-20261010/measured-v2/stores.bin)
    set_tests_properties(original_particle_physics_movement PROPERTIES SKIP_RETURN_CODE 77)
endif()
target_link_libraries(halo2_renderer PUBLIC halo2_shadow_render halo2_menu_world_storage)
target_sources(halo2_renderer PRIVATE src/gameplay/objects/source_machine_scene.cpp)
target_sources(halo2_renderer PRIVATE src/gameplay/objects/source_prop_scene.cpp src/gameplay/objects/source_object_geometry.cpp)
target_sources(halo2_renderer PRIVATE src/gameplay/objects/source_dynamic_object_scene.cpp)
target_sources(halo2_renderer PRIVATE src/gameplay/objects/source_scenery_scene.cpp)
target_sources(halo2_renderer PRIVATE src/gameplay/weapons/source_netgame_weapon_scene.cpp)
target_link_libraries(halo2_renderer PUBLIC halo2_systems Vulkan::Vulkan)
include(cmake/Upscaling.cmake)
if(TARGET menu_world_vertex_catalog)
    target_sources(halo2_renderer PRIVATE src/ui/menu_world_native_draw.cpp src/ui/menu_world_native_packet.cpp)
    target_include_directories(halo2_renderer PRIVATE ${MENU_WORLD_VERTEX_CATALOG_DIR})
    target_compile_definitions(halo2_renderer PRIVATE HALO2_MENU_WORLD_NATIVE_AVAILABLE=1)
    add_dependencies(halo2_renderer menu_world_vertex_catalog menu_world_geometry_shader menu_world_fragment_shader)
endif()
if(TARGET halo2_particle_renderer)
    target_link_libraries(halo2_renderer PRIVATE halo2_particle_renderer)
    target_compile_definitions(halo2_renderer PRIVATE HALO2_PARTICLE_GRAPHICS_AVAILABLE=1)
endif()
target_sources(halo2_renderer PRIVATE src/rendering/renderer/renderer_shader_directory.cpp)
# Only these two owners read the fallback path. Keep runtime installation
# prefixes out of the other renderer compilation units so a qualified retry
# does not rebuild unrelated rendering code for each new runtime directory.
set_property(SOURCE src/rendering/renderer/renderer_shader_directory.cpp
    src/ui/menu_world_native_draw.cpp APPEND PROPERTY COMPILE_DEFINITIONS
    HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
target_compile_options(halo2_renderer PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wno-missing-field-initializers>)
add_dependencies(halo2_renderer halo2_shaders)
add_executable(halo2_native src/app/main.cpp src/ui/frontend.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
if(TARGET halo2_mcc_loading_audio)
    add_dependencies(halo2_native halo2_mcc_loading_audio)
endif()
include(cmake/Movies.cmake)
include(cmake/RuntimeTagData.cmake)
halo2_stage_tag_data(halo2_native "${CMAKE_CURRENT_SOURCE_DIR}/data/tags")
install(DIRECTORY data/tags DESTINATION data)
target_sources(halo2_renderer PRIVATE src/ui/game_pause_menu.cpp)
target_sources(halo2_renderer PRIVATE src/campaign/campaign_text.cpp)
add_executable(controller_settings_tests tests/controller_settings_tests.cpp)
target_link_libraries(controller_settings_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
add_executable(game_pause_menu_tests tests/game_pause_menu_tests.cpp)
target_link_libraries(game_pause_menu_tests PRIVATE halo2_renderer)
halo2_bundle_discord_social(halo2_native)
halo2_bundle_eos(halo2_native)
target_link_libraries(halo2_native PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_include_directories(halo2_native PRIVATE "${HALO2_ONLINE_BRIDGE_INCLUDE}")
add_dependencies(halo2_native halo2_online_protocol_rust_build)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(halo2_native PRIVATE src/rendering/renderer/native_vulkan_pipeline_cache.cpp)
    target_link_options(halo2_native PRIVATE
        "-Wl,--wrap=vkCreateDevice" "-Wl,--wrap=vkDestroyDevice"
        "-Wl,--wrap=vkCreateGraphicsPipelines" "-Wl,--wrap=vkCreateComputePipelines")
endif()
target_compile_definitions(halo2_native PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(halo2_native halo2_shaders)
if(APPLE)
    halo2_macos_engine_bundle(halo2_native)
    if(BUILD_TESTING)
        include(cmake/macos/EngineTests.cmake)
        halo2_macos_engine_tests(halo2_native)
    endif()
endif()
add_executable(multiplayer_kbm_runtime_tests tests/multiplayer_kbm_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(multiplayer_kbm_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(multiplayer_kbm_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(multiplayer_kbm_runtime_tests halo2_shaders)
add_executable(multiplayer_species_runtime_tests tests/multiplayer_species_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(multiplayer_species_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(multiplayer_species_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(multiplayer_species_runtime_tests halo2_shaders)
add_executable(multiplayer_scope_runtime_tests tests/multiplayer_scope_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(multiplayer_scope_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(multiplayer_scope_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(multiplayer_scope_runtime_tests halo2_shaders)
add_executable(sniper_mouse_runtime_tests tests/sniper_mouse_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(sniper_mouse_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(sniper_mouse_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(sniper_mouse_runtime_tests halo2_shaders)
set(HALO2_SNIPER_MCC_MAPS "" CACHE PATH "Prepared MCC maps for live sniper mouse control checks")
find_program(HALO2_SNIPER_XVFB_RUN xvfb-run)
foreach(sniper sniper_rifle beam_rifle)
    foreach(fov 78 120)
        if(HALO2_SNIPER_XVFB_RUN)
            add_test(NAME native_${sniper}_mouse_${fov} COMMAND "${HALO2_SNIPER_XVFB_RUN}" -a
                $<TARGET_FILE:sniper_mouse_runtime_tests> "${HALO2_SNIPER_MCC_MAPS}" ${sniper} ${fov} 854)
        else()
            add_test(NAME native_${sniper}_mouse_${fov} COMMAND sniper_mouse_runtime_tests
                "${HALO2_SNIPER_MCC_MAPS}" ${sniper} ${fov} 854)
        endif()
        set_tests_properties(native_${sniper}_mouse_${fov} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600 RUN_SERIAL TRUE)
    endforeach()
endforeach()
add_executable(gameplay_fov_runtime_tests tests/gameplay_fov_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(gameplay_fov_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(gameplay_fov_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(gameplay_fov_runtime_tests halo2_shaders)
add_executable(two_player_split_runtime_tests tests/two_player_split_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(two_player_split_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(two_player_split_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(two_player_split_runtime_tests halo2_shaders)
add_executable(first_person_fov_render_tests tests/first_person_fov_render_tests.cpp)
target_link_libraries(first_person_fov_render_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
add_dependencies(first_person_fov_render_tests halo2_shaders)
set(HALO2_FIRST_PERSON_FOV_MCC_MAPS "" CACHE PATH "Prepared MCC maps for first-person FOV framebuffer regressions")
foreach(weapon IN ITEMS smg rocket_launcher)
    add_test(NAME first_person_fov_${weapon} COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_first_person_fov_render_test.py
        $<TARGET_FILE:first_person_fov_render_tests> "${HALO2_FIRST_PERSON_FOV_MCC_MAPS}" ${weapon})
    set_tests_properties(first_person_fov_${weapon} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 90)
endforeach()
add_executable(multiplayer_scoreboard_runtime_tests tests/multiplayer_scoreboard_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
add_executable(source_shield_live_runtime_tests tests/source_shield_live_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(source_shield_live_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(source_shield_live_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(source_shield_live_runtime_tests halo2_shaders)
add_executable(equipment_live_render_tests tests/equipment_live_render_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(equipment_live_render_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(equipment_live_render_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(equipment_live_render_tests halo2_shaders)
add_executable(source_shield_cycle_runtime_tests tests/source_shield_cycle_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(source_shield_cycle_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(source_shield_cycle_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(source_shield_cycle_runtime_tests halo2_shaders)
target_compile_definitions(source_shield_live_runtime_tests PRIVATE HALO2_SHIELD_MATERIAL_TRACE)
add_executable(player_armor_emblem_runtime_tests tests/player_armor_emblem_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(player_armor_emblem_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(player_armor_emblem_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(player_armor_emblem_runtime_tests halo2_shaders)
target_link_libraries(multiplayer_scoreboard_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(multiplayer_scoreboard_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(multiplayer_scoreboard_runtime_tests halo2_shaders)
add_executable(scoreboard_vista_gui_tests tests/scoreboard_vista_gui_tests.cpp)
target_link_libraries(scoreboard_vista_gui_tests PRIVATE halo2_systems)
add_executable(multiplayer_death_scoreboard_runtime_tests tests/multiplayer_death_scoreboard_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(multiplayer_death_scoreboard_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(multiplayer_death_scoreboard_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(multiplayer_death_scoreboard_runtime_tests halo2_shaders)
add_executable(multiplayer_animation_runtime_tests tests/multiplayer_animation_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
add_executable(multiplayer_muzzle_runtime_tests tests/multiplayer_muzzle_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
add_executable(muzzle_effects_tests tests/muzzle_effects_tests.cpp)
target_link_libraries(muzzle_effects_tests PRIVATE halo2_systems)
add_executable(muzzle_movement_metadata_tests tests/muzzle_movement_metadata_tests.cpp)
target_link_libraries(muzzle_movement_metadata_tests PRIVATE halo2_systems)
target_sources(halo2_systems PRIVATE src/rendering/effects/effect_sound.cpp)
target_link_libraries(multiplayer_muzzle_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(multiplayer_muzzle_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(multiplayer_muzzle_runtime_tests halo2_shaders)
target_link_libraries(multiplayer_animation_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(multiplayer_animation_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(multiplayer_animation_runtime_tests halo2_shaders)
add_executable(third_person_actions_runtime_tests tests/third_person_actions_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(third_person_actions_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(third_person_actions_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(third_person_actions_runtime_tests halo2_shaders)
add_executable(multiplayer_world_audio_runtime_tests tests/multiplayer_world_audio_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(multiplayer_world_audio_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(multiplayer_world_audio_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(multiplayer_world_audio_runtime_tests halo2_shaders)
add_executable(multiplayer_audio_runtime_tests tests/multiplayer_audio_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(multiplayer_audio_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(multiplayer_audio_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(multiplayer_audio_runtime_tests halo2_shaders)
add_executable(multiplayer_audio_late_join_tests tests/multiplayer_audio_late_join_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(multiplayer_audio_late_join_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(multiplayer_audio_late_join_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(multiplayer_audio_late_join_tests halo2_shaders)
add_executable(multiplayer_prop_audio_runtime_tests tests/multiplayer_prop_audio_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(multiplayer_prop_audio_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(multiplayer_prop_audio_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(multiplayer_prop_audio_runtime_tests halo2_shaders)
add_executable(multiplayer_grenade_runtime_tests tests/multiplayer_grenade_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(multiplayer_grenade_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(multiplayer_grenade_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(multiplayer_grenade_runtime_tests halo2_shaders)
add_executable(compare_mp_bsp_runtime tools/compare_mp_bsp_runtime.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(compare_mp_bsp_runtime PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(compare_mp_bsp_runtime PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(compare_mp_bsp_runtime halo2_shaders)
add_executable(halo2_launcher src/app/launcher.cpp)
add_executable(map_script_runtime_probe tools/map_script_runtime_probe.cpp)
target_link_libraries(map_script_runtime_probe PRIVATE halo2_systems)
add_executable(audit_game_variants tools/audit_game_variants.cpp)
target_link_libraries(audit_game_variants PRIVATE halo2_systems)
add_executable(export_frontend_frame_edges tools/export_frontend_frame_edges.cpp)
target_link_libraries(export_frontend_frame_edges PRIVATE halo2_cache)
add_executable(export_frontend_ui_inventory tools/export_frontend_ui_inventory.cpp)
target_link_libraries(export_frontend_ui_inventory PRIVATE halo2_cache)
add_executable(export_frontend_ui_pages tools/export_frontend_ui_pages.cpp)
target_link_libraries(export_frontend_ui_pages PRIVATE halo2_cache)
set_target_properties(halo2_launcher PROPERTIES OUTPUT_NAME "Halo2-Launcher")
target_link_libraries(halo2_launcher PRIVATE SDL2::SDL2 nlohmann_json::nlohmann_json)
option(HALO2_BUILD_LAUNCHERS_WITH_NATIVE "Build the native game when building source launchers" ON)
set(HALO2_LAUNCHER_RUNTIME_DIR "${CMAKE_CURRENT_BINARY_DIR}" CACHE PATH
    "Existing runtime directory used by local source launchers")
target_compile_definitions(halo2_launcher PRIVATE
    HALO2_PYTHON_EXECUTABLE="${Python3_EXECUTABLE}"
    HALO2_PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    HALO2_RUNTIME_DIR="${HALO2_LAUNCHER_RUNTIME_DIR}")
if(HALO2_BUILD_LAUNCHERS_WITH_NATIVE)
    add_dependencies(halo2_launcher halo2_native)
endif()
add_executable(halo2_title_launcher src/app/launcher.cpp)
set_target_properties(halo2_title_launcher PROPERTIES OUTPUT_NAME "Halo2-Title-Launcher")
target_link_libraries(halo2_title_launcher PRIVATE SDL2::SDL2 nlohmann_json::nlohmann_json)
target_compile_definitions(halo2_title_launcher PRIVATE HALO2_TITLE_FIRST=1
    HALO2_PYTHON_EXECUTABLE="${Python3_EXECUTABLE}"
    HALO2_PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    HALO2_RUNTIME_DIR="${HALO2_LAUNCHER_RUNTIME_DIR}")
if(HALO2_BUILD_LAUNCHERS_WITH_NATIVE)
    add_dependencies(halo2_title_launcher halo2_native)
endif()
# Relocatable first-run executable shipped at the repository root. It can
# prepare game files before any native runtime or generated shader exists.
add_executable(halo2_onboarding src/app/launcher.cpp)
set_target_properties(halo2_onboarding PROPERTIES OUTPUT_NAME "Halo2-Launcher"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/onboarding")
target_link_libraries(halo2_onboarding PRIVATE SDL2::SDL2 nlohmann_json::nlohmann_json)
target_compile_definitions(halo2_onboarding PRIVATE HALO2_PYTHON_EXECUTABLE="python3"
    HALO2_PROJECT_DIR="." HALO2_RUNTIME_DIR="build")
foreach(launcher halo2_launcher halo2_title_launcher halo2_onboarding)
    target_include_directories(${launcher} PRIVATE include)
endforeach()
if(APPLE)
    include(cmake/macos/Application.cmake)
    include(cmake/macos/Python.cmake)
    halo2_macos_python_dependency()
    include(cmake/macos/Steam.cmake)
    halo2_macos_steam_dependency()
    foreach(launcher halo2_launcher halo2_title_launcher halo2_onboarding)
        halo2_macos_application(${launcher})
    endforeach()
endif()
include(cmake/BuildOutput.cmake)
if(NOT MSVC)
    target_compile_options(halo2_native PRIVATE -Wall -Wextra -Wpedantic -Wno-missing-field-initializers)
endif()

include(CTest)
if(BUILD_TESTING)
    add_test(NAME source_layout_tooling COMMAND "${Python3_EXECUTABLE}" -B
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_source_layout.py")
    add_executable(vulkan_portability_tests tests/vulkan_portability_tests.cpp)
    target_include_directories(vulkan_portability_tests PRIVATE include "${Vulkan_INCLUDE_DIRS}")
    add_test(NAME vulkan_portability COMMAND vulkan_portability_tests)
    add_test(NAME test_storage_cleanup COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_test_storage.py)
    set_tests_properties(test_storage_cleanup PROPERTIES TIMEOUT 90)
endif()
include(cmake/CampaignKillVolumes.cmake)
include(cmake/CampaignAiPlacement.cmake)
include(cmake/CampaignPlayerQueries.cmake)
if(BUILD_TESTING)
    # Keep movie decoding independently buildable from gameplay/source oracles.
    add_executable(movie_tests tests/movie_tests.cpp src/platform/media/movie.cpp src/audio/audio.cpp src/audio/mcc_audio.cpp
        src/audio/vista_audio_decode.cpp src/audio/vista_audio_framing.cpp src/audio/sound_fade.cpp
        src/audio/sound_classes.cpp)
    target_include_directories(movie_tests PRIVATE include)
    target_link_libraries(movie_tests PRIVATE PkgConfig::AVCODEC PkgConfig::OPUS SDL2::SDL2 halo2_glm nlohmann_json::nlohmann_json OpenSSL::Crypto Threads::Threads)
    add_test(NAME frontend_movie_routing COMMAND movie_tests)
    add_test(NAME release_movie_bundling COMMAND ${Python3_EXECUTABLE} -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_bundle_movies.py)
    if(HALO2_BUNDLE_MOVIES)
        add_test(NAME bundled_movie_decode COMMAND movie_tests --bundle "${HALO2_MOVIE_SOURCE_DIR}")
        add_test(NAME bundled_movie_hardware_pixels COMMAND movie_tests --compare-decoders "${HALO2_MOVIE_SOURCE_DIR}")
        set_tests_properties(bundled_movie_hardware_pixels PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
        add_test(NAME bundled_credits_hardware_pixels COMMAND movie_tests --compare-file "${HALO2_MOVIE_SOURCE_DIR}/credits_60.mp4")
        set_tests_properties(bundled_credits_hardware_pixels PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    endif()
    add_test(NAME launcher_first_run COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/launcher_first_run_test.py $<TARGET_FILE:halo2_launcher>)
    add_test(NAME installation_resource_ownership COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_installation_paths.py)
    add_test(NAME bundled_launcher_first_run COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/launcher_first_run_test.py $<TARGET_FILE:halo2_onboarding>)
    add_test(NAME title_launcher_first_run COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/launcher_first_run_test.py $<TARGET_FILE:halo2_title_launcher>)
    add_test(NAME native_gameplay_test_capability COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_native_gameplay_test.py)
    add_test(NAME native_frontend_capability COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_frontend_capability.py)
    add_test(NAME native_frontend_runtime_selection COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_frontend_runtime_selection.py)
    add_test(NAME native_frontend_owner_readmission COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_frontend_owner_readmission.py)
    add_test(NAME native_multiplayer_disconnect_recovery COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_multiplayer_disconnect_recovery.py)
    set_tests_properties(native_multiplayer_disconnect_recovery PROPERTIES TIMEOUT 30)
    add_test(NAME mcc_presentation_dispatch COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mcc_presentation_dispatch.py)
    add_test(NAME mcc_sandbox_launcher COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mcc_sandbox_launcher.py)
    add_test(NAME mcc_prepared_cache_delta COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mcc_prepared_cache_delta.py)
    add_test(NAME native_frontend_launcher_routing COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_frontend_launcher_routing.py
        --launcher $<TARGET_FILE:halo2_launcher>)
    add_test(NAME native_gameplay_launcher_routing COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_gameplay_launcher_routing.py
        --launcher $<TARGET_FILE:halo2_launcher>)
    add_test(NAME linux_build_entry COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_build_linux.py)
    add_test(NAME platform_build_output COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_stage_build_output.py)
    add_test(NAME native_packaged_shader_defaults COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_packaged_shader_defaults.py)
    add_test(NAME mcc_onboarding COMMAND ${Python3_EXECUTABLE} -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_mcc_onboarding.py)
    add_test(NAME native_variant_assets COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_variant_assets.py)
    add_test(NAME steam_content_plan COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_steam_content.py)
    add_test(NAME steam_content_session COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_steam_session.py)
    add_test(NAME mcc_menu_assets COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_mcc_menu_assets.py)
    add_test(NAME mcc_bundled_movies COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mcc_movies.py)
    add_test(NAME mcc_authored_prompts COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mcc_prompts.py)
    add_test(NAME mcc_native_ui_layout COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mcc_ui_layout.py)
    add_test(NAME launcher_automatic_build COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_build_runtime.py)
    add_test(NAME launcher_update COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_update_runtime.py)
    find_program(HALO2_LAUNCHER_XVFB xvfb-run)
    if(HALO2_LAUNCHER_XVFB)
        add_test(NAME launcher_home_ui COMMAND ${HALO2_LAUNCHER_XVFB} -a ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_launcher_ui.py)
        add_test(NAME mcc_setup_ui COMMAND ${HALO2_LAUNCHER_XVFB} -a ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mcc_setup_ui.py)
        add_test(NAME steam_setup_ui COMMAND ${HALO2_LAUNCHER_XVFB} -a ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_macos_steam_ui.py)
    endif()
    add_executable(pause_gameplay_completion_tests tests/pause_gameplay_completion_tests.cpp)
    target_link_libraries(pause_gameplay_completion_tests PRIVATE halo2_renderer)
    set(HALO2_PAUSE_TEST_UI_ASSETS "" CACHE PATH "Imported UI sampling pack for campaign pause text tests")
    set(HALO2_PAUSE_TEST_UI_SOURCE "${HALO2_PAUSE_TEST_UI_ASSETS}")
    if(NOT HALO2_PAUSE_TEST_UI_SOURCE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/asset-sources.tsv")
        file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/assets/asset-sources.tsv" HALO2_PAUSE_ASSET_ROWS REGEX "^vista\t")
        foreach(HALO2_PAUSE_ASSET_ROW IN LISTS HALO2_PAUSE_ASSET_ROWS)
            string(REGEX REPLACE "^vista\t" "" HALO2_PAUSE_TEST_UI_SOURCE "${HALO2_PAUSE_ASSET_ROW}")
        endforeach()
    endif()
    add_test(NAME native_pause_gameplay_completion COMMAND pause_gameplay_completion_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/01a_tutorial.map
        "${HALO2_PAUSE_TEST_UI_SOURCE}")
    set_tests_properties(native_pause_gameplay_completion PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy")
    add_test(NAME native_controller_settings COMMAND controller_settings_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_pause_input_tables.json)
    add_test(NAME native_game_pause_menu COMMAND game_pause_menu_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer"
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_pause_input_tables.json)
    set_tests_properties(native_game_pause_menu PROPERTIES SKIP_RETURN_CODE 77
        ENVIRONMENT "SDL_AUDIODRIVER=dummy")
    if(UNIX AND NOT APPLE)
        add_library(game_pause_window_probe SHARED tests/game_pause_window_probe.cpp)
        target_link_libraries(game_pause_window_probe PRIVATE SDL2::SDL2 Vulkan::Vulkan ${CMAKE_DL_LIBS})
        add_executable(game_pause_window_tests tests/game_pause_window_tests.cpp)
        target_link_libraries(game_pause_window_tests PRIVATE halo2_renderer)
        if(HALO2_LAUNCHER_XVFB)
            add_test(NAME native_game_pause_window COMMAND ${HALO2_LAUNCHER_XVFB} -a
                $<TARGET_FILE:game_pause_window_tests> ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
                "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer")
            set_tests_properties(native_game_pause_window PROPERTIES SKIP_RETURN_CODE 77
                ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy" TIMEOUT 120
                FAIL_REGULAR_EXPRESSION "VUID-|Validation Error")
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib/libVkLayer_khronos_validation.so")
                set_property(TEST native_game_pause_window APPEND PROPERTY ENVIRONMENT
                    "VK_LAYER_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d"
                    "LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib:$ENV{LD_LIBRARY_PATH}")
            endif()
        endif()
    endif()
    add_executable(source_device_motion_tests tests/source_device_motion_tests.cpp)
    add_executable(prop_region_tests tests/prop_region_tests.cpp)
    target_link_libraries(prop_region_tests PRIVATE halo2_systems)
    add_executable(source_model_regions_tests tests/source_model_regions_tests.cpp)
    target_link_libraries(source_model_regions_tests PRIVATE halo2_cache)
    add_test(NAME source_model_regions COMMAND source_model_regions_tests)
    add_executable(source_machine_door_tests tests/source_machine_door_tests.cpp)
    target_link_libraries(source_machine_door_tests PRIVATE halo2_cache)
    add_test(NAME source_machine_door COMMAND source_machine_door_tests)
    add_executable(source_device_event_tests tests/source_device_event_tests.cpp)
    target_link_libraries(source_device_event_tests PRIVATE halo2_cache)
    add_test(NAME retail_source_device_event COMMAND source_device_event_tests --fixture
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_device_event.tsv)
    add_executable(source_device_world_tests tests/source_device_world_tests.cpp)
    add_executable(source_map_control_input_tests tests/source_map_control_input_tests.cpp)
    target_link_libraries(source_map_control_input_tests PRIVATE halo2_systems)
    add_test(NAME source_map_control_input COMMAND source_map_control_input_tests)
    set_tests_properties(source_map_control_input PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    add_executable(source_device_network_tests tests/source_device_network_tests.cpp)
    target_link_libraries(source_device_network_tests PRIVATE halo2_systems)
    add_test(NAME source_device_network COMMAND source_device_network_tests)
    set_tests_properties(source_device_network PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} SKIP_RETURN_CODE 77 TIMEOUT 120)
    target_link_libraries(source_device_world_tests PRIVATE halo2_systems)
    add_test(NAME source_device_world COMMAND source_device_world_tests)
    set_tests_properties(source_device_world PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(source_device_motion_tests PRIVATE halo2_cache)
    add_test(NAME retail_source_device_motion COMMAND source_device_motion_tests
        --fixture ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_device_motion.tsv)
endif()
if(BUILD_TESTING)
    add_executable(source_control_tests tests/source_control_tests.cpp)
    add_executable(source_control_target_tests tests/source_control_target_tests.cpp)
    add_executable(source_unit_center_tests tests/source_unit_center_tests.cpp)
    target_link_libraries(source_unit_center_tests PRIVATE halo2_systems)
    add_test(NAME source_unit_center_guards COMMAND source_unit_center_tests)
    add_test(NAME retail_source_unit_center COMMAND source_unit_center_tests --fixture
        "${PROJECT_SOURCE_DIR}/tests/fixtures/retail_source_unit_center.tsv")
    target_link_libraries(source_control_target_tests PRIVATE halo2_cache)
    add_test(NAME source_control_target_guards COMMAND source_control_target_tests)
    add_test(NAME retail_source_control_target COMMAND source_control_target_tests --fixture
        "${PROJECT_SOURCE_DIR}/tests/fixtures/retail_source_control_target.tsv")
    target_link_libraries(source_control_tests PRIVATE halo2_cache)
    add_test(NAME source_control_guards COMMAND source_control_tests)
    add_test(NAME retail_source_control COMMAND source_control_tests --fixture
        "${PROJECT_SOURCE_DIR}/tests/fixtures/retail_source_control.tsv")
    add_test(NAME retail_source_control_assets COMMAND source_control_tests --assets
        "${PROJECT_SOURCE_DIR}/tests/fixtures/retail_source_control_assets.tsv")
    set_tests_properties(retail_source_control_assets PROPERTIES
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" SKIP_RETURN_CODE 77)
endif()
if(BUILD_TESTING)
    add_test(NAME live_smg_muzzle_lifetimes COMMAND muzzle_effects_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(live_smg_muzzle_lifetimes PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_muzzle_presentation_handoff COMMAND muzzle_effects_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map" "${HALO2_TU15_GAMEPLAY_XBE}")
    set_tests_properties(native_muzzle_presentation_handoff PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME stock_smg_movement_metadata COMMAND muzzle_movement_metadata_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps" "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps")
endif()
include(cmake/SourceWeatherSimulation.cmake)
halo2_add_source_weather_simulation("${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(halo2_systems PRIVATE halo2_weather_simulation)
include(cmake/SourceWeatherGraphics.cmake)
halo2_add_source_weather_graphics("${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(halo2_renderer PUBLIC halo2_weather_render)
include(cmake/SourceWeatherDepthAlias.cmake)
halo2_add_source_weather_depth_alias("${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(halo2_renderer PUBLIC halo2_weather_depth_alias)
include(cmake/SourceWeatherReferenceChecks.cmake)
halo2_add_source_weather_reference_checks("${CMAKE_CURRENT_SOURCE_DIR}")
include(cmake/SourceSkyStencil.cmake)
halo2_add_source_sky_stencil("${CMAKE_CURRENT_SOURCE_DIR}")
if(BUILD_TESTING)
    add_executable(multiplayer_outcome_tests tests/multiplayer_outcome_tests.cpp)
    target_link_libraries(multiplayer_outcome_tests PRIVATE halo2_systems)
    add_test(NAME native_multiplayer_outcome_ownership COMMAND multiplayer_outcome_tests)
    add_executable(vista_local_user_tests tests/vista_local_user_tests.cpp src/ui/vista/vista_local_user.cpp)
    target_include_directories(vista_local_user_tests PRIVATE include)
    add_test(NAME original_vista_local_user COMMAND vista_local_user_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_local_user_original.tsv)
    add_executable(vista_controller_profile_tests tests/vista_controller_profile_tests.cpp
        src/ui/vista/vista_controller_profile.cpp src/ui/vista/vista_local_user.cpp)
    target_include_directories(vista_controller_profile_tests PRIVATE include)
    target_link_libraries(vista_controller_profile_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME original_vista_controller_profile COMMAND vista_controller_profile_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_controller_profile_original.json)
    add_executable(frontend_session_tests tests/frontend_session_tests.cpp)
    target_link_libraries(frontend_session_tests PRIVATE halo2_systems)
    add_test(NAME native_frontend_session_ownership COMMAND frontend_session_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map"
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_pcr_results_original.tsv)
    set_tests_properties(native_frontend_session_ownership PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(multiplayer_outcome_runtime_tests tests/multiplayer_outcome_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
    target_link_libraries(multiplayer_outcome_runtime_tests PRIVATE halo2_renderer)
    add_test(NAME native_multiplayer_outcome_handoff COMMAND multiplayer_outcome_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe)
    set_tests_properties(native_multiplayer_outcome_handoff PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(native_synchronization_tests tests/native_synchronization_tests.cpp)
    target_link_libraries(native_synchronization_tests PRIVATE halo2_native_synchronization)
    add_test(NAME native_synchronization COMMAND native_synchronization_tests)
    add_executable(native_synchronization_continuation_tests tests/native_synchronization_continuation_tests.cpp)
    target_link_libraries(native_synchronization_continuation_tests PRIVATE halo2_native_synchronization)
    add_test(NAME native_synchronization_continuation COMMAND native_synchronization_continuation_tests)
    set_tests_properties(native_synchronization_continuation PROPERTIES TIMEOUT 15)
    add_executable(native_threads_tests tests/native_threads_tests.cpp)
    target_link_libraries(native_threads_tests PRIVATE halo2_native_synchronization)
    add_test(NAME native_threads COMMAND native_threads_tests)
    add_executable(sdl_sound_platform_tests tests/sdl_sound_platform_tests.cpp)
    target_link_libraries(sdl_sound_platform_tests PRIVATE halo2_sound_platform)
    add_test(NAME native_sdl_sound_platform COMMAND sdl_sound_platform_tests)
    add_test(NAME native_sdl_sound_platform_device_failure COMMAND sdl_sound_platform_tests --device-failure)
    add_executable(sdl_sound_timeline_tests tests/sdl_sound_timeline_tests.cpp)
    target_link_libraries(sdl_sound_timeline_tests PRIVATE halo2_sound_platform)
    add_test(NAME native_sdl_sound_timeline COMMAND sdl_sound_timeline_tests)
    add_test(NAME original_opening_sdl_sound_timeline COMMAND sdl_sound_timeline_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/opening_dialogue_timing_tu15.tsv)
    set_tests_properties(original_opening_sdl_sound_timeline PROPERTIES SKIP_RETURN_CODE 77)
    include(cmake/SdlSoundContinuation.cmake)
    add_executable(sdl_sound_cache_packet_tests tests/sdl_sound_cache_packet_tests.cpp)
    target_link_libraries(sdl_sound_cache_packet_tests PRIVATE halo2_systems)
    add_test(NAME owned_sdl_sound_reload_packets COMMAND sdl_sound_cache_packet_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
    set_tests_properties(owned_sdl_sound_reload_packets PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_game_variant_tests tests/source_game_variant_tests.cpp)
    target_link_libraries(source_game_variant_tests PRIVATE halo2_game_variant)
    add_test(NAME original_game_variant_buffers COMMAND source_game_variant_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_game_variant_original.tsv)
    add_executable(game_variant_normalization_tests tests/game_variant_normalization_tests.cpp)
    target_link_libraries(game_variant_normalization_tests PRIVATE halo2_systems)
    add_test(NAME original_game_variant_normalization COMMAND game_variant_normalization_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/game_variant_normalization_original.tsv)
    set_tests_properties(original_game_variant_normalization PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(variant_write_order_tests tests/variant_write_order_tests.cpp)
    target_link_libraries(variant_write_order_tests PRIVATE halo2_systems)
    add_test(NAME original_game_variant_write_order COMMAND variant_write_order_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/variant_write_order_original.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(original_game_variant_write_order PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(particle_render_inputs_tests tests/particle_render_inputs_tests.cpp)
    add_executable(particle_birth_tests tests/particle_birth_tests.cpp)
    add_executable(particle_emitter_tests tests/particle_emitter_tests.cpp)
    add_executable(particle_creation_tests tests/particle_creation_tests.cpp)
    target_link_libraries(particle_creation_tests PRIVATE halo2_cache)
    add_test(NAME original_live_particle_creation COMMAND particle_creation_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_creation.inputs.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_creation.original.bin)
    set_tests_properties(original_live_particle_creation PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(particle_emitter_tests PRIVATE halo2_cache)
    add_test(NAME original_live_particle_emitters COMMAND particle_emitter_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_emitters.inputs.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_emitters.original.bin)
    set_tests_properties(original_live_particle_emitters PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_particle_emitter_oracle COMMAND particle_emitter_tests --controlled
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_emitter_oracle.inputs.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_emitter_oracle.original.bin)
    set_tests_properties(original_particle_emitter_oracle PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(particle_birth_tests PRIVATE halo2_cache)
    add_test(NAME original_live_particle_birth_initialization COMMAND particle_birth_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_birth.inputs.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_birth.original.bin)
    set_tests_properties(original_live_particle_birth_initialization PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_particle_birth_shapes COMMAND particle_birth_tests --controlled-shapes
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_birth_shapes.inputs.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_birth_shapes.original.bin)
    set_tests_properties(original_particle_birth_shapes PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(particle_render_inputs_tests PRIVATE halo2_cache)
    add_test(NAME original_live_particle_render_inputs COMMAND particle_render_inputs_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_render_inputs.inputs.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_render_inputs.original.bin)
    set_tests_properties(original_live_particle_render_inputs PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_particle_render_coordinates COMMAND particle_render_inputs_tests --coordinates
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_render_coordinates.inputs.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_render_coordinates.original.bin)
    set_tests_properties(original_particle_render_coordinates PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(particle_value_tests tests/particle_value_tests.cpp)
    target_link_libraries(particle_value_tests PRIVATE halo2_cache)
    add_test(NAME original_particle_value_cache COMMAND particle_value_tests --compare
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_values.inputs.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_values.original.bin)
    add_test(NAME original_live_particle_values_sprite COMMAND particle_value_tests --live-sprites
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_live_sprite.inputs.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_live_sprite.original.bin)
    set_tests_properties(original_particle_value_cache original_live_particle_values_sprite PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(particle_view_tests tests/particle_view_tests.cpp)
    target_link_libraries(particle_view_tests PRIVATE halo2_cache)
    add_test(NAME original_particle_view_constants COMMAND particle_view_tests --compare
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_view.inputs.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_view.original.bin)
    set_tests_properties(original_particle_view_constants PROPERTIES SKIP_RETURN_CODE 77)
    if(TARGET halo2_particle_renderer)
        add_executable(particle_graphics_tests tests/particle_graphics_tests.cpp)
        target_link_libraries(particle_graphics_tests PRIVATE halo2_particle_renderer)
        target_compile_definitions(particle_graphics_tests PRIVATE
            HALO2_PARTICLE_GRAPHICS_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
        add_test(NAME original_particle_graphics_gpu COMMAND particle_graphics_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/xemu-capture/particle-graphics-fixture
            ${CMAKE_CURRENT_BINARY_DIR}/particle-graphics-capture)
        set_tests_properties(original_particle_graphics_gpu PROPERTIES SKIP_RETURN_CODE 77)
        foreach(distortion_map ascension colossus)
            add_test(NAME authored_${distortion_map}_distortion_graphics_gpu COMMAND particle_graphics_tests
                --distortion ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/${distortion_map}.map
                ${CMAKE_CURRENT_BINARY_DIR}/${distortion_map}-distortion-capture)
            set_tests_properties(authored_${distortion_map}_distortion_graphics_gpu PROPERTIES SKIP_RETURN_CODE 77)
            add_test(NAME authored_${distortion_map}_distortion_feedback_gpu COMMAND particle_graphics_tests
                --distortion-feedback ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/${distortion_map}.map
                ${CMAKE_CURRENT_BINARY_DIR}/${distortion_map}-distortion-feedback-capture)
            set_tests_properties(authored_${distortion_map}_distortion_feedback_gpu PROPERTIES SKIP_RETURN_CODE 77)
        endforeach()
    endif()
    add_executable(particle_material_tests tests/particle_material_tests.cpp)
    target_link_libraries(particle_material_tests PRIVATE halo2_cache)
    add_test(NAME original_particle_material COMMAND particle_material_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map --compare
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_material.inputs.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_material.original.bin)
    set_tests_properties(original_particle_material PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(particle_sprite_tests tests/particle_sprite_tests.cpp)
    target_link_libraries(particle_sprite_tests PRIVATE halo2_cache)
    add_executable(particle_submission_tests tests/particle_submission_tests.cpp)
    target_link_libraries(particle_submission_tests PRIVATE halo2_cache)
    add_executable(particle_shader_selection_tests tests/particle_shader_selection_tests.cpp)
    target_link_libraries(particle_shader_selection_tests PRIVATE halo2_cache)
    add_test(NAME original_particle_shader_selection COMMAND particle_shader_selection_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map --compare
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_shader_selection.inputs.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_shader_selection.retail.bin)
    add_test(NAME original_particle_submission COMMAND particle_submission_tests --compare
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_submission.inputs.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_submission.retail.bin)
    set_tests_properties(original_particle_shader_selection original_particle_submission PROPERTIES SKIP_RETURN_CODE 77)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_vertex.comp.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${GLSLANG_VALIDATOR} -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders/particle_vertex.comp
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_vertex.comp.spv
        DEPENDS shaders/particle_vertex.comp shaders/vertex_program.glsl shaders/render_view.glsl VERBATIM)
    add_custom_target(particle_vertex_test_shader DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_vertex.comp.spv)
    add_executable(particle_vertex_tests tests/particle_vertex_tests.cpp)
    target_link_libraries(particle_vertex_tests PRIVATE halo2_cache Vulkan::Vulkan)
    target_compile_definitions(particle_vertex_tests PRIVATE
        HALO2_PARTICLE_VERTEX_TEST_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_vertex.comp.spv")
    add_dependencies(particle_vertex_tests particle_vertex_test_shader)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map)
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_specialized.comp
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_particle_vertex_probe.py
                --classifications 7 --output ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_specialized.comp
            DEPENDS tools/generate_particle_vertex_probe.py tools/audit_rendering.py
                assets/disc/maps/mainmenu.map VERBATIM)
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_specialized.comp.spv
            COMMAND ${GLSLANG_VALIDATOR} -V ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_specialized.comp
                -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_specialized.comp.spv
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_specialized.comp VERBATIM)
        add_custom_target(particle_specialized_test_shader DEPENDS
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_specialized.comp.spv)
        add_dependencies(particle_vertex_tests particle_specialized_test_shader)
        add_test(NAME original_particle_vertex_gpu COMMAND particle_vertex_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_vertex.inputs.bin
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_vertex.capture.bin
            --compare-gpu ${CMAKE_CURRENT_BINARY_DIR}/shaders/particle_specialized.comp.spv)
    endif()
    add_test(NAME retail_title_particle_assets COMMAND particle_sprite_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map)
    add_test(NAME retail_title_particle_sprite COMMAND particle_sprite_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map --compare
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_sprite.inputs.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_particle_sprite.retail.bin)
    set_tests_properties(retail_title_particle_assets retail_title_particle_sprite
        PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(weapon_error_tests tests/weapon_error_tests.cpp)
    target_include_directories(weapon_error_tests PRIVATE include)
    target_link_libraries(weapon_error_tests PRIVATE halo2_weapon_kernels)
    add_test(NAME retail_weapon_error_kernels COMMAND weapon_error_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_weapon_error.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe)
    set_tests_properties(retail_weapon_error_kernels PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(original_data_tests tests/original_data_tests.cpp)
    target_include_directories(original_data_tests PRIVATE include)
    target_link_libraries(original_data_tests PRIVATE halo2_weapon_kernels)
    add_test(NAME local_original_data COMMAND original_data_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    set_tests_properties(local_original_data PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(multiplayer_balance_tests tests/multiplayer_balance_tests.cpp)
    target_link_libraries(multiplayer_balance_tests PRIVATE halo2_systems)
    add_test(NAME original_multiplayer_balance COMMAND multiplayer_balance_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/title-update-1.5/native-balance/1.1-stores.tsv)
    set_tests_properties(original_multiplayer_balance PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(multiplayer_netgame_postprocess_tests tests/multiplayer_netgame_postprocess_tests.cpp)
    target_link_libraries(multiplayer_netgame_postprocess_tests PRIVATE halo2_cache)
    add_test(NAME original_multiplayer_netgame COMMAND multiplayer_netgame_postprocess_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/beavercreek.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/title-update-1.5/netgame-postprocess/measured/beavercreek-disc_1_1/netgame-flags-processed.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/title-update-1.5/netgame-native-leaf/review/fixtures/beavercreek-disc_1_1.tsv)
    set_tests_properties(original_multiplayer_netgame PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(smg_match_tests tests/smg_match_tests.cpp)
    target_link_libraries(smg_match_tests PRIVATE halo2_systems)
    add_test(NAME retail_smg_match COMMAND smg_match_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_smg_match.tsv)
    set_tests_properties(retail_smg_match PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_smg_lan COMMAND smg_match_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_smg_match.tsv --lan)
    set_tests_properties(retail_smg_lan PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(spawn_match_tests tests/spawn_match_tests.cpp)
    add_executable(spawn_placement_tests tests/spawn_placement_tests.cpp)
    target_link_libraries(spawn_placement_tests PRIVATE halo2_systems)
    add_test(NAME retail_spawn_placement COMMAND spawn_placement_tests
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-placement-fixtures.tsv)
    set_tests_properties(retail_spawn_placement PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(respawn_timing_tests tests/respawn_timing_tests.cpp)
    add_executable(slayer_accounting_tests tests/slayer_accounting_tests.cpp)
    add_executable(team_slayer_tests tests/team_slayer_tests.cpp
        src/gameplay/multiplayer/slayer.cpp src/gameplay/units/lethal_attribution.cpp)
    target_include_directories(team_slayer_tests PRIVATE include)
    add_test(NAME original_team_slayer COMMAND team_slayer_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/team_slayer_original.tsv)
    add_executable(team_session_tests tests/team_session_tests.cpp)
    target_link_libraries(team_session_tests PRIVATE halo2_systems)
    add_test(NAME original_team_session COMMAND team_session_tests
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/team_session_original.tsv)
    add_executable(team_match_tests tests/team_match_tests.cpp)
    add_executable(native_player_profile_tests tests/native_player_profile_tests.cpp)
    target_link_libraries(native_player_profile_tests PRIVATE halo2_systems)
    add_test(NAME native_player_profile COMMAND native_player_profile_tests ${CMAKE_CURRENT_SOURCE_DIR})
    add_executable(player_lives_native_tests tests/player_lives_native_tests.cpp)
    target_link_libraries(player_lives_native_tests PRIVATE halo2_systems)
    add_test(NAME native_player_lives COMMAND player_lives_native_tests ${CMAKE_CURRENT_SOURCE_DIR})
    add_executable(round_end_native_tests tests/round_end_native_tests.cpp)
    target_link_libraries(round_end_native_tests PRIVATE halo2_systems)
    add_test(NAME native_round_end COMMAND round_end_native_tests ${CMAKE_CURRENT_SOURCE_DIR})
    add_executable(game_end_native_tests tests/game_end_native_tests.cpp)
    target_link_libraries(game_end_native_tests PRIVATE halo2_systems)
    add_test(NAME native_game_end COMMAND game_end_native_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    target_link_libraries(team_match_tests PRIVATE halo2_systems)
    add_test(NAME native_team_match COMMAND team_match_tests
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/team_session_original.tsv)
    target_link_libraries(slayer_accounting_tests PRIVATE halo2_systems)
    add_test(NAME retail_slayer_accounting COMMAND slayer_accounting_tests
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/slayer-accounting-fixtures.tsv)
    set_tests_properties(retail_slayer_accounting PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(posed_collision_tests tests/posed_collision_tests.cpp)
    target_link_libraries(posed_collision_tests PRIVATE halo2_systems)
    add_test(NAME retail_posed_collision COMMAND posed_collision_tests
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/posed-collision-fixtures.tsv)
    set_tests_properties(retail_posed_collision PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(terrain_collision_tests tests/terrain_collision_tests.cpp)
    target_link_libraries(terrain_collision_tests PRIVATE halo2_systems)
    add_test(NAME retail_terrain_collision COMMAND terrain_collision_tests
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/terrain-collision-fixtures.tsv)
    set_tests_properties(retail_terrain_collision PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(terrain_outer_tests tests/terrain_outer_tests.cpp)
    target_link_libraries(terrain_outer_tests PRIVATE halo2_systems)
    add_test(NAME retail_terrain_outer COMMAND terrain_outer_tests
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/inventory-source/pickup-outer-fixtures/fixtures.tsv)
    set_tests_properties(retail_terrain_outer PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(pickup_visibility_tests tests/pickup_visibility_tests.cpp)
    target_link_libraries(pickup_visibility_tests PRIVATE halo2_systems)
    add_test(NAME retail_pickup_visibility COMMAND pickup_visibility_tests
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/pickup-consumer-fixtures.tsv)
    set_tests_properties(retail_pickup_visibility PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_pickup_pipeline COMMAND pickup_visibility_tests
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/pickup-consumer-fixtures.tsv
        --item-points)
    set_tests_properties(retail_pickup_pipeline PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_input_tests tests/unit_input_tests.cpp)
    target_link_libraries(unit_input_tests PRIVATE halo2_systems)
    add_test(NAME retail_unit_input COMMAND unit_input_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/unit-input-fixtures.tsv)
    set_tests_properties(retail_unit_input PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_pickup_runtime_tests tests/unit_pickup_runtime_tests.cpp)
    add_executable(unit_object_runtime_tests tests/unit_object_runtime_tests.cpp)
    target_link_libraries(unit_object_runtime_tests PRIVATE halo2_systems)
    add_test(NAME original_unit_object_composition COMMAND unit_object_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_object_runtime_original.tsv)
    set_tests_properties(original_unit_object_composition PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(unit_pickup_runtime_tests PRIVATE halo2_systems)
    add_test(NAME original_unit_pickup_runtime COMMAND unit_pickup_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_pickup_runtime_original.tsv)
    set_tests_properties(original_unit_pickup_runtime PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_action_lookup_tests tests/unit_action_lookup_tests.cpp)
    target_link_libraries(unit_action_lookup_tests PRIVATE halo2_systems)
    add_test(NAME original_unit_action_lookup COMMAND unit_action_lookup_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_action_lookup_original.tsv)
    set_tests_properties(original_unit_action_lookup PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_weapon_routing_tests tests/unit_weapon_routing_tests.cpp)
    target_link_libraries(unit_weapon_routing_tests PRIVATE halo2_systems)
    add_test(NAME original_unit_weapon_routing COMMAND unit_weapon_routing_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_weapon_routing_original.tsv)
    set_tests_properties(original_unit_weapon_routing PROPERTIES SKIP_RETURN_CODE 77)

    add_executable(unit_inventory_tests tests/unit_inventory_tests.cpp)
    target_link_libraries(unit_inventory_tests PRIVATE halo2_systems)
    add_test(NAME retail_unit_inventory COMMAND unit_inventory_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/unit-inventory-fixtures.tsv)
    set_tests_properties(retail_unit_inventory PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(object_schedule_tests tests/object_schedule_tests.cpp)
    target_link_libraries(object_schedule_tests PRIVATE halo2_systems)
    add_test(NAME retail_object_schedule COMMAND object_schedule_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/object-schedule-fixtures.tsv)
    set_tests_properties(retail_object_schedule PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(player_command_tests tests/player_command_tests.cpp)
    target_link_libraries(player_command_tests PRIVATE halo2_systems)
    add_test(NAME retail_player_command COMMAND player_command_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/player-command-fixtures.tsv)
    set_tests_properties(retail_player_command PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_player_control COMMAND player_command_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/player-command-fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/inventory-source/player-control-native/complete/fixtures.tsv)
    set_tests_properties(original_player_control PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(item_motion_tests tests/item_motion_tests.cpp)
    target_link_libraries(item_motion_tests PRIVATE halo2_systems)
    add_test(NAME retail_item_motion COMMAND item_motion_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/item-motion-fixtures.tsv)
    set_tests_properties(retail_item_motion PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(item_transform_math_tests tests/item_transform_math_tests.cpp)
    target_link_libraries(item_transform_math_tests PRIVATE halo2_systems)
    add_test(NAME retail_item_transform_math COMMAND item_transform_math_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/item-transform-math-fixtures.tsv)
    set_tests_properties(retail_item_transform_math PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(item_node_bounds_tests tests/item_node_bounds_tests.cpp)
    target_link_libraries(item_node_bounds_tests PRIVATE halo2_systems)
    add_test(NAME retail_item_node_bounds COMMAND item_node_bounds_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/item-node-bounds-fixtures.tsv)
    set_tests_properties(retail_item_node_bounds PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_contact_record_tests tests/physics_contact_record_tests.cpp)
    target_link_libraries(physics_contact_record_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_contact_record COMMAND physics_contact_record_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/contact-record/measured/fixtures.tsv)
    set_tests_properties(original_physics_contact_record PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_contact_lifecycle_tests tests/physics_contact_lifecycle_tests.cpp)
    target_link_libraries(physics_contact_lifecycle_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_contact_lifecycle COMMAND physics_contact_lifecycle_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/contact-lifecycle/final/exported-math/fixtures.tsv)
    set_tests_properties(original_physics_contact_lifecycle PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_body_state_tests tests/physics_body_state_tests.cpp)
    target_link_libraries(physics_body_state_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_body_state COMMAND physics_body_state_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-body-bridge/final/exported-owner/fixtures.tsv")
    set_tests_properties(original_physics_body_state PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_body_factory_tests tests/physics_body_factory_tests.cpp)
    target_link_libraries(physics_body_factory_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_body_factory COMMAND physics_body_factory_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-body-factory/final/exported-owner/fixtures.tsv")
    set_tests_properties(original_physics_body_factory PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_body_slots_tests tests/physics_body_slots_tests.cpp)
    target_link_libraries(physics_body_slots_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_body_slots COMMAND physics_body_slots_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-body-slots/final/exported-owner/fixtures.tsv")
    set_tests_properties(original_physics_body_slots PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_shape_wrapper_tests tests/physics_shape_wrapper_tests.cpp)
    target_link_libraries(physics_shape_wrapper_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_shape_wrapper COMMAND physics_shape_wrapper_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-body-prepare/final/exported-owner/fixtures.tsv")
    set_tests_properties(original_physics_shape_wrapper PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_body_grouping_tests tests/physics_body_grouping_tests.cpp)
    target_link_libraries(physics_body_grouping_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_body_grouping COMMAND physics_body_grouping_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-body-grouping/final/exported-owner/fixtures.tsv")
    set_tests_properties(original_physics_body_grouping PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_body_disposal_tests tests/physics_body_disposal_tests.cpp)
    target_link_libraries(physics_body_disposal_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_body_disposal COMMAND physics_body_disposal_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-body-disposal/final/exported-owner/fixtures.tsv")
    set_tests_properties(original_physics_body_disposal PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_world_membership_tests tests/physics_world_membership_tests.cpp)
    target_link_libraries(physics_world_membership_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_world_membership COMMAND physics_world_membership_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-world-insertion/final/exported-owner/fixtures.tsv")
    set_tests_properties(original_physics_world_membership PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_broadphase_tests tests/physics_broadphase_tests.cpp)
    target_link_libraries(physics_broadphase_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_broadphase COMMAND physics_broadphase_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-broadphase/final/exported-owner/fixtures.tsv)
    set_tests_properties(original_physics_broadphase PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_pair_manager_tests tests/physics_pair_manager_tests.cpp)
    target_link_libraries(physics_pair_manager_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_pair_manager COMMAND physics_pair_manager_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-pair-manager/final/exported-owner/fixtures.tsv)
    set_tests_properties(original_physics_pair_manager PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_agent_scheduler_tests tests/physics_agent_scheduler_tests.cpp)
    target_link_libraries(physics_agent_scheduler_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_agent_scheduler COMMAND physics_agent_scheduler_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-agent-scheduler/final/exported-owner/fixtures.tsv)
    set_tests_properties(original_physics_agent_scheduler PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_current_broadphase_tests tests/physics_current_broadphase_tests.cpp)
    target_link_libraries(physics_current_broadphase_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_current_broadphase COMMAND physics_current_broadphase_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-current-broadphase/final/exported-owner/fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-current-broadphase/final/exported-schedule/fixtures.tsv)
    set_tests_properties(original_physics_current_broadphase PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_bsp_children_tests tests/physics_bsp_children_tests.cpp)
    target_link_libraries(physics_bsp_children_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_bsp_children COMMAND physics_bsp_children_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-bsp-children/final/exported-owner/fixtures.tsv)
    set_tests_properties(original_physics_bsp_children PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_child_agents_tests tests/physics_child_agents_tests.cpp)
    target_link_libraries(physics_child_agents_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_child_agents COMMAND physics_child_agents_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-child-agents/final/exported-owner/fixtures.tsv)
    set_tests_properties(original_physics_child_agents PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_narrowphase_owners_tests tests/physics_narrowphase_owners_tests.cpp)
    target_link_libraries(physics_narrowphase_owners_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_narrowphase_owners COMMAND physics_narrowphase_owners_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-narrowphase-owners/final/exported-owner/fixtures.tsv)
    set_tests_properties(original_physics_narrowphase_owners PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_gjk_tests tests/physics_gjk_tests.cpp)
    target_link_libraries(physics_gjk_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_gjk COMMAND physics_gjk_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-gjk-owners/final/exported-owner/fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-gjk-owners/final/exported-composition/fixtures.tsv)
    set_tests_properties(original_physics_gjk PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_contact_batches_tests tests/physics_contact_batches_tests.cpp)
    target_link_libraries(physics_contact_batches_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_contact_batches COMMAND physics_contact_batches_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-contact-batches/final/exported-owner/fixtures.tsv)
    set_tests_properties(original_physics_contact_batches PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_contact_dispatch_tests tests/physics_contact_dispatch_tests.cpp)
    target_link_libraries(physics_contact_dispatch_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_contact_dispatch COMMAND physics_contact_dispatch_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/power-contact-dispatch/final/exported-owner/fixtures.tsv)
    set_tests_properties(original_physics_contact_dispatch PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_world_step_tests tests/physics_world_step_tests.cpp)
    target_link_libraries(physics_world_step_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_world_step COMMAND physics_world_step_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-world-step-20261010/exported-verified-v2/fixture.json)
    add_test(NAME original_physics_motion_tu15 COMMAND physics_world_step_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-world-step-20261010/u5-verified/fixture.json)
    add_test(NAME original_physics_connected_world COMMAND physics_world_step_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-world-step-20261010/connected-verified-v3/fixture.json)
    set_tests_properties(original_physics_world_step original_physics_motion_tu15 original_physics_connected_world PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_physics_solver_motion COMMAND physics_world_step_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-solver-motion-20261011/disc-verified/fixture.json)
    add_test(NAME original_physics_solver_motion_tu15 COMMAND physics_world_step_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-solver-motion-20261011/u5-verified/fixture.json)
    set_tests_properties(original_physics_solver_motion original_physics_solver_motion_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_physics_constraint_step COMMAND physics_world_step_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/sandbox-source/physics-solver-motion-20261011/constraints-verified/fixture.json)
    set_tests_properties(original_physics_constraint_step PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(spatial_registry_tests tests/spatial_registry_tests.cpp)
    target_link_libraries(spatial_registry_tests PRIVATE halo2_systems)
    add_test(NAME retail_spatial_registry COMMAND spatial_registry_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spatial-registry-fixtures.tsv)
    set_tests_properties(retail_spatial_registry PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_variation_tests tests/sound_variation_tests.cpp)
    target_link_libraries(sound_variation_tests PRIVATE halo2_cache)
    add_test(NAME retail_sound_variation COMMAND sound_variation_tests
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/sound-variation-fixtures.tsv)
    set_tests_properties(retail_sound_variation PROPERTIES SKIP_RETURN_CODE 77)
    if(NOT MSVC AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|amd64|i.86)$")
        add_executable(sound_variation_count32_tu15_tests tests/sound_variation_count32_tu15_tests.cpp)
        target_link_libraries(sound_variation_count32_tu15_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
        add_test(NAME sound_variation_count32_tu15_contract COMMAND sound_variation_count32_tu15_tests
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_variation_count32_tu15.json")
        set_tests_properties(sound_variation_count32_tu15_contract PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    endif()
    add_executable(sound_control_tests tests/sound_control_tests.cpp)
    target_link_libraries(sound_control_tests PRIVATE halo2_cache)
    add_executable(sound_packet_pitch_tests tests/sound_packet_pitch_tests.cpp)
    target_link_libraries(sound_packet_pitch_tests PRIVATE halo2_systems SDL2::SDL2)
    add_executable(sound_packet_gain_tests tests/sound_packet_gain_tests.cpp)
    target_link_libraries(sound_packet_gain_tests PRIVATE halo2_systems SDL2::SDL2)
    add_test(NAME original_sound_packet_gain COMMAND sound_packet_gain_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/triplicate.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_packet_gain.original.tsv)
    add_test(NAME original_sound_packet_pitch COMMAND sound_packet_pitch_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/triplicate.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_packet_pitch.original.tsv)
    add_executable(sound_distance_gain_tests tests/sound_distance_gain_tests.cpp)
    target_link_libraries(sound_distance_gain_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_distance_gain COMMAND sound_distance_gain_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_distance_gain.original.tsv)
    add_test(NAME original_sound_caller_control COMMAND sound_control_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/triplicate.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_control.original.tsv)
    add_executable(item_point_tests tests/item_point_tests.cpp)
    target_link_libraries(item_point_tests PRIVATE halo2_cache)
    add_test(NAME retail_item_point COMMAND item_point_tests
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/inventory-source/pickup-item-point/fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/item-point-metadata.tsv)
    set_tests_properties(retail_item_point PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(damage_history_tests tests/damage_history_tests.cpp)
    target_link_libraries(damage_history_tests PRIVATE halo2_systems)
    add_test(NAME retail_damage_history COMMAND damage_history_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/damage-history-fixtures.tsv)
    set_tests_properties(retail_damage_history PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(lethal_attribution_tests tests/lethal_attribution_tests.cpp)
    target_link_libraries(lethal_attribution_tests PRIVATE halo2_systems)
    add_test(NAME retail_lethal_attribution COMMAND lethal_attribution_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/lethal-attribution-fixtures.tsv)
    set_tests_properties(retail_lethal_attribution PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(lethal_match_tests tests/lethal_match_tests.cpp)
    target_link_libraries(lethal_match_tests PRIVATE halo2_systems)
    add_test(NAME retail_lethal_match COMMAND lethal_match_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/lethal-match-fixtures.tsv)
    set_tests_properties(retail_lethal_match PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(object_garbage_tests tests/object_garbage_tests.cpp)
    target_link_libraries(object_garbage_tests PRIVATE halo2_systems)
    add_test(NAME retail_object_garbage COMMAND object_garbage_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/object-garbage-fixtures.tsv)
    set_tests_properties(retail_object_garbage PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(owner_counter_tests tests/owner_counter_tests.cpp)
    target_link_libraries(owner_counter_tests PRIVATE halo2_systems)
    add_test(NAME retail_owner_counter COMMAND owner_counter_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/owner-counter-fixtures.tsv)
    set_tests_properties(retail_owner_counter PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(fp_registration_tests tests/fp_registration_tests.cpp)
    target_link_libraries(fp_registration_tests PRIVATE halo2_systems)
    add_test(NAME retail_fp_registration COMMAND fp_registration_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/fp-registration-fixtures.tsv)
    set_tests_properties(retail_fp_registration PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(remote_fp_binding_tests tests/remote_fp_binding_tests.cpp)
    target_link_libraries(remote_fp_binding_tests PRIVATE halo2_systems)
    add_test(NAME retail_remote_fp_binding COMMAND remote_fp_binding_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
    set_tests_properties(retail_remote_fp_binding PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(multiplayer_fp_network_tests tests/multiplayer_fp_network_tests.cpp)
    target_link_libraries(multiplayer_fp_network_tests PRIVATE halo2_systems)
    add_executable(weapon_effect_tests tests/weapon_effect_tests.cpp)
    target_link_libraries(weapon_effect_tests PRIVATE halo2_systems)
    add_test(NAME original_weapon_effect_request_visibility COMMAND weapon_effect_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/inventory-source/smg-effect-owner/native-fixtures/fixtures.txt)
    add_executable(unit_grenade_tests tests/unit_grenade_tests.cpp)
    target_link_libraries(unit_grenade_tests PRIVATE halo2_systems)
    add_test(NAME retail_unit_grenade_callbacks COMMAND unit_grenade_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/grenade-callbacks/measured/fixtures.tsv)
    set_tests_properties(retail_unit_grenade_callbacks PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_animation_tests tests/unit_animation_tests.cpp)
    target_link_libraries(unit_animation_tests PRIVATE halo2_systems)
    add_executable(unit_animation_definition_tests tests/unit_animation_definition_tests.cpp)
    target_link_libraries(unit_animation_definition_tests PRIVATE halo2_cache)
    add_test(NAME original_unit_animation_cache_definitions COMMAND unit_animation_definition_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/unit-animation-definitions-20261008/measured/fixtures.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/unit-animation-definitions-20261008/graph-metadata/measured/fixtures.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/inventory-source/unit-animation-clock/native-fixtures/fixtures.txt)
    set_tests_properties(original_unit_animation_cache_definitions PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_animation_metadata_store_tests tests/unit_animation_metadata_store_tests.cpp)
    target_link_libraries(unit_animation_metadata_store_tests PRIVATE halo2_cache)
    add_test(NAME original_unit_animation_metadata_store COMMAND unit_animation_metadata_store_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/unit-animation-definitions-20261008/graph-metadata/measured/fixtures.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/unit-animation-definitions-20261008/measured/fixtures.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "$ENV{HOME}/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_unit_animation_metadata_store PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_metadata_store_tests tests/sound_metadata_store_tests.cpp)
    target_link_libraries(sound_metadata_store_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_metadata_store COMMAND sound_metadata_store_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/sound-metadata-provider-20261008/measured/fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_sound_metadata_store PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_object_constants_tests tests/sound_object_constants_tests.cpp)
    target_link_libraries(sound_object_constants_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_object_constants COMMAND sound_object_constants_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/sound-object-constants-20261008/operands.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_sound_object_constants PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_action_tables_tests tests/unit_action_tables_tests.cpp)
    target_link_libraries(unit_action_tables_tests PRIVATE halo2_cache)
    add_test(NAME original_unit_action_tables COMMAND unit_action_tables_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/unit-action-tables-provider-20261008/measured/fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_unit_action_tables PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_constructor_constants_tests tests/unit_constructor_constants_tests.cpp)
    target_link_libraries(unit_constructor_constants_tests PRIVATE halo2_cache)
    add_test(NAME original_unit_constructor_constants COMMAND unit_constructor_constants_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/unit-constructor-constants-provider-20261008/measured/fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_unit_constructor_constants PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_driver_parameters_tests tests/sound_driver_parameters_tests.cpp)
    add_executable(sound_dsp_encoder_stereo_tu15_tests tests/sound_dsp_encoder_stereo_tu15_tests.cpp)
    target_link_libraries(sound_dsp_encoder_stereo_tu15_tests PRIVATE halo2_sound_dsp_encoder_stereo_tu15)
    add_test(NAME original_sound_dsp_encoder_stereo_tu15 COMMAND sound_dsp_encoder_stereo_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_dsp_encoder_stereo_tu15)
    set_tests_properties(original_sound_dsp_encoder_stereo_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_gp_image_tu15_tests tests/sound_gp_image_tu15_tests.cpp)
    target_link_libraries(sound_gp_image_tu15_tests PRIVATE halo2_sound_gp_image_tu15)
    add_executable(export_sound_gp_boot_inputs_tu15 tools/export_sound_gp_boot_inputs_tu15.cpp)
    target_link_libraries(export_sound_gp_boot_inputs_tu15 PRIVATE halo2_sound_gp_image_tu15)
    add_test(NAME original_sound_gp_image_tu15 COMMAND sound_gp_image_tu15_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_gp_image_tu15)
    set_tests_properties(original_sound_gp_image_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_startup_constants_tests tests/sound_startup_constants_tests.cpp)
    add_executable(sound_map_metadata_tests tests/sound_map_metadata_tests.cpp)
    add_executable(sound_mix_metadata_tests tests/sound_mix_metadata_tests.cpp)
    target_link_libraries(sound_mix_metadata_tests PRIVATE halo2_cache)
    add_test(NAME owned_sound_mix_metadata COMMAND sound_mix_metadata_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/sound-mix-metadata-provider-20261009/measured/fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(owned_sound_mix_metadata PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_map_cache_binding_tests tests/sound_map_cache_binding_tests.cpp)
    target_link_libraries(sound_map_cache_binding_tests PRIVATE halo2_cache)
    add_test(NAME owned_sound_map_cache_binding COMMAND sound_map_cache_binding_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/sound-map-activation-native/fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(owned_sound_map_cache_binding PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_cache_startup_constants_tests tests/sound_cache_startup_constants_tests.cpp)
    add_executable(sound_cache_startup_tests tests/sound_cache_startup_tests.cpp)
    add_executable(native_sound_storage_tests tests/native_sound_storage_tests.cpp)
    add_executable(sound_map_activation_tests tests/sound_map_activation_tests.cpp)
    target_link_libraries(sound_map_activation_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_map_activation COMMAND sound_map_activation_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/sound-map-activation-native/fixtures.tsv)
    add_executable(sound_map_activation_constants_tests tests/sound_map_activation_constants_tests.cpp)
    target_link_libraries(sound_map_activation_constants_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_map_activation_constants COMMAND sound_map_activation_constants_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/sound-map-activation-constants-provider-20261009/measured/constants.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_sound_map_activation_constants PROPERTIES SKIP_RETURN_CODE 77)
    if(UNIX)
        add_executable(sound_driver_startup_tests tests/sound_driver_startup_tests.cpp)
        target_link_libraries(sound_driver_startup_tests PRIVATE halo2_cache)
        add_test(NAME original_sound_driver_startup COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
            ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-driver-startup-native/fixtures.tsv
            $<TARGET_FILE:sound_driver_startup_tests>)
        set_tests_properties(original_sound_driver_startup PROPERTIES SKIP_RETURN_CODE 77)
    endif()
    add_executable(sound_driver_owners_tests tests/sound_driver_owners_tests.cpp)
    target_link_libraries(sound_driver_owners_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_driver_owners COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-driver-owners-native/fixtures.tsv
        $<TARGET_FILE:sound_driver_owners_tests>)
    set_tests_properties(original_sound_driver_owners PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_driver_cleanup_tests tests/sound_driver_cleanup_tests.cpp)
    target_link_libraries(sound_driver_cleanup_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_driver_cleanup COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-driver-cleanup-production-20261009/measured/fixtures.tsv
        $<TARGET_FILE:sound_driver_cleanup_tests>)
    set_tests_properties(original_sound_driver_cleanup PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_driver_listener_tests tests/sound_driver_listener_tests.cpp)
    target_link_libraries(sound_driver_listener_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_driver_listener COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-driver-listener-native/fixtures.tsv
        $<TARGET_FILE:sound_driver_listener_tests>)
    set_tests_properties(original_sound_driver_listener PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_submit_tests tests/sound_submit_tests.cpp)
    target_link_libraries(sound_submit_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_submit COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-submit-native/fixtures.tsv
        $<TARGET_FILE:sound_submit_tests>
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-submit-native/literal-lockout-regions.bin)
    set_tests_properties(original_sound_submit PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_promotion_tests tests/sound_promotion_tests.cpp)
    target_link_libraries(sound_promotion_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_promotion COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-promotion-native/fixtures.tsv
        $<TARGET_FILE:sound_promotion_tests>)
    set_tests_properties(original_sound_promotion PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_promotion_constants_tests tests/sound_promotion_constants_tests.cpp)
    target_link_libraries(sound_promotion_constants_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_promotion_constants COMMAND sound_promotion_constants_tests
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-promotion-constants-provider-20261009/measured/constants.tsv
        ${CMAKE_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_sound_promotion_constants PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_promotion_map_owner_tests tests/sound_promotion_map_owner_tests.cpp)
    target_link_libraries(sound_promotion_map_owner_tests PRIVATE halo2_cache)
    add_test(NAME owned_sound_promotion_map_owner COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-promotion-native/fixtures.tsv
        $<TARGET_FILE:sound_promotion_map_owner_tests>)
    set_tests_properties(owned_sound_promotion_map_owner PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_create_tests tests/sound_create_tests.cpp)
    target_link_libraries(sound_create_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_create COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-create-native/fixtures.tsv
        $<TARGET_FILE:sound_create_tests>)
    set_tests_properties(original_sound_create PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_admission_tests tests/sound_admission_tests.cpp)
    target_link_libraries(sound_admission_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_admission COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-admission-native/fixtures/fixtures.tsv
        $<TARGET_FILE:sound_admission_tests>)
    set_tests_properties(original_sound_admission PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(live_sound_admission_tests tests/live_sound_admission_tests.cpp)
    target_link_libraries(live_sound_admission_tests PRIVATE halo2_cache)
    add_test(NAME original_live_sound_admission COMMAND live_sound_admission_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/live_sound_admission.original.tsv)
    add_executable(live_sound_spatial_tests tests/live_sound_spatial_tests.cpp)
    target_link_libraries(live_sound_spatial_tests PRIVATE halo2_cache)
    add_test(NAME original_live_sound_spatial COMMAND live_sound_spatial_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/live_sound_spatial.original.tsv)
    add_executable(sound_driver_spatial_tests tests/sound_driver_spatial_tests.cpp)
    target_link_libraries(sound_driver_spatial_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_driver_spatial COMMAND sound_driver_spatial_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_driver_spatial.original.tsv)
    add_executable(sound_sdk_spatial_tests tests/sound_sdk_spatial_tests.cpp)
    target_link_libraries(sound_sdk_spatial_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_sdk_spatial COMMAND sound_sdk_spatial_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_spatial.original.json)
    add_executable(sound_sdk_spatial_data_tests tests/sound_sdk_spatial_data_tests.cpp)
    target_link_libraries(sound_sdk_spatial_data_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_sdk_spatial_data COMMAND sound_sdk_spatial_data_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_spatial_data.original.json
        ${CMAKE_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_sound_sdk_spatial_data PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_sdk_hrtf_tests tests/sound_sdk_hrtf_tests.cpp)
    target_link_libraries(sound_sdk_hrtf_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_sdk_hrtf COMMAND sound_sdk_hrtf_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_hrtf.original.json
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_spatial_data.original.json)
    add_executable(sound_sdk_calculate_tests tests/sound_sdk_calculate_tests.cpp)
    target_link_libraries(sound_sdk_calculate_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_sdk_calculate COMMAND sound_sdk_calculate_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_calculate.original.json
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_spatial_data.original.json)
    add_executable(sound_sdk_voice_data_tests tests/sound_sdk_voice_data_tests.cpp)
    target_link_libraries(sound_sdk_voice_data_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_sdk_voice_data COMMAND sound_sdk_voice_data_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_voice_data.original.json
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_spatial_data.original.json)
    add_executable(sound_sdk_device_tests tests/sound_sdk_device_tests.cpp)
    target_link_libraries(sound_sdk_device_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_sdk_device COMMAND sound_sdk_device_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_device.original.json)
    add_executable(sound_sdk_commit_tests tests/sound_sdk_commit_tests.cpp)
    target_link_libraries(sound_sdk_commit_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_sdk_commit COMMAND sound_sdk_commit_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_commit.original.json
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_spatial_data.original.json)
    add_executable(sound_sdk_setters_tests tests/sound_sdk_setters_tests.cpp)
    target_link_libraries(sound_sdk_setters_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_sdk_setters COMMAND sound_sdk_setters_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_setters.original.json)
    add_executable(sound_sdk_construction_tests tests/sound_sdk_construction_tests.cpp)
    target_link_libraries(sound_sdk_construction_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_sdk_construction COMMAND sound_sdk_construction_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_construction.original.json)
    add_executable(sound_sdk_voice_initialization_tests tests/sound_sdk_voice_initialization_tests.cpp)
    target_link_libraries(sound_sdk_voice_initialization_tests PRIVATE halo2_cache halo2_native_synchronization)
    add_test(NAME original_sound_sdk_voice_initialization COMMAND sound_sdk_voice_initialization_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_voice_initialization.original.json)
    add_executable(sound_sdk_hardware_voice_tests tests/sound_sdk_hardware_voice_tests.cpp)
    target_link_libraries(sound_sdk_hardware_voice_tests PRIVATE halo2_cache halo2_native_synchronization)
    add_test(NAME original_sound_sdk_hardware_voice COMMAND sound_sdk_hardware_voice_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_hardware_voice.original.json
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_spatial_data.original.json)
    add_executable(sound_sdk_voice_dispatch_tests tests/sound_sdk_voice_dispatch_tests.cpp)
    target_link_libraries(sound_sdk_voice_dispatch_tests PRIVATE halo2_cache halo2_native_synchronization)
    add_test(NAME original_sound_sdk_voice_dispatch COMMAND sound_sdk_voice_dispatch_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_voice_dispatch.original.json
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_spatial_data.original.json)
    add_executable(sound_sdk_sample_pages_tests tests/sound_sdk_sample_pages_tests.cpp)
    target_link_libraries(sound_sdk_sample_pages_tests PRIVATE halo2_cache halo2_native_synchronization)
    add_test(NAME original_sound_sdk_sample_pages COMMAND sound_sdk_sample_pages_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_sample_pages.original.json)
    add_executable(sound_sdk_voice_attachment_tests tests/sound_sdk_voice_attachment_tests.cpp)
    target_link_libraries(sound_sdk_voice_attachment_tests PRIVATE halo2_cache halo2_native_synchronization)
    add_test(NAME original_sound_sdk_voice_attachment COMMAND sound_sdk_voice_attachment_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_voice_attachment.original.json)
    add_executable(sound_sdk_voice_retirement_tests tests/sound_sdk_voice_retirement_tests.cpp)
    target_link_libraries(sound_sdk_voice_retirement_tests PRIVATE halo2_cache halo2_native_synchronization)
    add_test(NAME original_sound_sdk_voice_retirement COMMAND sound_sdk_voice_retirement_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_voice_retirement.original.json)
    add_executable(sound_sdk_notifications_tests tests/sound_sdk_notifications_tests.cpp)
    target_link_libraries(sound_sdk_notifications_tests PRIVATE halo2_cache halo2_native_synchronization)
    add_test(NAME original_sound_sdk_notifications COMMAND sound_sdk_notifications_tests
        ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_sdk_notifications.original.json)
    add_executable(sound_base_constants_tests tests/sound_base_constants_tests.cpp)
    target_link_libraries(sound_base_constants_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_base_constants COMMAND sound_base_constants_tests
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/base-sound-parameters-20261012/constants.tsv
        ${CMAKE_SOURCE_DIR}/assets/disc/default.xbe)
    set_tests_properties(original_sound_base_constants PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_create_constants_tests tests/sound_create_constants_tests.cpp)
    target_link_libraries(sound_create_constants_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_create_constants COMMAND sound_create_constants_tests
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-create-constants-provider-20261009/measured/constants.tsv
        ${CMAKE_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_sound_create_constants PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_driver_stream_tests tests/sound_driver_stream_tests.cpp)
    target_link_libraries(sound_driver_stream_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_driver_stream COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-driver-stream-native/fixtures.tsv
        $<TARGET_FILE:sound_driver_stream_tests>)
    set_tests_properties(original_sound_driver_stream PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_driver_stream_constants_tests tests/sound_driver_stream_constants_tests.cpp)
    target_link_libraries(sound_driver_stream_constants_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_driver_stream_constants COMMAND sound_driver_stream_constants_tests
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-driver-stream-constants-provider-20261009/measured/constants.tsv
        ${CMAKE_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_sound_driver_stream_constants PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_driver_master_gain_tests tests/sound_driver_master_gain_tests.cpp)
    target_link_libraries(sound_driver_master_gain_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_driver_master_gain COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-driver-master-gain-native/fixtures/fixtures.tsv
        $<TARGET_FILE:sound_driver_master_gain_tests>)
    set_tests_properties(original_sound_driver_master_gain PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_looping_tests tests/sound_looping_tests.cpp)
    target_link_libraries(sound_looping_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_looping COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-looping-native/fixtures-v3/fixtures.tsv
        $<TARGET_FILE:sound_looping_tests>)
    set_tests_properties(original_sound_looping PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_driver_master_gain_constants_tests tests/sound_driver_master_gain_constants_tests.cpp)
    target_link_libraries(sound_driver_master_gain_constants_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_driver_master_gain_constants COMMAND sound_driver_master_gain_constants_tests
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-driver-master-gain-native/fixtures/constants.tsv
        ${CMAKE_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_sound_driver_master_gain_constants PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(object_construction_tables_tests tests/object_construction_tables_tests.cpp)
    target_link_libraries(object_construction_tables_tests PRIVATE halo2_systems)
    add_test(NAME original_object_construction_tables COMMAND object_construction_tables_tests
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/object-construction-provider-20261009/measured/tables.tsv
        ${CMAKE_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_object_construction_tables PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(looping_construction_operands_tests tests/looping_construction_operands_tests.cpp)
    target_link_libraries(looping_construction_operands_tests PRIVATE halo2_cache)
    add_test(NAME original_looping_construction_operands COMMAND looping_construction_operands_tests
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/looping-construction-operands-20261009/measured/operands.tsv
        ${CMAKE_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_looping_construction_operands PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(object_model_resources_tests tests/object_model_resources_tests.cpp)
    target_link_libraries(object_model_resources_tests PRIVATE halo2_systems)
    add_test(NAME original_object_model_resources COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/object-model-resources-native/fixtures/fixtures.tsv
        $<TARGET_FILE:object_model_resources_tests>)
    set_tests_properties(original_object_model_resources PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(object_physics_construction_tests tests/object_physics_construction_tests.cpp)
    target_link_libraries(object_physics_construction_tests PRIVATE halo2_systems)
    add_test(NAME original_object_physics_construction COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/object-physics-construction-native/fixtures/fixtures.tsv
        $<TARGET_FILE:object_physics_construction_tests>)
    set_tests_properties(original_object_physics_construction PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_component_construction_tests tests/physics_component_construction_tests.cpp)
    target_link_libraries(physics_component_construction_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_component_construction COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/physics-component-construction-native/fixtures-v2/fixtures.tsv
        $<TARGET_FILE:physics_component_construction_tests>)
    set_tests_properties(original_physics_component_construction PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_component_rebuild_tests tests/physics_component_rebuild_tests.cpp)
    target_link_libraries(physics_component_rebuild_tests PRIVATE halo2_systems)
    add_test(NAME original_physics_component_rebuild COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/physics-component-rebuild-native/fixtures/fixtures.tsv
        $<TARGET_FILE:physics_component_rebuild_tests>)
    set_tests_properties(original_physics_component_rebuild PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(physics_construction_operands_tests tests/physics_construction_operands_tests.cpp)
    target_link_libraries(physics_construction_operands_tests PRIVATE halo2_cache)
    add_test(NAME original_physics_construction_operands COMMAND physics_construction_operands_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/physics-component-rebuild-native/immutable-operands/operands.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_physics_construction_operands PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(fp_sound_submit_tests tests/fp_sound_submit_tests.cpp)
    target_link_libraries(fp_sound_submit_tests PRIVATE halo2_cache)
    add_test(NAME original_fp_sound_submit_composition COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/fp-sound-submit-composition/fixtures-final/fixtures.tsv
        $<TARGET_FILE:fp_sound_submit_tests>
        ${CMAKE_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    set_tests_properties(original_fp_sound_submit_composition PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_capabilities_tests tests/sound_capabilities_tests.cpp)
    target_link_libraries(sound_capabilities_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_capabilities COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-capabilities-native/fixtures-v2/fixtures.tsv
        $<TARGET_FILE:sound_capabilities_tests>)
    set_tests_properties(original_sound_capabilities PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_cache_request_tests tests/sound_cache_request_tests.cpp)
    target_link_libraries(sound_cache_request_tests PRIVATE halo2_systems halo2_native_synchronization)
    add_test(NAME original_sound_cache_request COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-cache-request-native/fixtures.tsv
        $<TARGET_FILE:sound_cache_request_tests>
        ${CMAKE_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(original_sound_cache_request PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_cache_lifecycle_tests tests/sound_cache_lifecycle_tests.cpp)
    target_link_libraries(sound_cache_lifecycle_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_cache_lifecycle COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-cache-lifecycle-native/fixtures.tsv
        $<TARGET_FILE:sound_cache_lifecycle_tests>)
    set_tests_properties(original_sound_cache_lifecycle PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_driver_update_tests tests/sound_driver_update_tests.cpp)
    target_link_libraries(sound_driver_update_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_driver_update COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-driver-update-native/fixtures.tsv
        $<TARGET_FILE:sound_driver_update_tests>)
    set_tests_properties(original_sound_driver_update PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_create_cache_tests tests/sound_create_cache_tests.cpp)
    target_link_libraries(sound_create_cache_tests PRIVATE halo2_systems halo2_native_synchronization)
    add_test(NAME original_sound_create_cache COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/run_owned_fixture_test.py
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-create-cache-native/fixtures.tsv
        $<TARGET_FILE:sound_create_cache_tests>
        ${CMAKE_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(original_sound_create_cache PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_driver_owner_operands_tests tests/sound_driver_owner_operands_tests.cpp)
    target_link_libraries(sound_driver_owner_operands_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_driver_owner_operands COMMAND sound_driver_owner_operands_tests
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-driver-owner-operands-provider-20261009/measured/operands.tsv
        ${CMAKE_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_sound_driver_owner_operands PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_driver_startup_operands_tests tests/sound_driver_startup_operands_tests.cpp)
    target_link_libraries(sound_driver_startup_operands_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_driver_startup_operands COMMAND sound_driver_startup_operands_tests
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-driver-startup-operands-provider-20261009/measured/operands.tsv
        ${CMAKE_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_sound_driver_startup_operands PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(native_sound_storage_tests PRIVATE halo2_cache)
    add_test(NAME native_sound_storage COMMAND native_sound_storage_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/sound-cache-startup-native/fixtures.tsv)
    target_link_libraries(sound_cache_startup_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_cache_startup COMMAND sound_cache_startup_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/sound-cache-startup-native/fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/sound-startup-native/fixtures.tsv)
    set_tests_properties(original_sound_cache_startup PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(sound_cache_startup_constants_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_cache_startup_constants COMMAND sound_cache_startup_constants_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/sound-cache-startup-constants-provider-20261009/measured/constants.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_sound_cache_startup_constants PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(sound_map_metadata_tests PRIVATE halo2_cache)
    add_test(NAME owned_sound_map_metadata COMMAND sound_map_metadata_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/sound-map-metadata-provider-20261009/measured/fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(owned_sound_map_metadata PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_startup_tests tests/sound_startup_tests.cpp)
    target_link_libraries(sound_startup_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_startup COMMAND sound_startup_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/sound-startup-native/fixtures.tsv)
    set_tests_properties(original_sound_startup PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(sound_startup_constants_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_startup_constants COMMAND sound_startup_constants_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/sound-startup-constants-provider-20261008/measured/constants.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_sound_startup_constants PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(sound_driver_parameters_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_driver_parameters COMMAND sound_driver_parameters_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/sound-platform-voice-contract/scalar-fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/sound-driver-parameters-provider-20261008/measured/constants.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_sound_driver_parameters PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_sound_base_parameters COMMAND sound_driver_parameters_tests --selected-image
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/sound-platform-voice-contract/scalar-fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe)
    set_tests_properties(original_sound_base_parameters PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_tag_metadata_store_tests tests/source_tag_metadata_store_tests.cpp)
    target_link_libraries(source_tag_metadata_store_tests PRIVATE halo2_cache)
    add_test(NAME original_source_tag_metadata_store COMMAND source_tag_metadata_store_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/source-tag-metadata-provider-20261008/measured/fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(original_source_tag_metadata_store PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_tag_metadata_store_mcc_tests tests/source_tag_metadata_store_mcc_tests.cpp)
    target_link_libraries(source_tag_metadata_store_mcc_tests PRIVATE halo2_cache)
    set(HALO2_MCC_SOURCE_TAG_DONOR "" CACHE FILEPATH "Optional actual MCC introduction map for retained source metadata validation")
    add_test(NAME mcc_source_tag_metadata_store COMMAND source_tag_metadata_store_mcc_tests "${HALO2_MCC_SOURCE_TAG_DONOR}")
    set_tests_properties(mcc_source_tag_metadata_store PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(mcc_particle_movement_tests tests/mcc_particle_movement_tests.cpp)
    target_link_libraries(mcc_particle_movement_tests PRIVATE halo2_cache)
    set(HALO2_MCC_PARTICLE_MOVEMENT_MAP "" CACHE FILEPATH "Prepared MCC map for movement controller metadata validation")
    add_test(NAME mcc_particle_movement COMMAND mcc_particle_movement_tests "${HALO2_MCC_PARTICLE_MOVEMENT_MAP}")
    set_tests_properties(mcc_particle_movement PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(campaign_character_pass_composition_tests tests/campaign_character_pass_composition_tests.cpp)
    target_link_libraries(campaign_character_pass_composition_tests PRIVATE halo2_cache)
    add_test(NAME campaign_character_material_fog_phase COMMAND campaign_character_pass_composition_tests --phase-only)
    add_test(NAME mcc_campaign_character_pass_composition COMMAND campaign_character_pass_composition_tests "${HALO2_MCC_SOURCE_TAG_DONOR}")
    set_tests_properties(mcc_campaign_character_pass_composition PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(sound_object_request_tests tests/sound_object_request_tests.cpp)
    target_link_libraries(sound_object_request_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_object_request COMMAND sound_object_request_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/sound-object-request-native/fixtures.tsv)
    set_tests_properties(original_sound_object_request PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_object_cache_binding_tests tests/sound_object_cache_binding_tests.cpp)
    target_link_libraries(sound_object_cache_binding_tests PRIVATE halo2_cache)
    add_test(NAME owned_sound_object_cache_binding COMMAND sound_object_cache_binding_tests
        ${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/sound-object-request-native/fixtures.tsv
        ${CMAKE_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(owned_sound_object_cache_binding PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(fp_sound_request_tests tests/fp_sound_request_tests.cpp)
    target_compile_definitions(fp_sound_request_tests PRIVATE HALO2_FP_SOUND_CACHE_TESTS=1)
    target_link_libraries(fp_sound_request_tests PRIVATE halo2_cache)
    # Original comparisons require the local ignored authority fixture/map.
    # The native request module itself builds without original assets.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/fp-sound-request-native/canonical-fixtures.tsv"
       AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
        add_test(NAME original_first_person_sound_request COMMAND fp_sound_request_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/fp-sound-request-native/canonical-fixtures.tsv
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
    endif()
    add_executable(animation_variant_tests tests/animation_variant_tests.cpp)
    target_link_libraries(animation_variant_tests PRIVATE halo2_systems halo2_cache)
    target_compile_definitions(animation_variant_tests PRIVATE HALO2_VARIANT_CACHE_TESTS=1)
    add_test(NAME original_animation_variant COMMAND animation_variant_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/animation-variant/native/fixtures.txt
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "$ENV{HOME}/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_animation_variant PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_unit_animation_manager COMMAND unit_animation_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/inventory-source/unit-animation-clock/native-fixtures/fixtures.txt)
    add_test(NAME original_owned_unit_animation_manager COMMAND unit_animation_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/inventory-source/unit-animation-clock/owned-fixtures/fixtures.txt)
    add_executable(unit_animation_pose_tests tests/unit_animation_pose_tests.cpp)
    target_link_libraries(unit_animation_pose_tests PRIVATE halo2_systems)
    add_test(NAME original_unit_animation_pose_delivery COMMAND unit_animation_pose_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/inventory-source/unit-animation-clock/pose/native-fixtures/fixtures.txt)
    set_tests_properties(original_unit_animation_manager original_owned_unit_animation_manager
        original_unit_animation_pose_delivery PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(fp_clock_tests tests/fp_clock_tests.cpp)
    target_link_libraries(fp_clock_tests PRIVATE halo2_systems)
    add_test(NAME retail_fp_clock COMMAND fp_clock_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/fp-clock-fixtures.tsv)
    set_tests_properties(retail_fp_clock PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(multiplayer_sound_resources_tests tests/multiplayer_sound_resources_tests.cpp)
    target_link_libraries(multiplayer_sound_resources_tests PRIVATE halo2_systems)
    add_test(NAME owned_multiplayer_first_person_sound_resources COMMAND multiplayer_sound_resources_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/reported-failures-20261008/sound-resources/measured/fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(owned_multiplayer_first_person_sound_resources PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(multiplayer_all_sound_resources_tests tests/multiplayer_all_sound_resources_tests.cpp)
    target_link_libraries(multiplayer_all_sound_resources_tests PRIVATE halo2_systems)
    add_test(NAME owned_multiplayer_all_sound_resources COMMAND multiplayer_all_sound_resources_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/reported-failures-20261008/all-sound-resources/measured/fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(owned_multiplayer_all_sound_resources PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME owned_multiplayer_latest_dlc_sound_resources COMMAND multiplayer_all_sound_resources_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/reported-failures-final-dlc-20261008/measured/fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(owned_multiplayer_latest_dlc_sound_resources PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_animation_presentation COMMAND fp_clock_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/fp-clock-fixtures.tsv" --presentation)
    set_tests_properties(retail_animation_presentation PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(smg_damage_tests tests/smg_damage_tests.cpp)
    target_link_libraries(smg_damage_tests PRIVATE halo2_systems)
    add_test(NAME retail_smg_damage COMMAND smg_damage_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/smg-damage-fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/smg-feedback-live.tsv)
    set_tests_properties(retail_smg_damage PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_damage_effects_tests tests/unit_damage_effects_tests.cpp)
    target_link_libraries(unit_damage_effects_tests PRIVATE halo2_systems)
    add_test(NAME original_unit_damage_effects COMMAND unit_damage_effects_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/unit-damage-effects.tsv)
    set_tests_properties(original_unit_damage_effects PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_vitality_update_tests tests/unit_vitality_update_tests.cpp)
    target_link_libraries(unit_vitality_update_tests PRIVATE halo2_systems)
    add_test(NAME original_unit_vitality_update COMMAND unit_vitality_update_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/shield-break-recharge-live/ordinary-recharge-transitions.tsv)
    set_tests_properties(original_unit_vitality_update PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(damage_feedback_tests tests/damage_feedback_tests.cpp)
    target_link_libraries(damage_feedback_tests PRIVATE halo2_systems)
    add_test(NAME original_damage_feedback COMMAND damage_feedback_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/damage-feedback.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/shield-function-inputs.tsv)
    set_tests_properties(original_damage_feedback PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(shield_overlay_tests tests/shield_overlay_tests.cpp)
    target_link_libraries(shield_overlay_tests PRIVATE halo2_systems)
    add_test(NAME original_shield_overlay COMMAND shield_overlay_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/shield-overlay.tsv)
    set_tests_properties(original_shield_overlay PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(equipment_material_tests tests/equipment_material_tests.cpp)
    target_link_libraries(equipment_material_tests PRIVATE halo2_cache)
    add_test(NAME original_equipment_materials COMMAND equipment_material_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps)
    set_tests_properties(original_equipment_materials PROPERTIES SKIP_RETURN_CODE 77)
    set(HALO2_MCC_EQUIPMENT_MAPS "" CACHE PATH "Prepared MCC maps directory for equipment shader validation")
    add_test(NAME mcc_equipment_materials COMMAND equipment_material_tests "${HALO2_MCC_EQUIPMENT_MAPS}"
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps)
    set_tests_properties(mcc_equipment_materials PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(equipment_runtime_tu15_tests tests/equipment_runtime_tu15_tests.cpp)
    target_link_libraries(equipment_runtime_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    set(HALO2_MCC_EQUIPMENT_MAP "" CACHE FILEPATH "Prepared MCC Headlong map for equipment pickup acceptance")
    add_test(NAME native_mcc_equipment_pickups COMMAND equipment_runtime_tu15_tests
        "${HALO2_MCC_EQUIPMENT_MAP}" ${CMAKE_CURRENT_BINARY_DIR}/equipment-pickups.json)
    set_tests_properties(native_mcc_equipment_pickups PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)
    add_test(NAME native_mcc_powerups_all_maps COMMAND equipment_runtime_tu15_tests
        "${HALO2_MCC_EQUIPMENT_MAPS}" ${CMAKE_CURRENT_BINARY_DIR}/equipment-all-map-powerups.json --all-maps)
    set_tests_properties(native_mcc_powerups_all_maps PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 1200)
    add_executable(equipment_ammo_runtime_tests tests/equipment_ammo_runtime_tests.cpp)
    target_link_libraries(equipment_ammo_runtime_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_mcc_equipment_ammo COMMAND equipment_ammo_runtime_tests
        "${HALO2_MCC_EQUIPMENT_MAP}" ${CMAKE_CURRENT_BINARY_DIR}/equipment-ammo.json)
    set_tests_properties(native_mcc_equipment_ammo PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)
    add_executable(source_shield_material_tests tests/source_shield_material_tests.cpp)
    target_link_libraries(source_shield_material_tests PRIVATE halo2_systems)
    add_test(NAME original_shield_material COMMAND source_shield_material_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/shield-material.tsv)
    set_tests_properties(original_shield_material PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_overshield_material COMMAND source_shield_material_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/overshield_material_original.tsv)
    set_tests_properties(original_overshield_material PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_overshield_camouflage_material COMMAND source_shield_material_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/overshield_camouflage_material_original.tsv --camouflage)
    set_tests_properties(original_overshield_camouflage_material PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_shield_model_shaders_tests tests/source_shield_model_shaders_tests.cpp)
    target_link_libraries(source_shield_model_shaders_tests PRIVATE halo2_cache)
    target_compile_definitions(source_shield_model_shaders_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_shield_override_parts COMMAND source_shield_model_shaders_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/shield-override-parts.tsv)
    set_tests_properties(original_shield_override_parts PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(smg_ready_tests tests/smg_ready_tests.cpp)
    target_link_libraries(smg_ready_tests PRIVATE halo2_systems)
    add_test(NAME retail_smg_ready COMMAND smg_ready_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/smg-ready-fixtures.tsv)
    set_tests_properties(retail_smg_ready PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(smg_equipped_tests tests/smg_equipped_tests.cpp)
    target_link_libraries(smg_equipped_tests PRIVATE halo2_systems)
    add_test(NAME retail_smg_equipped COMMAND smg_equipped_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/smg-equipped-fixtures.tsv)
    set_tests_properties(retail_smg_equipped PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(smg_prepared_effect_tests tests/smg_prepared_effect_tests.cpp)
    target_link_libraries(smg_prepared_effect_tests PRIVATE halo2_systems)
    add_test(NAME original_smg_prepared_effect COMMAND smg_prepared_effect_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/smg-prepared-effect-20261008/fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/smg_complete_effect_calls_original.tsv)
    set_tests_properties(original_smg_prepared_effect PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(datum_pool_tests tests/datum_pool_tests.cpp)
    add_executable(native_object_lifetimes_tests tests/native_object_lifetimes_tests.cpp)
    target_link_libraries(native_object_lifetimes_tests PRIVATE halo2_systems)
    add_test(NAME native_object_lifetimes COMMAND native_object_lifetimes_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    add_executable(source_object_body_storage_tests tests/source_object_body_storage_tests.cpp)
    target_link_libraries(source_object_body_storage_tests PRIVATE halo2_systems)
    add_test(NAME original_source_object_body_storage COMMAND source_object_body_storage_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/body-storage/measured/storage.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/body-storage/measured/extents.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(original_source_object_body_storage PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(datum_pool_tests PRIVATE halo2_systems)
    add_test(NAME retail_datum_pool COMMAND datum_pool_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/datum-pool-fixtures.tsv)
    set_tests_properties(retail_datum_pool PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_source_bound_datum_pool COMMAND datum_pool_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/datum-pool-fixtures.tsv source-bound)
    set_tests_properties(retail_source_bound_datum_pool PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_source_datum_pool_continuation COMMAND datum_pool_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/datum-pool-fixtures.tsv source-continuation)
    set_tests_properties(retail_source_datum_pool_continuation PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(multiplayer_identity_tests tests/multiplayer_identity_tests.cpp)
    target_link_libraries(multiplayer_identity_tests PRIVATE halo2_systems)
    add_test(NAME retail_multiplayer_identity COMMAND multiplayer_identity_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/player-identity-fixture.tsv)
    set_tests_properties(retail_multiplayer_identity PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(respawn_timing_tests PRIVATE halo2_systems)
    add_test(NAME retail_respawn_timing COMMAND respawn_timing_tests
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/respawn-timing-fixtures.tsv)
    set_tests_properties(retail_respawn_timing PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(netgame_weapon_tests tests/netgame_weapon_tests.cpp)
    target_link_libraries(netgame_weapon_tests PRIVATE halo2_systems)
    add_test(NAME retail_netgame_weapons COMMAND netgame_weapon_tests
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/netgame-weapons-fixtures.tsv)
    set_tests_properties(retail_netgame_weapons PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(spawn_match_tests PRIVATE halo2_systems)
    add_test(NAME retail_spawn_match COMMAND spawn_match_tests
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-map-fixtures.tsv)
    set_tests_properties(retail_spawn_match PROPERTIES SKIP_RETURN_CODE 77)
endif()
if(BUILD_TESTING)
    add_executable(hud_tests tests/hud_tests.cpp)
    add_executable(hud_vista_artwork_tests tests/hud_vista_artwork_tests.cpp)
    target_link_libraries(hud_vista_artwork_tests PRIVATE halo2_systems)
    add_executable(hud_vista_crosshair_tests tests/hud_vista_crosshair_tests.cpp)
    target_link_libraries(hud_vista_crosshair_tests PRIVATE halo2_systems)
    add_executable(mcc_hud_import_tests tests/mcc_hud_import_tests.cpp)
    target_link_libraries(mcc_hud_import_tests PRIVATE halo2_systems)
    add_test(NAME mcc_classic_hud_import COMMAND mcc_hud_import_tests
        "${CMAKE_SOURCE_DIR}/assets/disc/maps" "${CMAKE_SOURCE_DIR}/assets/mcc-classic-hud")
    set_tests_properties(mcc_classic_hud_import PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(hud_gameplay_tests tests/hud_gameplay_tests.cpp)
    target_link_libraries(hud_gameplay_tests PRIVATE halo2_systems)
    add_executable(hud_action_prompt_tests tests/hud_action_prompt_tests.cpp)
    target_link_libraries(hud_action_prompt_tests PRIVATE halo2_systems)
    target_compile_definitions(hud_action_prompt_tests PRIVATE
        HALO2_ACTION_PROMPT_TEMPLATE_REFERENCE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mcc_action_prompt_templates.json")
    add_executable(action_prompt_bindings_tests tests/action_prompt_bindings_tests.cpp
        src/ui/hud/action_prompt_bindings.cpp src/input/controller_settings.cpp src/input/pc_settings.cpp)
    target_include_directories(action_prompt_bindings_tests PRIVATE include)
    target_link_libraries(action_prompt_bindings_tests PRIVATE SDL2::SDL2)
    add_test(NAME native_action_prompt_bindings COMMAND action_prompt_bindings_tests)
    add_executable(controller_prompt_hud_tests tests/controller_prompt_hud_tests.cpp)
    target_link_libraries(controller_prompt_hud_tests PRIVATE halo2_systems)
    set(HALO2_CONTROLLER_PROMPT_TEST_MAPS "" CACHE PATH "Prepared MCC maps for mixed-player prompt rendering")
    if(HALO2_CONTROLLER_PROMPT_TEST_MAPS)
        add_test(NAME mixed_player_controller_prompt_hud COMMAND ${Python3_EXECUTABLE} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_controller_prompt_hud_test.py
            $<TARGET_FILE:controller_prompt_hud_tests> "${HALO2_CONTROLLER_PROMPT_TEST_MAPS}")
    endif()
    add_test(NAME controller_prompt_artwork COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_controller_prompt_artwork.py)
    add_test(NAME controller_prompt_packaging COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_controller_prompt_assets.py)
    add_test(NAME native_action_prompt_template_reference COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_action_prompt_templates.py --check)
    add_executable(audit_action_prompt_templates tools/audit_action_prompt_templates.cpp)
    target_link_libraries(audit_action_prompt_templates PRIVATE halo2_cache)
    add_test(NAME native_hud_action_prompts_display COMMAND hud_action_prompt_tests --display
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(native_hud_action_prompts_display PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    set(HALO2_MCC_ACTION_PROMPT_MAPS "" CACHE PATH "Owned prepared MCC maps for action prompt display validation")
    if(HALO2_MCC_ACTION_PROMPT_MAPS)
        add_test(NAME native_hud_action_prompts_display_mcc COMMAND hud_action_prompt_tests --display
            "${HALO2_MCC_ACTION_PROMPT_MAPS}")
        set_tests_properties(native_hud_action_prompts_display_mcc PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    endif()
    add_executable(source_unit_interaction_target_tests tests/source_unit_interaction_target_tests.cpp)
    target_link_libraries(source_unit_interaction_target_tests PRIVATE halo2_systems)
    add_test(NAME source_unit_interaction_owner_guards COMMAND source_unit_interaction_target_tests)
    add_executable(campaign_weapon_prompt_tests tests/campaign_weapon_prompt_tests.cpp)
    target_link_libraries(campaign_weapon_prompt_tests PRIVATE halo2_systems)
    add_test(NAME native_campaign_weapon_prompts COMMAND campaign_weapon_prompt_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/mcc-classic-hud)
    set_tests_properties(native_campaign_weapon_prompts PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(campaign_action_prompt_tests tests/campaign_action_prompt_tests.cpp)
    target_link_libraries(campaign_action_prompt_tests PRIVATE halo2_systems)
    add_test(NAME native_campaign_action_prompts COMMAND campaign_action_prompt_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/mcc-classic-hud)
    set_tests_properties(native_campaign_action_prompts PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(hud_action_prompt_catalog_tests tests/hud_action_prompt_catalog_tests.cpp)
    target_link_libraries(hud_action_prompt_catalog_tests PRIVATE halo2_systems)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/zanzibar.map")
        add_test(NAME native_hud_action_prompts COMMAND hud_action_prompt_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/asset-sources.tsv")
            file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/assets/asset-sources.tsv" HALO2_PROMPT_ASSET_ROWS REGEX "^vista\t")
            if(HALO2_PROMPT_ASSET_ROWS)
                list(GET HALO2_PROMPT_ASSET_ROWS 0 HALO2_PROMPT_VISTA_ROW)
                string(REGEX REPLACE "^vista\t" "" HALO2_PROMPT_VISTA_ASSETS "${HALO2_PROMPT_VISTA_ROW}")
                add_test(NAME native_hud_action_prompts_vista COMMAND hud_action_prompt_tests
                    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps "${HALO2_PROMPT_VISTA_ASSETS}")
                add_test(NAME native_hud_action_prompts_display_vista COMMAND hud_action_prompt_tests --display
                    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps "${HALO2_PROMPT_VISTA_ASSETS}")
                set_tests_properties(native_hud_action_prompts_display_vista PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
                add_test(NAME native_hud_action_prompt_catalog COMMAND hud_action_prompt_catalog_tests
                    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps "${HALO2_PROMPT_VISTA_ASSETS}")
                set_tests_properties(native_hud_action_prompt_catalog PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
                if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/mcc-classic-hud/menu-ui-source.tsv")
                    add_test(NAME native_hud_action_prompts_vista_buttons_mcc COMMAND hud_action_prompt_tests
                        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/mcc-classic-hud)
                    set_tests_properties(native_hud_action_prompts_vista_buttons_mcc PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
                endif()
                if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/gemini.map")
                    add_test(NAME native_hud_action_prompt_catalog_dlc COMMAND hud_action_prompt_catalog_tests
                        ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps "${HALO2_PROMPT_VISTA_ASSETS}")
                    set_tests_properties(native_hud_action_prompt_catalog_dlc PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
                endif()
            endif()
        endif()
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
        add_test(NAME retail_hud_gameplay COMMAND hud_gameplay_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    endif()
    add_executable(motion_sensor_tests tests/motion_sensor_tests.cpp)
    add_executable(hud_sensor_tests tests/hud_sensor_tests.cpp)
    target_link_libraries(hud_sensor_tests PRIVATE halo2_systems)
    add_executable(hud_handoff_tests tests/hud_handoff_tests.cpp)
    target_link_libraries(hud_handoff_tests PRIVATE halo2_systems)
    add_test(NAME native_hud_handoff COMMAND hud_handoff_tests "${HALO2_TU15_GAMEPLAY_XBE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
    set_tests_properties(native_hud_handoff PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    if(DEFINED HALO2_PROMPT_VISTA_ASSETS AND EXISTS "${HALO2_PROMPT_VISTA_ASSETS}/hud.tsv")
        add_test(NAME native_hud_handoff_vista COMMAND hud_handoff_tests "${HALO2_TU15_GAMEPLAY_XBE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps" "${HALO2_PROMPT_VISTA_ASSETS}")
        set_tests_properties(native_hud_handoff_vista PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
        add_test(NAME native_hud_motion_sensor COMMAND hud_sensor_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        add_test(NAME native_hud_motion_sensor_viewports COMMAND hud_sensor_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps --viewports
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_radar_viewports.tsv)
    endif()
    target_link_libraries(motion_sensor_tests PRIVATE halo2_cache)
    add_test(NAME original_motion_sensor COMMAND motion_sensor_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/hud-functionality/motion-sensor.tsv)
    set_tests_properties(original_motion_sensor PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(hud_tests PRIVATE halo2_systems)
    add_test(NAME native_hud_layout COMMAND hud_tests)
    add_executable(hud_score_tests tests/hud_score_tests.cpp)
    add_executable(hud_objective_tests tests/hud_objective_tests.cpp)
    target_link_libraries(hud_objective_tests PRIVATE halo2_systems)
    target_compile_definitions(hud_objective_tests PRIVATE
        HALO2_OBJECTIVE_QUERY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hud_objective_query_original.tsv"
        HALO2_OBJECTIVE_GEOMETRY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hud_objective_geometry_original.tsv")
    add_test(NAME original_hud_objectives COMMAND hud_objective_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(original_hud_objectives PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(hud_score_tests PRIVATE halo2_systems)
    add_test(NAME original_hud_scores COMMAND hud_score_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hud_score_original.tsv)
    add_executable(hud_score_icon_tests tests/hud_score_icon_tests.cpp)
    target_link_libraries(hud_score_icon_tests PRIVATE halo2_systems)
    target_compile_definitions(hud_score_icon_tests PRIVATE
        HALO2_SCORE_ICON_GEOMETRY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hud_score_icons_original.tsv")
    add_test(NAME retail_hud_score_icons COMMAND hud_score_icon_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_hud_score_icons PROPERTIES SKIP_RETURN_CODE 77)
    if(HALO2_LAUNCHER_XVFB)
        add_executable(hud_score_icon_gpu_tests tests/hud_score_icon_tests.cpp)
        target_link_libraries(hud_score_icon_gpu_tests PRIVATE halo2_renderer)
        target_compile_definitions(hud_score_icon_gpu_tests PRIVATE HALO2_SCORE_ICONS_GPU=1
            HALO2_SCORE_ICON_GEOMETRY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hud_score_icons_original.tsv")
        add_dependencies(hud_score_icon_gpu_tests halo2_shaders)
        add_test(NAME retail_hud_score_icons_gpu COMMAND ${HALO2_LAUNCHER_XVFB} -a
            $<TARGET_FILE:hud_score_icon_gpu_tests> ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
            "" "@HALO2_TEST_SCRATCH@/score-icons")
        set_tests_properties(retail_hud_score_icons_gpu PROPERTIES SKIP_RETURN_CODE 77
            ENVIRONMENT "SDL_VIDEODRIVER=x11;SDL_AUDIODRIVER=dummy" TIMEOUT 120
            FAIL_REGULAR_EXPRESSION "VUID-|Validation Error")
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib/libVkLayer_khronos_validation.so")
            set_property(TEST retail_hud_score_icons_gpu APPEND PROPERTY ENVIRONMENT
                "VK_LAYER_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d"
                "LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib:$ENV{LD_LIBRARY_PATH}")
        endif()
    endif()
    set(HALO2_SCORE_ICON_UI_ASSETS "${HALO2_PAUSE_TEST_UI_ASSETS}" CACHE PATH
        "Optional selected HUD/scoreboard artwork for score icon validation")
    if(EXISTS "${HALO2_SCORE_ICON_UI_ASSETS}/hud.tsv" AND
            EXISTS "${HALO2_SCORE_ICON_UI_ASSETS}/scoreboard.tsv")
        add_test(NAME retail_hud_score_icons_selected_ui COMMAND hud_score_icon_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps "${HALO2_SCORE_ICON_UI_ASSETS}")
        set_tests_properties(retail_hud_score_icons_selected_ui PROPERTIES SKIP_RETURN_CODE 77)
        if(HALO2_LAUNCHER_XVFB)
            add_test(NAME retail_hud_score_icons_selected_ui_gpu COMMAND ${HALO2_LAUNCHER_XVFB} -a
                $<TARGET_FILE:hud_score_icon_gpu_tests> ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
                "${HALO2_SCORE_ICON_UI_ASSETS}" "@HALO2_TEST_SCRATCH@/score-icons-selected-ui")
            get_test_property(retail_hud_score_icons_gpu ENVIRONMENT _halo2_score_icon_environment)
            set_tests_properties(retail_hud_score_icons_selected_ui_gpu PROPERTIES
                SKIP_RETURN_CODE 77 TIMEOUT 120 ENVIRONMENT "${_halo2_score_icon_environment}"
                FAIL_REGULAR_EXPRESSION "VUID-|Validation Error")
        endif()
    endif()
    set(HALO2_SCORE_ICON_MCC_MAPS "" CACHE PATH "Optional prepared MCC maps for score icon validation")
    if(EXISTS "${HALO2_SCORE_ICON_MCC_MAPS}/lockout.map")
        add_test(NAME native_mcc_hud_score_icons COMMAND hud_score_icon_tests "${HALO2_SCORE_ICON_MCC_MAPS}")
        set_tests_properties(native_mcc_hud_score_icons PROPERTIES SKIP_RETURN_CODE 77)
        if(HALO2_LAUNCHER_XVFB)
            add_test(NAME native_mcc_hud_score_icons_gpu COMMAND ${HALO2_LAUNCHER_XVFB} -a
                $<TARGET_FILE:hud_score_icon_gpu_tests> "${HALO2_SCORE_ICON_MCC_MAPS}"
                "" "@HALO2_TEST_SCRATCH@/score-icons-mcc")
            get_test_property(retail_hud_score_icons_gpu ENVIRONMENT _halo2_score_icon_environment)
            set_tests_properties(native_mcc_hud_score_icons_gpu PROPERTIES
                SKIP_RETURN_CODE 77 TIMEOUT 120 ENVIRONMENT "${_halo2_score_icon_environment}"
                FAIL_REGULAR_EXPRESSION "VUID-|Validation Error")
        endif()
    endif()
    if(HALO2_LAUNCHER_XVFB)
        add_executable(hud_objective_gpu_tests tests/hud_objective_tests.cpp)
        target_link_libraries(hud_objective_gpu_tests PRIVATE halo2_renderer)
        target_compile_definitions(hud_objective_gpu_tests PRIVATE HALO2_OBJECTIVE_GPU=1
            HALO2_OBJECTIVE_QUERY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hud_objective_query_original.tsv"
            HALO2_OBJECTIVE_GEOMETRY_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hud_objective_geometry_original.tsv")
        add_dependencies(hud_objective_gpu_tests halo2_shaders)
        add_test(NAME original_hud_objectives_gpu COMMAND ${HALO2_LAUNCHER_XVFB} -a
            $<TARGET_FILE:hud_objective_gpu_tests> ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
            "" "@HALO2_TEST_SCRATCH@/objectives")
        get_test_property(retail_hud_score_icons_gpu ENVIRONMENT _halo2_objective_environment)
        set_tests_properties(original_hud_objectives_gpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120
            ENVIRONMENT "${_halo2_objective_environment}" FAIL_REGULAR_EXPRESSION "VUID-|Validation Error")
    endif()
    if(EXISTS "${HALO2_SCORE_ICON_UI_ASSETS}/hud.tsv")
        add_test(NAME selected_ui_hud_objectives COMMAND hud_objective_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps "${HALO2_SCORE_ICON_UI_ASSETS}")
        set_tests_properties(selected_ui_hud_objectives PROPERTIES SKIP_RETURN_CODE 77)
        if(HALO2_LAUNCHER_XVFB)
            add_test(NAME selected_ui_hud_objectives_gpu COMMAND ${HALO2_LAUNCHER_XVFB} -a
                $<TARGET_FILE:hud_objective_gpu_tests> ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
                "${HALO2_SCORE_ICON_UI_ASSETS}" "@HALO2_TEST_SCRATCH@/objectives-selected-ui")
            set_tests_properties(selected_ui_hud_objectives_gpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120
                ENVIRONMENT "${_halo2_objective_environment}" FAIL_REGULAR_EXPRESSION "VUID-|Validation Error")
        endif()
    endif()
    if(EXISTS "${HALO2_SCORE_ICON_MCC_MAPS}/lockout.map")
        add_test(NAME native_mcc_hud_objectives COMMAND hud_objective_tests "${HALO2_SCORE_ICON_MCC_MAPS}")
        set_tests_properties(native_mcc_hud_objectives PROPERTIES SKIP_RETURN_CODE 77)
        if(HALO2_LAUNCHER_XVFB)
            add_test(NAME native_mcc_hud_objectives_gpu COMMAND ${HALO2_LAUNCHER_XVFB} -a
                $<TARGET_FILE:hud_objective_gpu_tests> "${HALO2_SCORE_ICON_MCC_MAPS}"
                "" "@HALO2_TEST_SCRATCH@/objectives-mcc")
            set_tests_properties(native_mcc_hud_objectives_gpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120
                ENVIRONMENT "${_halo2_objective_environment}" FAIL_REGULAR_EXPRESSION "VUID-|Validation Error")
        endif()
    endif()
    add_executable(hud_aim_tests tests/hud_aim_tests.cpp)
    target_link_libraries(hud_aim_tests PRIVATE halo2_cache)
    add_test(NAME retail_hud_aim COMMAND hud_aim_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_hud_aim.tsv)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
        add_test(NAME retail_multiplayer_hud_scores COMMAND hud_score_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hud_score_original.tsv
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        add_test(NAME retail_multiplayer_hud COMMAND hud_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_hud_positions.tsv
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_hud_text_positions.tsv)
        add_executable(hud_render_tests tests/hud_render_tests.cpp)
        target_link_libraries(hud_render_tests PRIVATE halo2_renderer Vulkan::Vulkan)
        target_compile_definitions(hud_render_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
        add_dependencies(hud_render_tests halo2_shaders)
        add_test(NAME retail_hud_framebuffer COMMAND hud_render_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_BINARY_DIR}/hud-framebuffer)
        set_tests_properties(retail_hud_framebuffer PROPERTIES SKIP_RETURN_CODE 77)
        if(HALO2_LAUNCHER_XVFB)
            add_test(NAME original_shield_radar_alignment_gpu COMMAND ${HALO2_LAUNCHER_XVFB} -a
                $<TARGET_FILE:hud_render_tests> ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
                "@HALO2_TEST_SCRATCH@/shield-radar")
            set_tests_properties(original_shield_radar_alignment_gpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180
                ENVIRONMENT "${_halo2_objective_environment}" FAIL_REGULAR_EXPRESSION "VUID-|Validation Error")
            if(EXISTS "${HALO2_SCORE_ICON_UI_ASSETS}/hud.tsv")
                add_test(NAME selected_ui_shield_radar_alignment_gpu COMMAND ${HALO2_LAUNCHER_XVFB} -a
                    $<TARGET_FILE:hud_render_tests> ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
                    "@HALO2_TEST_SCRATCH@/shield-radar-selected-ui" "${HALO2_SCORE_ICON_UI_ASSETS}")
                set_tests_properties(selected_ui_shield_radar_alignment_gpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180
                    ENVIRONMENT "${_halo2_objective_environment}" FAIL_REGULAR_EXPRESSION "VUID-|Validation Error")
            endif()
            if(EXISTS "${HALO2_SCORE_ICON_MCC_MAPS}/lockout.map")
                add_test(NAME native_mcc_shield_radar_alignment_gpu COMMAND ${HALO2_LAUNCHER_XVFB} -a
                    $<TARGET_FILE:hud_render_tests> "${HALO2_SCORE_ICON_MCC_MAPS}"
                    "@HALO2_TEST_SCRATCH@/shield-radar-mcc")
                set_tests_properties(native_mcc_shield_radar_alignment_gpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180
                    ENVIRONMENT "${_halo2_objective_environment}" FAIL_REGULAR_EXPRESSION "VUID-|Validation Error")
            endif()
        endif()
    endif()
    add_executable(multiplayer_tests tests/multiplayer_tests.cpp)
    target_link_libraries(multiplayer_tests PRIVATE halo2_systems)
    add_executable(king_runtime_tests tests/king_runtime_tests.cpp)
    target_link_libraries(king_runtime_tests PRIVATE halo2_systems)
    add_executable(king_objective_network_tests tests/king_objective_network_tests.cpp)
    target_link_libraries(king_objective_network_tests PRIVATE halo2_systems)
    add_executable(king_recording_runtime_tests tests/king_recording_runtime_tests.cpp)
    target_link_libraries(king_recording_runtime_tests PRIVATE halo2_systems)
    add_test(NAME native_king_recording_runtime COMMAND king_recording_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        "$ENV{HOME}/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    add_test(NAME native_king_objective_network COMMAND king_objective_network_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
    set_tests_properties(native_king_objective_network PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(player_damage_tests tests/player_damage_tests.cpp
        src/gameplay/units/source_player_damage.cpp src/gameplay/units/damage_amount.cpp src/gameplay/multiplayer/source_king.cpp)
    target_include_directories(player_damage_tests PRIVATE include)
    target_link_libraries(player_damage_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME original_player_damage COMMAND player_damage_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/king_trait_consumers_original.json)
    add_executable(camouflage_tests tests/camouflage_tests.cpp
        src/rendering/source_camouflage.cpp src/gameplay/multiplayer/source_king.cpp)
    target_include_directories(camouflage_tests PRIVATE include)
    target_link_libraries(camouflage_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME original_camouflage COMMAND camouflage_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/camouflage_state_original.json
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/camouflage_disruption_original.json)
    add_executable(camouflage_runtime_tests tests/camouflage_runtime_tests.cpp)
    add_executable(camouflage_context_tests tests/camouflage_context_tests.cpp src/rendering/source_camouflage_context.cpp)
    target_include_directories(camouflage_context_tests PRIVATE include)
    target_link_libraries(camouflage_context_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME original_camouflage_context COMMAND camouflage_context_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/camouflage_render_context_original.json)
    add_executable(camouflage_material_tests tests/camouflage_material_tests.cpp)
    target_link_libraries(camouflage_material_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_camouflage_material_animation COMMAND camouflage_material_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/camouflage_material_animation_original.json)
    add_executable(shader_texture_reference_tests tests/shader_texture_reference_tests.cpp)
    target_link_libraries(shader_texture_reference_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_shader_texture_reference COMMAND shader_texture_reference_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/game-variants/camouflage-pass-20261011/report.json
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/game-variants/camouflage-overlay-20261011/assets.json)
    add_executable(source_vertex_extern_tests tests/source_vertex_extern_tests.cpp)
    target_link_libraries(source_vertex_extern_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_vertex_extern COMMAND source_vertex_extern_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/camouflage_vertex_extern_original.json
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/game-variants/camouflage-pass-20261011/report.json)
    add_executable(source_shader_rgb_tests tests/source_shader_rgb_tests.cpp)
    target_link_libraries(source_shader_rgb_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_shader_rgb COMMAND source_shader_rgb_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/state_rgb_pack_original.json
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/game-variants/camouflage-binder-20261011/binder-report.json)
    add_executable(shader_state_import_tests tests/shader_state_import_tests.cpp)
    target_link_libraries(shader_state_import_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_shader_state_import COMMAND shader_state_import_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/game-variants/camouflage-overlay-20261011/assets.json)
    add_executable(camouflage_state_material_tests tests/camouflage_state_material_tests.cpp)
    target_link_libraries(camouflage_state_material_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_camouflage_state_material COMMAND camouflage_state_material_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/camouflage_state_banks_original.json
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/camouflage_state_inputs_original.json)
    add_executable(source_shader_state_tests tests/source_shader_state_tests.cpp)
    target_link_libraries(source_shader_state_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_shader_state COMMAND source_shader_state_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/render_state_extern_original.json)
    add_executable(camouflage_pass_bank_tests tests/camouflage_pass_bank_tests.cpp src/rendering/source_camouflage_context.cpp)
    target_link_libraries(camouflage_pass_bank_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_camouflage_pass_banks COMMAND camouflage_pass_bank_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/camouflage_pass_banks_original.json)
    add_executable(source_shader_fog_tests tests/source_shader_fog_tests.cpp)
    target_link_libraries(source_shader_fog_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_shader_fog COMMAND source_shader_fog_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/fog_externs_original.json)
    add_executable(source_scene_copy_tests tests/source_scene_copy_tests.cpp)
    add_executable(hologram_material_tests tests/hologram_material_tests.cpp)
    add_executable(source_transparent_group_parts_tests tests/source_transparent_group_parts_tests.cpp)
    target_link_libraries(source_transparent_group_parts_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_transparent_group_parts COMMAND source_transparent_group_parts_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/opening_regret_group_v719.json)
    set_tests_properties(original_transparent_group_parts PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(hologram_material_tests PRIVATE halo2_cache)
    add_test(NAME original_hologram_material COMMAND hologram_material_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(original_hologram_material PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(source_scene_copy_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_scene_copy_geometry COMMAND source_scene_copy_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/game-variants/camouflage-scene-copy-20261011/report.json
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/scene_copy_geometry_original.json)
    add_executable(scene_copy_destination_tests tests/scene_copy_destination_tests.cpp)
    target_link_libraries(scene_copy_destination_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_scene_copy_destinations COMMAND scene_copy_destination_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/scene_copy_destinations_original.json)
    add_executable(water_copy_gpu_tests tests/water_copy_gpu_tests.cpp)
    add_executable(scene_copy_gpu_tests tests/scene_copy_gpu_tests.cpp src/rendering/source_scene_copy.cpp)
    target_include_directories(scene_copy_gpu_tests PRIVATE include)
    target_link_libraries(scene_copy_gpu_tests PRIVATE Vulkan::Vulkan nlohmann_json::nlohmann_json)
    add_dependencies(scene_copy_gpu_tests halo2_shaders)
    add_test(NAME original_scene_copy_host_gpu COMMAND scene_copy_gpu_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/scene_copy_destinations_original.json
        ${CMAKE_CURRENT_BINARY_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/scene-copy-host/report.json)
    add_executable(source_scene_copy_gpu_tests tests/source_scene_copy_gpu_tests.cpp
        src/rendering/source_scene_copy_gpu.cpp src/rendering/source_scene_copy.cpp)
    target_include_directories(source_scene_copy_gpu_tests PRIVATE include)
    target_compile_definitions(source_scene_copy_gpu_tests PRIVATE
        HALO2_SHADOW_PROBE_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
    target_link_libraries(source_scene_copy_gpu_tests PRIVATE Vulkan::Vulkan nlohmann_json::nlohmann_json Threads::Threads)
    add_dependencies(source_scene_copy_gpu_tests halo2_shaders)
    add_test(NAME native_scene_copy_resources_gpu COMMAND source_scene_copy_gpu_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/scene_copy_destinations_original.json
        ${CMAKE_CURRENT_BINARY_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/scene-copy-native/report.json)
    set_tests_properties(native_scene_copy_resources_gpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    target_link_libraries(water_copy_gpu_tests PRIVATE halo2_cache Vulkan::Vulkan nlohmann_json::nlohmann_json)
    add_dependencies(water_copy_gpu_tests halo2_shaders)
    add_test(NAME original_water_copy_host_gpu COMMAND water_copy_gpu_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/scene_copy_destinations_original.json
        ${CMAKE_CURRENT_BINARY_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/water-copy-host-framebuffer)
    target_link_libraries(camouflage_runtime_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_camouflage_runtime COMMAND camouflage_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/camouflage_timeline_original.json
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    add_test(NAME native_king_runtime COMMAND king_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    add_test(NAME native_multiplayer COMMAND multiplayer_tests)
    add_test(NAME native_multiplayer_asset_identity COMMAND multiplayer_tests --asset-identity)
    add_test(NAME native_vehicle_intent COMMAND multiplayer_tests --vehicle-intent)
    add_test(NAME native_request_ingress COMMAND multiplayer_tests --request-ingress)
    add_test(NAME native_lan_admission_security COMMAND multiplayer_tests --admission-security)
    add_test(NAME native_session_configuration COMMAND multiplayer_tests --session-config)
    add_test(NAME native_session_configuration_loading COMMAND multiplayer_tests --session-config-loading)
    add_test(NAME native_lan_startup COMMAND multiplayer_tests --startup-delay)
    add_test(NAME native_lan_snapshot_stream COMMAND multiplayer_tests --snapshot-stream)
    add_executable(multiplayer_species_tests tests/multiplayer_species_tests.cpp)
    target_link_libraries(multiplayer_species_tests PRIVATE halo2_systems)
    add_test(NAME native_multiplayer_species COMMAND multiplayer_species_tests ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(native_multiplayer_species PROPERTIES SKIP_RETURN_CODE 77)
    foreach(species_mode single mixed lan)
        add_test(NAME native_multiplayer_species_${species_mode} COMMAND multiplayer_species_runtime_tests
            ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/species-runtime ${species_mode})
        set_tests_properties(native_multiplayer_species_${species_mode} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    endforeach()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
        add_test(NAME retail_multiplayer COMMAND multiplayer_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    endif()
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/texture_program.comp.spv
        COMMAND ${GLSLANG_VALIDATOR} -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/texture_program.comp
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/texture_program.comp.spv
        DEPENDS tests/texture_program.comp shaders/texture_program.glsl shaders/render_view.glsl VERBATIM)
    add_executable(texture_program_tests tests/texture_program_tests.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/texture_program.comp.spv)
    target_include_directories(texture_program_tests PRIVATE include)
    target_link_libraries(texture_program_tests PRIVATE Vulkan::Vulkan halo2_cache)
    target_compile_definitions(texture_program_tests PRIVATE
        HALO2_TEXTURE_TEST_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/texture_program.comp.spv")
    add_test(NAME native_texture_addressing COMMAND texture_program_tests)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
        add_executable(special_texture_gpu_tests tests/special_texture_gpu_tests.cpp)
        target_link_libraries(special_texture_gpu_tests PRIVATE halo2_renderer Vulkan::Vulkan)
        add_dependencies(special_texture_gpu_tests halo2_shaders)
        add_test(NAME retail_special_texture_gpu COMMAND special_texture_gpu_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_BINARY_DIR}/special-texture-framebuffer)
        set_tests_properties(retail_special_texture_gpu PROPERTIES SKIP_RETURN_CODE 77)
    endif()
    add_executable(render_view_tests tests/render_view_tests.cpp)
    target_link_libraries(render_view_tests PRIVATE halo2_cache)
    add_test(NAME render_view_resolution COMMAND render_view_tests)
    add_executable(source_view_rectangle_tests tests/source_view_rectangle_tests.cpp)
    target_link_libraries(source_view_rectangle_tests PRIVATE halo2_systems)
    add_test(NAME retail_source_view_rectangle COMMAND source_view_rectangle_tests)
    set_tests_properties(retail_source_view_rectangle PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
        add_executable(source_view_rectangle_gpu_tests tests/source_view_rectangle_gpu_tests.cpp)
        target_link_libraries(source_view_rectangle_gpu_tests PRIVATE halo2_renderer Vulkan::Vulkan)
        add_dependencies(source_view_rectangle_gpu_tests halo2_shaders)
        add_test(NAME retail_source_view_rectangle_gpu COMMAND source_view_rectangle_gpu_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_BINARY_DIR}/source-view-rectangle-framebuffer)
        set_tests_properties(retail_source_view_rectangle_gpu PROPERTIES SKIP_RETURN_CODE 77)
    endif()
    add_executable(vertex_admission_tests tests/vertex_admission_tests.cpp)
    target_link_libraries(vertex_admission_tests PRIVATE halo2_cache)
    add_test(NAME vertex_program_admission COMMAND vertex_admission_tests)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/prt_vertex.comp.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${GLSLANG_VALIDATOR} -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders/prt_vertex.comp
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/prt_vertex.comp.spv
        DEPENDS shaders/prt_vertex.comp shaders/vertex_program.glsl shaders/render_view.glsl VERBATIM)
    add_custom_target(prt_vertex_test_shader DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/prt_vertex.comp.spv)
    add_executable(prt_vertex_tests tests/prt_vertex_tests.cpp)
    target_link_libraries(prt_vertex_tests PRIVATE halo2_cache Vulkan::Vulkan)
    target_compile_definitions(prt_vertex_tests PRIVATE
        HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        HALO2_PRT_VERTEX_TEST_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/prt_vertex.comp.spv")
    add_dependencies(prt_vertex_tests prt_vertex_test_shader)
    add_test(NAME original_prt_vertex_cpu COMMAND prt_vertex_tests)
    add_test(NAME original_prt_vertex_gpu COMMAND prt_vertex_tests --gpu)
    set_tests_properties(original_prt_vertex_cpu original_prt_vertex_gpu PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_shadow_vertex_tests tests/source_shadow_vertex_tests.cpp)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/source_vertex_declaration.comp.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${GLSLANG_VALIDATOR} -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders/source_vertex_declaration.comp
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/source_vertex_declaration.comp.spv
        DEPENDS shaders/source_vertex_declaration.comp shaders/source_vertex_inputs.glsl
            shaders/vertex_program.glsl shaders/render_view.glsl VERBATIM)
    add_custom_target(source_vertex_declaration_test_shader DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/source_vertex_declaration.comp.spv)
    add_executable(source_vertex_declaration_tests tests/source_vertex_declaration_tests.cpp)
    target_link_libraries(source_vertex_declaration_tests PRIVATE halo2_cache Vulkan::Vulkan)
    target_compile_definitions(source_vertex_declaration_tests PRIVATE
        HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        HALO2_VERTEX_DECLARATION_TEST_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/source_vertex_declaration.comp.spv")
    add_dependencies(source_vertex_declaration_tests source_vertex_declaration_test_shader)
    if(MSVC)
        target_compile_options(source_vertex_declaration_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_vertex_declaration_tests PRIVATE -ffp-contract=off)
    endif()
    add_test(NAME source_world_vertex_declaration_cpu COMMAND source_vertex_declaration_tests)
    add_test(NAME source_world_vertex_declaration_gpu COMMAND source_vertex_declaration_tests --gpu)
    add_executable(king_boundary_material_tests tests/king_boundary_material_tests.cpp)
    target_link_libraries(king_boundary_material_tests PRIVATE halo2_cache Vulkan::Vulkan nlohmann_json::nlohmann_json)
    target_compile_definitions(king_boundary_material_tests PRIVATE
        HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        HALO2_VERTEX_DECLARATION_TEST_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/source_vertex_declaration.comp.spv")
    add_dependencies(king_boundary_material_tests source_vertex_declaration_test_shader)
    if(MSVC)
        target_compile_options(king_boundary_material_tests PRIVATE /fp:strict)
    else()
        target_compile_options(king_boundary_material_tests PRIVATE -ffp-contract=off)
    endif()
    add_test(NAME native_king_boundary_material_cpu COMMAND king_boundary_material_tests)
    add_test(NAME native_king_boundary_material_gpu COMMAND king_boundary_material_tests --gpu)
    add_executable(king_boundary_renderer_tests tests/king_boundary_renderer_tests.cpp)
    target_link_libraries(king_boundary_renderer_tests PRIVATE halo2_renderer halo2_systems nlohmann_json::nlohmann_json)
    target_compile_definitions(king_boundary_renderer_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME native_king_boundary_renderer COMMAND king_boundary_renderer_tests
        ${CMAKE_CURRENT_BINARY_DIR}/king-boundary-renderer-results)
    set_tests_properties(native_king_boundary_renderer PROPERTIES RUN_SERIAL TRUE)
    add_executable(source_cpu_skin_inputs_tests tests/source_cpu_skin_inputs_tests.cpp)
    add_executable(source_shield_vertex_tests tests/source_shield_vertex_tests.cpp)
    target_link_libraries(source_shield_vertex_tests PRIVATE halo2_cache Vulkan::Vulkan)
    target_compile_definitions(source_shield_vertex_tests PRIVATE
        HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        HALO2_SHIELD_VERTEX_TEST_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/source_shield_vertex.comp.spv")
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/source_shield_vertex.comp.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${GLSLANG_VALIDATOR} -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders/source_shield_vertex.comp
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/source_shield_vertex.comp.spv
        DEPENDS shaders/source_shield_vertex.comp shaders/source_cpu_skin_outputs.glsl
            shaders/source_vertex_inputs.glsl shaders/vertex_program.glsl shaders/render_view.glsl VERBATIM)
    add_custom_target(source_shield_vertex_test_shader DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/source_shield_vertex.comp.spv)
    add_dependencies(source_shield_vertex_tests source_shield_vertex_test_shader)
    if(MSVC)
        target_compile_options(source_shield_vertex_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_shield_vertex_tests PRIVATE -fno-fast-math -ffp-contract=off)
    endif()
    add_test(NAME source_shield_vertex_cpu COMMAND source_shield_vertex_tests)
    add_test(NAME source_shield_vertex_gpu COMMAND source_shield_vertex_tests --gpu)
    set_tests_properties(source_shield_vertex_cpu source_shield_vertex_gpu PROPERTIES SKIP_RETURN_CODE 77)
    set_tests_properties(source_shield_vertex_gpu PROPERTIES ENVIRONMENT
        "VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation;VK_LAYER_VALIDATE_SYNC=1;VK_LAYER_ENABLES=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT")
    target_link_libraries(source_cpu_skin_inputs_tests PRIVATE halo2_cache Vulkan::Vulkan)
    target_compile_definitions(source_cpu_skin_inputs_tests PRIVATE
        HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        HALO2_VERTEX_DECLARATION_TEST_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/source_vertex_declaration.comp.spv")
    add_dependencies(source_cpu_skin_inputs_tests source_vertex_declaration_test_shader)
    if(MSVC)
        target_compile_options(source_cpu_skin_inputs_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_cpu_skin_inputs_tests PRIVATE -ffp-contract=off)
    endif()
    add_test(NAME source_cpu_skin_inputs_cpu COMMAND source_cpu_skin_inputs_tests)
    add_test(NAME source_cpu_skin_inputs_gpu COMMAND source_cpu_skin_inputs_tests --gpu)
    add_test(NAME campaign_character_vertex_cpu COMMAND source_cpu_skin_inputs_tests --campaign)
    add_test(NAME campaign_character_vertex_gpu COMMAND source_cpu_skin_inputs_tests --campaign --gpu)
    set_tests_properties(source_world_vertex_declaration_gpu source_cpu_skin_inputs_gpu campaign_character_vertex_gpu PROPERTIES ENVIRONMENT
        "VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation;VK_LAYER_VALIDATE_SYNC=1;VK_LAYER_ENABLES=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT")
    add_executable(source_sky_order_gpu_tests tests/source_sky_order_gpu_tests.cpp)
    target_link_libraries(source_sky_order_gpu_tests PRIVATE halo2_renderer)
    target_compile_definitions(source_sky_order_gpu_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME source_sky_order_gpu COMMAND source_sky_order_gpu_tests
        ${CMAKE_CURRENT_BINARY_DIR}/source-sky-order)
    add_executable(sky_all_maps_gpu_tests tests/sky_all_maps_gpu_tests.cpp)
    target_link_libraries(sky_all_maps_gpu_tests PRIVATE halo2_renderer nlohmann_json::nlohmann_json)
    target_compile_definitions(sky_all_maps_gpu_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME sky_all_maps_gpu COMMAND sky_all_maps_gpu_tests
        ${CMAKE_CURRENT_BINARY_DIR}/sky-all-maps)
    set_tests_properties(sky_all_maps_gpu PROPERTIES TIMEOUT 7200 RUN_SERIAL TRUE ENVIRONMENT
        "VK_LAYER_VALIDATE_SYNC=1")
    add_executable(source_cpu_skin_render_tests tests/source_cpu_skin_render_tests.cpp)
    target_link_libraries(source_cpu_skin_render_tests PRIVATE halo2_renderer)
    target_compile_definitions(source_cpu_skin_render_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME source_cpu_skin_renderer_gpu COMMAND source_cpu_skin_render_tests
        ${CMAKE_CURRENT_BINARY_DIR}/cpu-skin-input-inspection)
    add_executable(source_shield_render_tests tests/source_shield_render_tests.cpp)
    target_link_libraries(source_shield_render_tests PRIVATE halo2_renderer)
    target_compile_definitions(source_shield_render_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME source_shield_renderer_gpu COMMAND source_shield_render_tests
        ${CMAKE_CURRENT_BINARY_DIR}/shield-input-inspection)
    set_tests_properties(source_shield_renderer_gpu PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT
        "VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation;VK_LAYER_VALIDATE_SYNC=1;VK_LAYER_ENABLES=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT")
    set_tests_properties(source_world_vertex_declaration_cpu source_world_vertex_declaration_gpu PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(source_shadow_vertex_tests PRIVATE halo2_cache Vulkan::Vulkan)
    target_compile_definitions(source_shadow_vertex_tests PRIVATE
        HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        HALO2_SHADOW_VERTEX_TEST_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/prt_vertex.comp.spv")
    add_dependencies(source_shadow_vertex_tests prt_vertex_test_shader)
    if(MSVC)
        target_compile_options(source_shadow_vertex_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_shadow_vertex_tests PRIVATE -ffp-contract=off)
    endif()
    add_test(NAME original_shadow_vertex_cpu COMMAND source_shadow_vertex_tests)
    add_test(NAME original_shadow_vertex_gpu COMMAND source_shadow_vertex_tests --gpu)
    set_tests_properties(original_shadow_vertex_cpu original_shadow_vertex_gpu PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_shadow_caster_world_tests tests/source_shadow_caster_world_tests.cpp)
    target_link_libraries(source_shadow_caster_world_tests PRIVATE halo2_cache)
    target_compile_definitions(source_shadow_caster_world_tests PRIVATE
        HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    if(MSVC)
        target_compile_options(source_shadow_caster_world_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_shadow_caster_world_tests PRIVATE -ffp-contract=off)
    endif()
    add_test(NAME original_shadow_caster_world_cpu COMMAND source_shadow_caster_world_tests)
    set_tests_properties(original_shadow_caster_world_cpu PROPERTIES SKIP_RETURN_CODE 77)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/source_vertex_bank.comp.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${GLSLANG_VALIDATOR} -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders/source_vertex_bank.comp
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/source_vertex_bank.comp.spv
        DEPENDS shaders/source_vertex_bank.comp shaders/vertex_program.glsl shaders/source_vertex_inputs.glsl shaders/render_view.glsl VERBATIM)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/source_vertex_bank_production.comp.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${GLSLANG_VALIDATOR} -V -DH2_VERTEX_PRODUCTION_INPUTS=1 -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders/source_vertex_bank.comp
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/source_vertex_bank_production.comp.spv
        DEPENDS shaders/source_vertex_bank.comp shaders/vertex_program.glsl shaders/source_vertex_inputs.glsl shaders/render_view.glsl VERBATIM)
    add_custom_target(source_vertex_bank_test_shader DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/source_vertex_bank.comp.spv
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/source_vertex_bank_production.comp.spv)
    add_executable(source_vertex_bank_tests tests/source_vertex_bank_tests.cpp)
    target_link_libraries(source_vertex_bank_tests PRIVATE halo2_cache Vulkan::Vulkan)
    target_compile_definitions(source_vertex_bank_tests PRIVATE
        HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        HALO2_VERTEX_BANK_TEST_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/source_vertex_bank.comp.spv"
        HALO2_VERTEX_BANK_PRODUCTION_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/source_vertex_bank_production.comp.spv")
    add_dependencies(source_vertex_bank_tests source_vertex_bank_test_shader)
    if(MSVC)
        target_compile_options(source_vertex_bank_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_vertex_bank_tests PRIVATE -ffp-contract=off)
    endif()
    add_test(NAME source_vertex_bank_cpu COMMAND source_vertex_bank_tests)
    add_test(NAME source_vertex_bank_gpu COMMAND source_vertex_bank_tests --gpu)
    set_tests_properties(source_vertex_bank_gpu PROPERTIES ENVIRONMENT
        "VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation;VK_LAYER_VALIDATE_SYNC=1;VK_LAYER_ENABLES=VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT")
    add_executable(source_model_fade_tests tests/source_model_fade_tests.cpp)
    target_link_libraries(source_model_fade_tests PRIVATE halo2_cache)
    target_compile_definitions(source_model_fade_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    if(MSVC)
        target_compile_options(source_model_fade_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_model_fade_tests PRIVATE -ffp-contract=off)
    endif()
    add_test(NAME retail_source_model_fade COMMAND source_model_fade_tests)
    add_executable(source_shadow_view_tests tests/source_shadow_view_tests.cpp)
    target_link_libraries(source_shadow_view_tests PRIVATE halo2_cache)
    target_compile_definitions(source_shadow_view_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    if(MSVC)
        target_compile_options(source_shadow_view_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_shadow_view_tests PRIVATE "-fno-fast-math" "-ffp-contract=off")
    endif()
    add_test(NAME retail_source_shadow_view COMMAND source_shadow_view_tests)
    add_test(NAME original_effect_light_cameras COMMAND source_shadow_view_tests --point-cameras
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_cameras.original.bin)
    add_executable(source_shadow_secondary_tests tests/source_shadow_secondary_tests.cpp)
    target_link_libraries(source_shadow_secondary_tests PRIVATE halo2_cache)
    target_compile_definitions(source_shadow_secondary_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME retail_source_shadow_secondary COMMAND source_shadow_secondary_tests)
    add_executable(source_draw_unit_cull_tests tests/source_draw_unit_cull_tests.cpp)
    add_executable(source_bsp_draw_collector_tests tests/source_bsp_draw_collector_tests.cpp)
    add_executable(source_draw_unit_tests tests/source_draw_unit_tests.cpp)
    target_link_libraries(source_draw_unit_tests PRIVATE halo2_cache)
    target_compile_definitions(source_draw_unit_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_shared_draw_unit_allocator COMMAND source_draw_unit_tests)
    add_executable(source_object_draw_collector_tests tests/source_object_draw_collector_tests.cpp)
    target_link_libraries(source_object_draw_collector_tests PRIVATE halo2_cache)
    target_compile_definitions(source_object_draw_collector_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_object_draw_collector COMMAND source_object_draw_collector_tests)
    add_executable(source_object_presentation_tests tests/source_object_presentation_tests.cpp)
    target_link_libraries(source_object_presentation_tests PRIVATE halo2_cache)
    target_compile_definitions(source_object_presentation_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_object_effect_child_presentation COMMAND source_object_presentation_tests)
    add_executable(source_model_resources_tests tests/source_model_resources_tests.cpp)
    target_link_libraries(source_model_resources_tests PRIVATE halo2_cache)
    target_compile_definitions(source_model_resources_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_model_resource_selection COMMAND source_model_resources_tests)
    add_executable(source_model_build_inputs_tests tests/source_model_build_inputs_tests.cpp)
    target_link_libraries(source_model_build_inputs_tests PRIVATE halo2_cache)
    target_compile_definitions(source_model_build_inputs_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_model_build_inputs COMMAND source_model_build_inputs_tests)
    add_executable(source_model_admission_tests tests/source_model_admission_tests.cpp)
    target_link_libraries(source_model_admission_tests PRIVATE halo2_cache)
    target_compile_definitions(source_model_admission_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_model_render_admission COMMAND source_model_admission_tests)
    add_executable(source_model_node_bank_tests tests/source_model_node_bank_tests.cpp)
    target_link_libraries(source_model_node_bank_tests PRIVATE halo2_cache)
    target_compile_definitions(source_model_node_bank_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_model_geometry_node_bank COMMAND source_model_node_bank_tests)
    add_executable(source_model_build_tests tests/source_model_build_tests.cpp)
    target_link_libraries(source_model_build_tests PRIVATE halo2_cache)
    target_compile_definitions(source_model_build_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_model_render_build COMMAND source_model_build_tests)
    add_executable(source_model_lighting_tests tests/source_model_lighting_tests.cpp)
    target_link_libraries(source_model_lighting_tests PRIVATE halo2_cache)
    target_compile_definitions(source_model_lighting_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_model_lighting_cache COMMAND source_model_lighting_tests)
    add_test(NAME original_model_lighting_interpolation_gate COMMAND source_model_lighting_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_model_lighting.original.bin --typed)
    add_executable(source_sphere_rectangle_tests tests/source_sphere_rectangle_tests.cpp)
    target_link_libraries(source_sphere_rectangle_tests PRIVATE halo2_cache)
    target_compile_definitions(source_sphere_rectangle_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_sphere_screen_rectangle COMMAND source_sphere_rectangle_tests)
    add_executable(source_bsp_part_commands_tests tests/source_bsp_part_commands_tests.cpp)
    add_executable(source_bsp_breakable_visibility_tu15_tests tests/source_bsp_breakable_visibility_tu15_tests.cpp)
    target_link_libraries(source_bsp_breakable_visibility_tu15_tests PRIVATE halo2_cache)
    add_test(NAME source_bsp_breakable_visibility_tu15 COMMAND source_bsp_breakable_visibility_tu15_tests)
    add_executable(source_opaque_sort_tests tests/source_opaque_sort_tests.cpp)
    add_executable(source_effect_light_opaque_tests tests/source_effect_light_opaque_tests.cpp)
    target_link_libraries(source_effect_light_opaque_tests PRIVATE halo2_cache)
    target_compile_definitions(source_effect_light_opaque_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_effect_light_opaque_selection COMMAND source_effect_light_opaque_tests)
    add_executable(source_effect_light_ranges_tests tests/source_effect_light_ranges_tests.cpp)
    target_link_libraries(source_effect_light_ranges_tests PRIVATE halo2_cache)
    target_compile_definitions(source_effect_light_ranges_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_effect_light_draw_ranges COMMAND source_effect_light_ranges_tests)
    add_executable(source_effect_light_raw_tests tests/source_effect_light_raw_tests.cpp)
    target_link_libraries(source_effect_light_raw_tests PRIVATE halo2_systems)
    add_test(NAME original_effect_light_raw_bindings COMMAND source_effect_light_raw_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer-shading/world-effects-goal-20261011/light-raw-bindings-oracle-v2)
    add_executable(effect_light_scissor_tests tests/effect_light_scissor_tests.cpp)
    target_link_libraries(effect_light_scissor_tests PRIVATE halo2_cache)
    target_compile_definitions(effect_light_scissor_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_effect_light_scissor COMMAND effect_light_scissor_tests)
    target_link_libraries(source_opaque_sort_tests PRIVATE halo2_cache)
    target_compile_definitions(source_opaque_sort_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_effect_light_opaque_sort COMMAND source_opaque_sort_tests)
    target_link_libraries(source_bsp_part_commands_tests PRIVATE halo2_cache)
    target_compile_definitions(source_bsp_part_commands_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_effect_light_part_commands COMMAND source_bsp_part_commands_tests)
    add_test(NAME original_effect_light_command_frame COMMAND source_bsp_part_commands_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_command_frame.original.bin)
    add_test(NAME original_effect_light_geometry_frame COMMAND source_bsp_draw_collector_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_geometry_frame.original.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
    add_executable(source_bsp_frame_tests tests/source_bsp_frame_tests.cpp)
    target_link_libraries(source_bsp_frame_tests PRIVATE halo2_cache)
    target_compile_definitions(source_bsp_frame_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_bsp_frame_resets COMMAND source_bsp_frame_tests)
    add_executable(source_bsp_accelerator_tests tests/source_bsp_accelerator_tests.cpp)
    target_link_libraries(source_bsp_accelerator_tests PRIVATE halo2_cache)
    add_test(NAME retail_source_bsp_accelerator COMMAND source_bsp_accelerator_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_bsp_accelerator_original.tsv)
    add_test(NAME original_effect_light_accelerator COMMAND source_bsp_accelerator_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_accelerator.original.tsv --sphere)
    add_executable(source_bsp_ranges_tests tests/source_bsp_ranges_tests.cpp)
    target_link_libraries(source_bsp_ranges_tests PRIVATE halo2_cache)
    add_test(NAME retail_source_bsp_ranges COMMAND source_bsp_ranges_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_bsp_ranges_original.bin)
    set_tests_properties(retail_source_bsp_ranges PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_prt_contact_tests tests/source_prt_contact_tests.cpp)
    target_link_libraries(source_prt_contact_tests PRIVATE halo2_cache)
    target_compile_definitions(source_prt_contact_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME retail_source_prt_contact COMMAND source_prt_contact_tests)
    add_executable(source_bloom_context_tests tests/source_bloom_context_tests.cpp)
    target_link_libraries(source_bloom_context_tests PRIVATE halo2_cache)
    target_compile_definitions(source_bloom_context_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME retail_source_bloom_context COMMAND source_bloom_context_tests)
    add_executable(source_object_light_mix_tests tests/source_object_light_mix_tests.cpp)
    target_link_libraries(source_object_light_mix_tests PRIVATE halo2_cache)
    target_compile_definitions(source_object_light_mix_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME retail_source_object_light_mix COMMAND source_object_light_mix_tests)
    add_executable(source_object_posed_target_tests tests/source_object_posed_target_tests.cpp)
    target_link_libraries(source_object_posed_target_tests PRIVATE halo2_cache)
    target_compile_definitions(source_object_posed_target_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME retail_source_object_posed_target COMMAND source_object_posed_target_tests)
    add_executable(source_object_light_uv_tests tests/source_object_light_uv_tests.cpp)
    target_link_libraries(source_object_light_uv_tests PRIVATE halo2_cache)
    target_compile_definitions(source_object_light_uv_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME retail_source_object_light_uv COMMAND source_object_light_uv_tests)

    add_executable(source_object_light_sample_tests tests/source_object_light_sample_tests.cpp)
    target_link_libraries(source_object_light_sample_tests PRIVATE halo2_cache)
    target_compile_definitions(source_object_light_sample_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME retail_source_object_light_sample COMMAND source_object_light_sample_tests)
    target_link_libraries(source_draw_unit_cull_tests PRIVATE halo2_cache)
    target_link_libraries(source_bsp_draw_collector_tests PRIVATE halo2_cache)
    target_compile_definitions(source_bsp_draw_collector_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    if(MSVC)
        target_compile_options(source_bsp_draw_collector_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_bsp_draw_collector_tests PRIVATE -fno-fast-math -ffp-contract=off)
    endif()
    add_test(NAME original_effect_light_bsp_draw_collector COMMAND source_bsp_draw_collector_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_bsp_draw_collector.original.bin"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    add_executable(source_bsp_effect_lights_tests tests/source_bsp_effect_lights_tests.cpp)
    add_executable(mcc_fp_sphere_material_tests tests/mcc_fp_sphere_material_tests.cpp)
    target_link_libraries(mcc_fp_sphere_material_tests PRIVATE halo2_systems)
    set(HALO2_MCC_FP_LIGHT_MATERIAL_MAP "" CACHE FILEPATH "Prepared MCC map for first-person sphere material regression")
    add_test(NAME original_tu15_mcc_fp_sphere_materials COMMAND mcc_fp_sphere_material_tests "${HALO2_MCC_FP_LIGHT_MATERIAL_MAP}")
    set_tests_properties(original_tu15_mcc_fp_sphere_materials PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(source_bsp_effect_lights_tests PRIVATE halo2_systems halo2_menu_world_storage)
    add_test(NAME original_live_bsp_effect_light_draws COMMAND source_bsp_effect_lights_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_bsp_effect_light.original.json")
    add_test(NAME original_tu15_live_bsp_effect_light_draws COMMAND source_bsp_effect_lights_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/live_bsp_effect_light.tu15.original.json")
    add_executable(rumble_asset_tests tests/rumble_asset_tests.cpp)
    add_executable(player_screen_flash_tests tests/player_screen_flash_tests.cpp)
    add_executable(player_screen_flash_kernel_tests tests/player_screen_flash_tests.cpp src/gameplay/units/player_screen_flash.cpp src/core/kernels.cpp)
    target_include_directories(player_screen_flash_kernel_tests PRIVATE include)
    target_compile_definitions(player_screen_flash_kernel_tests PRIVATE HALO2_FLASH_KERNEL_ONLY=1)
    target_link_libraries(player_screen_flash_kernel_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_player_screen_flash_kernel COMMAND player_screen_flash_kernel_tests
        unused "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/player_screen_flash_tu15/damage-flash.tsv"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/player_screen_flash_tu15/original.json")
    add_executable(player_screen_flash_render_tests tests/player_screen_flash_render_tests.cpp)
    target_link_libraries(player_screen_flash_render_tests PRIVATE halo2_renderer)
    add_test(NAME native_player_screen_flash_render COMMAND player_screen_flash_render_tests
        "${CMAKE_CURRENT_BINARY_DIR}/player-screen-flash-render")
    set_tests_properties(native_player_screen_flash_render PROPERTIES TIMEOUT 120)
    if(UNIX AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib/libVkLayer_khronos_validation.so")
        set_tests_properties(native_player_screen_flash_render PROPERTIES ENVIRONMENT
            "VK_LAYER_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d;LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib:$ENV{LD_LIBRARY_PATH}")
    endif()
    target_sources(player_screen_flash_tests PRIVATE src/gameplay/units/player_screen_flash.cpp src/gameplay/units/player_screen_flash_content.cpp src/core/kernels.cpp)
    target_link_libraries(player_screen_flash_tests PRIVATE halo2_cache)
    add_test(NAME original_tu15_player_screen_flash COMMAND player_screen_flash_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/player_screen_flash_tu15/damage-flash.tsv"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/player_screen_flash_tu15/original.json")
    add_executable(player_screen_flash_mcc_tests tests/player_screen_flash_mcc_tests.cpp
        src/gameplay/units/player_screen_flash.cpp src/gameplay/units/player_screen_flash_content.cpp src/core/kernels.cpp)
    target_link_libraries(player_screen_flash_mcc_tests PRIVATE halo2_cache)
    set(HALO2_MCC_PLAYER_FLASH_MAPS "" CACHE PATH "Prepared MCC maps for all player screen flash metadata checks")
    set(HALO2_MCC_PLAYER_FLASH_INVENTORY "" CACHE FILEPATH "Imported player screen flash inventory")
    add_test(NAME mcc_all_player_screen_flashes COMMAND player_screen_flash_mcc_tests
        "${HALO2_MCC_PLAYER_FLASH_MAPS}" "${HALO2_MCC_PLAYER_FLASH_INVENTORY}")
    set_tests_properties(mcc_all_player_screen_flashes PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
    add_test(NAME mcc_player_screen_flash_import COMMAND ${Python3_EXECUTABLE} -m unittest discover
        -s "${CMAKE_CURRENT_SOURCE_DIR}/tests" -p test_mcc_player_screen_flashes.py)
    target_link_libraries(rumble_asset_tests PRIVATE halo2_systems SDL2::SDL2)
    add_test(NAME original_shared_rumble_metadata COMMAND rumble_asset_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/midship.map")
    target_compile_definitions(source_draw_unit_cull_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    if(MSVC)
        target_compile_options(source_draw_unit_cull_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_draw_unit_cull_tests PRIVATE -fno-fast-math -ffp-contract=off)
    endif()
    add_test(NAME retail_source_draw_unit_cull COMMAND source_draw_unit_cull_tests)
    add_test(NAME original_effect_light_shared_mask_cull COMMAND source_draw_unit_cull_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_shared_mask_cull.original.tsv")
    add_test(NAME original_effect_light_draw_unit_cull COMMAND source_draw_unit_cull_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_draw_unit_cull.original.tsv)
    add_executable(source_local_player_priority_tests tests/source_local_player_priority_tests.cpp)
    target_link_libraries(source_local_player_priority_tests PRIVATE halo2_cache)
    target_compile_definitions(source_local_player_priority_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME retail_source_local_player_priority COMMAND source_local_player_priority_tests)
    add_executable(source_object_render_queue_tests tests/source_object_render_queue_tests.cpp)
    target_link_libraries(source_object_render_queue_tests PRIVATE halo2_cache)
    target_compile_definitions(source_object_render_queue_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME retail_source_object_render_queue COMMAND source_object_render_queue_tests)
    add_executable(source_object_render_entry_tests tests/source_object_render_entry_tests.cpp)
    target_link_libraries(source_object_render_entry_tests PRIVATE halo2_cache)
    target_compile_definitions(source_object_render_entry_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_source_object_render_entry COMMAND source_object_render_entry_tests)
    add_executable(source_geometry_resource_tests tests/source_geometry_resource_tests.cpp)
    target_link_libraries(source_geometry_resource_tests PRIVATE halo2_cache)
    target_compile_definitions(source_geometry_resource_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_source_geometry_resource COMMAND source_geometry_resource_tests)
    add_executable(source_geometry_request_tests tests/source_geometry_request_tests.cpp)
    target_link_libraries(source_geometry_request_tests PRIVATE halo2_cache)
    target_compile_definitions(source_geometry_request_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_source_geometry_request COMMAND source_geometry_request_tests)
    add_executable(source_geometry_acquire_tests tests/source_geometry_acquire_tests.cpp)
    target_link_libraries(source_geometry_acquire_tests PRIVATE halo2_systems)
    target_compile_definitions(source_geometry_acquire_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_source_geometry_acquire COMMAND source_geometry_acquire_tests)
    add_executable(source_geometry_request_joined_tests tests/source_geometry_request_joined_tests.cpp)
    target_link_libraries(source_geometry_request_joined_tests PRIVATE halo2_systems)
    target_compile_definitions(source_geometry_request_joined_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_source_geometry_request_joined COMMAND source_geometry_request_joined_tests)
    add_executable(source_physical_cache_tests tests/source_physical_cache_tests.cpp)
    target_link_libraries(source_physical_cache_tests PRIVATE halo2_systems)
    target_compile_definitions(source_physical_cache_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_source_physical_cache COMMAND source_physical_cache_tests)
    add_executable(source_geometry_retirement_tests tests/source_geometry_retirement_tests.cpp)
    target_link_libraries(source_geometry_retirement_tests PRIVATE halo2_systems)
    target_compile_definitions(source_geometry_retirement_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_source_geometry_retirement COMMAND source_geometry_retirement_tests)
    add_executable(source_geometry_cache_joined_tests tests/source_geometry_cache_joined_tests.cpp)
    target_link_libraries(source_geometry_cache_joined_tests PRIVATE halo2_systems)
    target_compile_definitions(source_geometry_cache_joined_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_source_geometry_cache_joined COMMAND source_geometry_cache_joined_tests)
    add_executable(source_object_render_flags_tests tests/source_object_render_flags_tests.cpp)
    target_link_libraries(source_object_render_flags_tests PRIVATE halo2_cache)
    add_test(NAME original_source_object_render_flags COMMAND source_object_render_flags_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_object_owner_flag31_original.tsv)
    set_tests_properties(original_source_object_render_flags PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_shadow_plan_tests tests/source_shadow_plan_tests.cpp)
    target_link_libraries(source_shadow_plan_tests PRIVATE halo2_cache)
    target_compile_definitions(source_shadow_plan_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    if(MSVC)
        target_compile_options(source_shadow_plan_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_shadow_plan_tests PRIVATE -ffp-contract=off)
    endif()
    add_test(NAME original_shadow_plan COMMAND source_shadow_plan_tests)
    set_tests_properties(original_shadow_plan PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_shadow_raw_draw_tests tests/source_shadow_raw_draw_tests.cpp)
    target_link_libraries(source_shadow_raw_draw_tests PRIVATE halo2_cache)
    target_compile_definitions(source_shadow_raw_draw_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    if(MSVC)
        target_compile_options(source_shadow_raw_draw_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_shadow_raw_draw_tests PRIVATE -fno-fast-math -ffp-contract=off)
    endif()
    add_test(NAME original_tu15_shadow_raw_draws COMMAND source_shadow_raw_draw_tests)
    set_tests_properties(original_tu15_shadow_raw_draws PROPERTIES SKIP_RETURN_CODE 77)
    foreach(shadow_probe_shader source_shadow_probe.vert source_shadow_probe.frag source_shadow_retained.vert source_shadow_retained.frag)
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/${shadow_probe_shader}.spv
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
            COMMAND ${GLSLANG_VALIDATOR} -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
                ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shadow_probe_shader}
                -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/${shadow_probe_shader}.spv
            DEPENDS shaders/${shadow_probe_shader} shaders/combiner.glsl shaders/texture_program.glsl shaders/vertex_program.glsl VERBATIM)
    endforeach()
    add_custom_target(source_shadow_probe_shaders DEPENDS
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/source_shadow_probe.vert.spv
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/source_shadow_probe.frag.spv
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/source_shadow_retained.vert.spv
        ${CMAKE_CURRENT_BINARY_DIR}/shaders/source_shadow_retained.frag.spv)
    add_executable(source_shadow_render_tests tests/source_shadow_render_tests.cpp)
    target_link_libraries(source_shadow_render_tests PRIVATE halo2_shadow_render Vulkan::Vulkan)
    target_compile_definitions(source_shadow_render_tests PRIVATE
        HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        HALO2_SHADOW_PROBE_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
    add_dependencies(source_shadow_render_tests source_shadow_probe_shaders)
    add_test(NAME original_shadow_render_gpu COMMAND source_shadow_render_tests)
    set_tests_properties(original_shadow_render_gpu PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_shadow_silhouette_tests tests/source_shadow_silhouette_tests.cpp)
    target_link_libraries(source_shadow_silhouette_tests PRIVATE halo2_shadow_render Vulkan::Vulkan)
    target_compile_definitions(source_shadow_silhouette_tests PRIVATE
        HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        HALO2_SHADOW_PROBE_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
    add_dependencies(source_shadow_silhouette_tests source_shadow_probe_shaders)
    add_test(NAME original_shadow_silhouette_gpu COMMAND source_shadow_silhouette_tests)
    add_executable(source_shadow_renderer_tests tests/source_shadow_renderer_tests.cpp)
    target_link_libraries(source_shadow_renderer_tests PRIVATE halo2_renderer)
    target_compile_definitions(source_shadow_renderer_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_shadow_renderer_gpu COMMAND source_shadow_renderer_tests
        ${CMAKE_CURRENT_BINARY_DIR}/shadow-renderer-results)
    set_tests_properties(original_shadow_renderer_gpu PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(effect_light_renderer_tests tests/effect_light_renderer_tests.cpp)
    target_link_libraries(effect_light_renderer_tests PRIVATE halo2_renderer)
    target_compile_definitions(effect_light_renderer_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_effect_light_renderer_gpu COMMAND effect_light_renderer_tests
        ${CMAKE_CURRENT_BINARY_DIR}/effect-light-renderer-results)
    set_tests_properties(original_effect_light_renderer_gpu PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_effect_light_authored_colors_gpu COMMAND effect_light_renderer_tests --authored-colors
        ${CMAKE_CURRENT_BINARY_DIR}/effect-light-authored-colors-results)
    set_tests_properties(original_effect_light_authored_colors_gpu PROPERTIES SKIP_RETURN_CODE 77)
    if(TARGET menu_world_vertex_catalog)
        add_executable(hologram_draw_gpu_tests tests/hologram_draw_gpu_tests.cpp
            src/ui/menu_world_native_draw.cpp src/ui/menu_world_native_packet.cpp src/rendering/source_scene_copy_gpu.cpp)
        target_link_libraries(hologram_draw_gpu_tests PRIVATE halo2_menu_world_storage)
        target_compile_definitions(hologram_draw_gpu_tests PRIVATE HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
        target_include_directories(hologram_draw_gpu_tests PRIVATE ${MENU_WORLD_VERTEX_CATALOG_DIR})
        add_dependencies(hologram_draw_gpu_tests menu_world_vertex_catalog halo2_shaders menu_world_geometry_shader menu_world_fragment_shader)
        add_test(NAME original_hologram_raw_gpu COMMAND hologram_draw_gpu_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map
            ${CMAKE_CURRENT_BINARY_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/hologram-draw/report.json)
        set_tests_properties(original_hologram_raw_gpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map")
            add_test(NAME original_hologram_triangle_carrier_gpu COMMAND hologram_draw_gpu_tests
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map
                ${CMAKE_CURRENT_BINARY_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/hologram-carrier/report.json --triangle-carrier)
            set_tests_properties(original_hologram_triangle_carrier_gpu PROPERTIES TIMEOUT 180)
            if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
                add_test(NAME original_hologram_depth_differential_gpu COMMAND hologram_draw_gpu_tests
                    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map
                    ${CMAKE_CURRENT_BINARY_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/hologram-depth-differential/report.json --depth-differential)
                set_tests_properties(original_hologram_depth_differential_gpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
                if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json")
                    set_tests_properties(original_hologram_depth_differential_gpu PROPERTIES ENVIRONMENT
                        "VK_LAYER_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d;LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib:$ENV{LD_LIBRARY_PATH}")
                endif()
            endif()
        endif()
        if(APPLE)
            add_subdirectory(tests/macos/native_world_draw)
        endif()
        add_executable(source_effect_light_frame_tests tests/source_effect_light_frame_tests.cpp)
        target_link_libraries(source_effect_light_frame_tests PRIVATE halo2_renderer)
        target_include_directories(source_effect_light_frame_tests PRIVATE ${MENU_WORLD_VERTEX_CATALOG_DIR})
        if(UNIX AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/renderdoc/usr/include/renderdoc_app.h")
            target_include_directories(source_effect_light_frame_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/.tools/renderdoc/usr/include")
            target_link_libraries(source_effect_light_frame_tests PRIVATE ${CMAKE_DL_LIBS})
        endif()
        add_dependencies(source_effect_light_frame_tests menu_world_vertex_catalog)
        add_executable(effect_light_raw_renderer_tests tests/effect_light_raw_renderer_tests.cpp)
        target_link_libraries(effect_light_raw_renderer_tests PRIVATE halo2_renderer)
        target_compile_definitions(effect_light_raw_renderer_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        add_test(NAME original_effect_light_raw_renderer_gpu COMMAND ${CMAKE_COMMAND}
            -DTEST_BINARY=$<TARGET_FILE:effect_light_raw_renderer_tests>
            -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer-shading/world-effects-goal-20261011/light-raw-bindings-oracle-v2
            -DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/effect-light-raw-renderer-results
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_effect_light_raw_renderer.cmake)
    endif()
    add_executable(effect_light_material_tests tests/effect_light_material_tests.cpp)
    target_link_libraries(effect_light_material_tests PRIVATE halo2_systems)
    target_compile_definitions(effect_light_material_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_effect_light_materials COMMAND effect_light_material_tests)
    set_tests_properties(original_effect_light_materials PROPERTIES SKIP_RETURN_CODE 77)
    set_tests_properties(original_shadow_silhouette_gpu PROPERTIES SKIP_RETURN_CODE 77)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/weather_vertex.comp.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${GLSLANG_VALIDATOR} -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
            ${CMAKE_CURRENT_SOURCE_DIR}/shaders/weather_vertex.comp
            -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/weather_vertex.comp.spv
        DEPENDS shaders/weather_vertex.comp shaders/vertex_program.glsl shaders/render_view.glsl VERBATIM)
    add_custom_target(weather_vertex_test_shader DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/weather_vertex.comp.spv)
    add_executable(weather_vertex_tests tests/weather_vertex_tests.cpp)
    target_link_libraries(weather_vertex_tests PRIVATE halo2_cache Vulkan::Vulkan)
    target_compile_definitions(weather_vertex_tests PRIVATE
        HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
        HALO2_WEATHER_VERTEX_TEST_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/weather_vertex.comp.spv")
    add_dependencies(weather_vertex_tests weather_vertex_test_shader)
    add_test(NAME source_weather_vertex_cpu COMMAND weather_vertex_tests)
    add_test(NAME source_weather_vertex_gpu COMMAND weather_vertex_tests --gpu)
    set_tests_properties(source_weather_vertex_cpu source_weather_vertex_gpu PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(surface_format_tests tests/surface_format_tests.cpp)
    target_include_directories(surface_format_tests PRIVATE include)
    target_link_libraries(surface_format_tests PRIVATE Vulkan::Vulkan)
    add_test(NAME native_display_transfer COMMAND surface_format_tests)
    add_executable(frontend_tests tests/frontend_tests.cpp)
    add_executable(discord_login_tests tests/discord_login_tests.cpp)
    add_executable(steam_login_tests tests/steam_login_tests.cpp)
    target_link_libraries(steam_login_tests PRIVATE halo2_discord_identity CURL::libcurl OpenSSL::Crypto nlohmann_json::nlohmann_json)
    add_test(NAME steam_identity COMMAND steam_login_tests)
    add_test(NAME steam_native_http COMMAND ${Python3_EXECUTABLE} -m tests.steam_login_http_test $<TARGET_FILE:online_login_probe>)
    set_tests_properties(steam_native_http PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} TIMEOUT 30)
    add_executable(online_directory_tests tests/online_directory_tests.cpp)
    target_link_libraries(online_directory_tests PRIVATE halo2_online_directory nlohmann_json::nlohmann_json)
    add_test(NAME online_directory_client COMMAND online_directory_tests)
    add_executable(online_matchmaking_tests tests/online_matchmaking_tests.cpp)
    target_link_libraries(online_matchmaking_tests PRIVATE halo2_online_matchmaking CURL::libcurl nlohmann_json::nlohmann_json)
    add_test(NAME online_matchmaking_client COMMAND online_matchmaking_tests)
    add_test(NAME online_search_qos_client COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_search_qos_http_test.py $<TARGET_FILE:online_search_qos_tests>)
    add_executable(online_account_tests tests/online_account_tests.cpp)
    target_link_libraries(online_account_tests PRIVATE halo2_online_account CURL::libcurl nlohmann_json::nlohmann_json)
    add_test(NAME online_account_client COMMAND online_account_tests)
    # HTTP fixtures exercise retries without making live EOS identity calls.
    set_tests_properties(online_account_client PROPERTIES ENVIRONMENT "H2_EOS_PROFILES=0")
    add_executable(discord_party_activity_tests tests/discord_party_activity_tests.cpp)
    target_link_libraries(discord_party_activity_tests PRIVATE halo2_online_account CURL::libcurl nlohmann_json::nlohmann_json)
    add_test(NAME discord_party_activity COMMAND discord_party_activity_tests)
    set_tests_properties(discord_party_activity PROPERTIES TIMEOUT 30)
    add_executable(discord_activity_inbox_tests tests/discord_activity_inbox_tests.cpp)
    target_link_libraries(discord_activity_inbox_tests PRIVATE halo2_discord_inbox)
    add_test(NAME discord_activity_inbox COMMAND discord_activity_inbox_tests)
    add_executable(online_party_activity_http_tests tests/online_party_activity_http_tests.cpp)
    target_link_libraries(online_party_activity_http_tests PRIVATE halo2_online_account halo2_discord_inbox CURL::libcurl)
    add_test(NAME online_party_activity_http COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_party_activity_http_test.py $<TARGET_FILE:online_party_activity_http_tests>)
    set_tests_properties(online_party_activity_http PROPERTIES TIMEOUT 40 SKIP_RETURN_CODE 77)
    add_executable(online_voice_tests tests/online_voice_tests.cpp)
    target_link_libraries(online_voice_tests PRIVATE halo2_online_voice nlohmann_json::nlohmann_json PkgConfig::OPUS)
    add_test(NAME online_voice_codec COMMAND online_voice_tests)
    add_test(NAME game_voice_policy COMMAND game_voice_tests)
    add_test(NAME online_match_launch COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_match_launch_http_test.py $<TARGET_FILE:online_match_launch_tests> core)
    set_tests_properties(online_match_launch PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    add_test(NAME online_source_rendered_workload COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_match_launch_http_test.py $<TARGET_FILE:online_match_launch_tests> source-render casual)
    set_tests_properties(online_source_rendered_workload PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77)
    add_test(NAME online_source_runtime_recovery COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_match_launch_http_test.py $<TARGET_FILE:online_match_launch_tests> source-render casual)
    set_tests_properties(online_source_runtime_recovery PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77
        ENVIRONMENT "HALO2_RUNTIME_RETENTION=1")
    add_test(NAME online_source_runtime_stalled_host COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_match_launch_http_test.py $<TARGET_FILE:online_match_launch_tests> source-render casual)
    set_tests_properties(online_source_runtime_stalled_host PROPERTIES TIMEOUT 240 SKIP_RETURN_CODE 77
        ENVIRONMENT "HALO2_RUNTIME_RETENTION=1;HALO2_RUNTIME_STALLED_HOST=1")
    foreach(couch_mode core mixed)
        add_test(NAME online_couch_${couch_mode} COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_couch_launch_http_test.py $<TARGET_FILE:online_couch_launch_tests> ${couch_mode})
        set_tests_properties(online_couch_${couch_mode} PROPERTIES TIMEOUT 260 SKIP_RETURN_CODE 77)
    endforeach()
    add_test(NAME online_match_team_launch COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_match_launch_http_test.py $<TARGET_FILE:online_match_launch_tests> team)
    set_tests_properties(online_match_team_launch PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    add_test(NAME online_match_party_launch COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_match_launch_http_test.py $<TARGET_FILE:online_match_launch_tests> party)
    set_tests_properties(online_match_party_launch PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    add_test(NAME online_match_casual_launch COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_match_launch_http_test.py $<TARGET_FILE:online_match_launch_tests> core casual)
    set_tests_properties(online_match_casual_launch PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    add_test(NAME online_match_casual_party_launch COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_match_launch_http_test.py $<TARGET_FILE:online_match_launch_tests> party casual)
    set_tests_properties(online_match_casual_party_launch PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    add_test(NAME online_match_minor_clan_launch COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_match_launch_http_test.py $<TARGET_FILE:online_match_launch_tests> clan)
    set_tests_properties(online_match_minor_clan_launch PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    add_test(NAME online_match_major_clan_launch COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_match_launch_http_test.py $<TARGET_FILE:online_match_launch_tests> clan-major)
    set_tests_properties(online_match_major_clan_launch PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    add_test(NAME online_match_full_clan_launch COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_match_launch_http_test.py $<TARGET_FILE:online_match_launch_tests> clan-full)
    set_tests_properties(online_match_full_clan_launch PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    foreach(gathering_case IN ITEMS ranked_party casual_party full_clan)
        if(gathering_case STREQUAL "ranked_party")
            set(gathering_arguments party ranked)
        elseif(gathering_case STREQUAL "casual_party")
            set(gathering_arguments party casual)
        else()
            set(gathering_arguments clan-full)
        endif()
        add_test(NAME online_gathering_${gathering_case} COMMAND ${CMAKE_COMMAND} -E env HALO2_MATCHMAKING_GATHERINGS=1
            ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_match_launch_http_test.py
            $<TARGET_FILE:online_match_launch_tests> ${gathering_arguments})
        set_tests_properties(online_gathering_${gathering_case} PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77 RUN_SERIAL TRUE)
    endforeach()
    add_test(NAME online_match_variant_mismatch COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_match_launch_http_test.py $<TARGET_FILE:online_match_launch_tests> mismatch)
    set_tests_properties(online_match_variant_mismatch PROPERTIES TIMEOUT 90 SKIP_RETURN_CODE 77)
    add_test(NAME online_voice_http COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_voice_http_test.py $<TARGET_FILE:online_voice_tests>)
    set_tests_properties(online_voice_http PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 70 RUN_SERIAL TRUE)
    add_test(NAME online_party_voice_http COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_voice_http_test.py $<TARGET_FILE:online_voice_tests> party)
    set_tests_properties(online_party_voice_http PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 70 RUN_SERIAL TRUE)
    find_program(HALO2_CADDY_EXECUTABLE NAMES caddy)
    if(HALO2_CADDY_EXECUTABLE)
        foreach(voice_group IN ITEMS clan party)
            add_test(NAME online_${voice_group}_voice_proxy COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_voice_http_test.py $<TARGET_FILE:online_voice_tests>
                ${voice_group} --caddy ${HALO2_CADDY_EXECUTABLE})
            set_tests_properties(online_${voice_group}_voice_proxy PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 70 RUN_SERIAL TRUE)
        endforeach()
    endif()
    add_test(NAME online_directory_http COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_directory_http_test.py $<TARGET_FILE:online_directory_tests>)
    set_tests_properties(online_directory_http PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 45)
    target_link_libraries(discord_login_tests PRIVATE halo2_discord_identity CURL::libcurl OpenSSL::Crypto)
    add_test(NAME discord_identity COMMAND discord_login_tests)
    add_executable(discord_social_tests tests/discord_social_tests.cpp)
    target_link_libraries(discord_social_tests PRIVATE halo2_discord_social halo2_discord_identity CURL::libcurl)
    target_link_libraries(discord_social_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME discord_social_consent COMMAND discord_social_tests)
    halo2_bundle_discord_social(discord_social_tests)
    add_test(NAME online_match_voice_blocks COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_match_launch_http_test.py $<TARGET_FILE:online_match_launch_tests> blocks)
    set_tests_properties(online_match_voice_blocks PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    add_executable(discord_friends_probe EXCLUDE_FROM_ALL tools/discord_friends_probe.cpp)
    target_link_libraries(discord_friends_probe PRIVATE halo2_discord_social halo2_discord_identity)
    halo2_bundle_discord_social(discord_friends_probe)
    add_executable(frontend_online_tests tests/frontend_online_tests.cpp)
    target_include_directories(frontend_online_tests PRIVATE src)
    target_link_libraries(frontend_online_tests PRIVATE halo2_renderer CURL::libcurl nlohmann_json::nlohmann_json)
    halo2_bundle_discord_social(frontend_online_tests)
    add_dependencies(frontend_online_tests halo2_shaders)
    set(HALO2_MCC_PARTY_UI_TEST_CACHE "" CACHE PATH "Prepared MCC cache root for native party roster regression")
    if(HALO2_MCC_PARTY_UI_TEST_CACHE)
        add_test(NAME frontend_online_identity COMMAND frontend_online_tests "${HALO2_MCC_PARTY_UI_TEST_CACHE}/halo2/h2_maps_win64_dx11")
    else()
        add_test(NAME frontend_online_identity COMMAND frontend_online_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
            --asset-sources ${CMAKE_CURRENT_SOURCE_DIR}/assets/asset-sources.tsv)
    endif()
    set_tests_properties(frontend_online_identity PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME frontend_matchmaking_http COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend_matchmaking_http_test.py $<TARGET_FILE:frontend_online_tests>)
    if(HALO2_MCC_PARTY_UI_TEST_CACHE)
        add_test(NAME mcc_frontend_rank_ui COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend_matchmaking_http_test.py $<TARGET_FILE:frontend_online_tests>
            --mcc-root "${HALO2_MCC_PARTY_UI_TEST_CACHE}" --rank-ui 1)
        set_tests_properties(mcc_frontend_rank_ui PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 180)
    endif()
    set_tests_properties(frontend_matchmaking_http PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 55 RUN_SERIAL TRUE)
    add_test(NAME frontend_clans_http COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend_clans_http_test.py $<TARGET_FILE:frontend_online_tests>)
    set_tests_properties(frontend_clans_http PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 70 RUN_SERIAL TRUE)
    if(HALO2_MCC_PARTY_UI_TEST_CACHE)
        add_test(NAME frontend_parties_http COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend_parties_http_test.py $<TARGET_FILE:frontend_online_tests>
            --mcc-root "${HALO2_MCC_PARTY_UI_TEST_CACHE}")
    else()
        add_test(NAME frontend_parties_http COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend_parties_http_test.py $<TARGET_FILE:frontend_online_tests>)
    endif()
    set_tests_properties(frontend_parties_http PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 100 RUN_SERIAL TRUE)
    set(HALO2_MCC_PARTY_UI_TEST_CACHE "" CACHE PATH "Prepared MCC cache root for native party roster regression")
    if(HALO2_MCC_PARTY_UI_TEST_CACHE)
        add_test(NAME mcc_frontend_parties_http COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend_parties_http_test.py $<TARGET_FILE:frontend_online_tests>
            --mcc-root "${HALO2_MCC_PARTY_UI_TEST_CACHE}" --party-render 1)
        set_tests_properties(mcc_frontend_parties_http PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 100 RUN_SERIAL TRUE)
    endif()
    # Exercise the actual device adapter without rebuilding the native world.
    add_executable(multiplayer_kbm_tests tests/multiplayer_kbm_tests.cpp
        src/input/multiplayer_kbm.cpp src/input/pc_settings.cpp)
    target_include_directories(multiplayer_kbm_tests PRIVATE include)
    target_link_libraries(multiplayer_kbm_tests PRIVATE SDL2::SDL2 halo2_glm)
    if(HALO2_SANITIZERS AND NOT MSVC)
        target_compile_options(multiplayer_kbm_tests PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(multiplayer_kbm_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME multiplayer_kbm_bindings COMMAND multiplayer_kbm_tests)
    add_executable(pc_mouse_look_tests tests/pc_mouse_look_tests.cpp
        src/input/pc_settings.cpp src/gameplay/camera/source_camera.cpp)
    target_include_directories(pc_mouse_look_tests PRIVATE include)
    target_link_libraries(pc_mouse_look_tests PRIVATE SDL2::SDL2 halo2_glm)
    add_test(NAME pc_mouse_look COMMAND pc_mouse_look_tests)
    add_executable(gameplay_mouse_capture_tests tests/gameplay_mouse_capture_tests.cpp)
    target_include_directories(gameplay_mouse_capture_tests PRIVATE include)
    target_link_libraries(gameplay_mouse_capture_tests PRIVATE SDL2::SDL2)
    if(UNIX AND NOT APPLE)
        find_program(HALO2_MOUSE_CAPTURE_XVFB_RUN xvfb-run)
    endif()
    if(HALO2_MOUSE_CAPTURE_XVFB_RUN)
        add_test(NAME gameplay_mouse_capture_cleanup COMMAND ${HALO2_MOUSE_CAPTURE_XVFB_RUN} -a
            ${CMAKE_COMMAND} -E env SDL_VIDEODRIVER=x11 $<TARGET_FILE:gameplay_mouse_capture_tests>)
    else()
        add_test(NAME gameplay_mouse_capture_cleanup COMMAND gameplay_mouse_capture_tests)
    endif()
    include(cmake/WeaponScopeParity.cmake)
    add_executable(multiplayer_scope_network_tests tests/multiplayer_scope_network_tests.cpp)
    target_link_libraries(multiplayer_scope_network_tests PRIVATE halo2_systems)
    add_executable(multiplayer_inventory_tests tests/multiplayer_inventory_tests.cpp)
    target_link_libraries(multiplayer_inventory_tests PRIVATE halo2_systems)
    add_test(NAME retail_multiplayer_inventory COMMAND multiplayer_inventory_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/inventory-source/native-retained-inventory/fixtures.tsv)
    set_tests_properties(retail_multiplayer_inventory PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_scope_camera_tests tests/unit_scope_camera_tests.cpp)
    add_executable(unit_zoom_hud_tests tests/unit_zoom_hud_tests.cpp)
    add_executable(hud_bitmap_geometry_tests tests/hud_bitmap_geometry_tests.cpp)
    add_executable(unit_weapon_selection_tests tests/unit_weapon_selection_tests.cpp)
    target_link_libraries(unit_weapon_selection_tests PRIVATE halo2_systems)
    add_test(NAME original_unit_weapon_selection COMMAND unit_weapon_selection_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/inventory-source/single-weapon-selection/native/fixtures.tsv)
    set_tests_properties(original_unit_weapon_selection PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(hud_bitmap_geometry_tests PRIVATE halo2_cache)
    add_test(NAME original_hud_bitmap_geometry COMMAND hud_bitmap_geometry_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/scope-bitmap-geometry/measured-complete/fixtures.tsv)
    set_tests_properties(original_hud_bitmap_geometry PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_weapon_switch_tests tests/unit_weapon_switch_tests.cpp)
    target_link_libraries(unit_weapon_switch_tests PRIVATE halo2_systems)
    add_test(NAME original_unit_weapon_switch COMMAND unit_weapon_switch_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/inventory-source/weapon-switch-native/fixtures.tsv)
    set_tests_properties(original_unit_weapon_switch PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_constructor_tests tests/unit_constructor_tests.cpp)
    target_link_libraries(unit_constructor_tests PRIVATE halo2_systems)
    add_test(NAME original_retained_unit_constructors COMMAND unit_constructor_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-constructors/native-frozen/fixtures.tsv)
    set_tests_properties(original_retained_unit_constructors PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_animation_selection_tests tests/unit_animation_selection_tests.cpp)
    target_link_libraries(unit_animation_selection_tests PRIVATE halo2_systems)
    target_compile_definitions(unit_animation_selection_tests PRIVATE HALO2_SELECTION_CACHE_TESTS=1)
    add_test(NAME original_unit_animation_selection COMMAND unit_animation_selection_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/animation-selection/native/fixtures.tsv)
    set_tests_properties(original_unit_animation_selection PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_unit_animation_selection COMMAND unit_animation_selection_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/animation-selection/native/fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
    set_tests_properties(retail_unit_animation_selection PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_animation_resolution_tests tests/unit_animation_resolution_tests.cpp)
    target_link_libraries(unit_animation_resolution_tests PRIVATE halo2_systems)
    target_compile_definitions(unit_animation_resolution_tests PRIVATE HALO2_RESOLUTION_CACHE_TESTS=1)
    add_test(NAME original_unit_animation_resolution COMMAND unit_animation_resolution_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/animation-resolution/native-owned/fixtures.tsv)
    set_tests_properties(original_unit_animation_resolution PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_unit_animation_resolution COMMAND unit_animation_resolution_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/animation-resolution/native-owned/fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
    set_tests_properties(retail_unit_animation_resolution PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_grenade_begin_tests tests/unit_grenade_begin_tests.cpp)
    target_link_libraries(unit_grenade_begin_tests PRIVATE halo2_systems)
    add_test(NAME original_unit_grenade_begin COMMAND unit_grenade_begin_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/grenade-begin-body/native-complete/fixtures.tsv)
    set_tests_properties(original_unit_grenade_begin PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(object_creation_request_tests tests/object_creation_request_tests.cpp)
    target_link_libraries(object_creation_request_tests PRIVATE halo2_systems)
    add_test(NAME original_object_creation_request COMMAND object_creation_request_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/object-creation-request-native/fixtures/fixtures.tsv)
    set_tests_properties(original_object_creation_request PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_scenario_request_tests tests/source_scenario_request_tests.cpp)
    target_link_libraries(source_scenario_request_tests PRIVATE halo2_systems)
    add_test(NAME original_source_scenario_request COMMAND source_scenario_request_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/scenario-request-measured/fixtures.tsv)
    add_test(NAME retail_source_scenario_request COMMAND source_scenario_request_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/damage-shields-goal-20261011/scenario-request-measured/fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
    set_tests_properties(original_source_scenario_request retail_source_scenario_request PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(object_type_construction_tests tests/object_type_construction_tests.cpp)
    target_link_libraries(object_type_construction_tests PRIVATE halo2_systems)
    add_test(NAME original_object_type_construction COMMAND object_type_construction_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/object-type-construction/fixtures.tsv)
    set_tests_properties(original_object_type_construction PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(object_construction_tests tests/object_construction_tests.cpp)
    target_link_libraries(object_construction_tests PRIVATE halo2_systems)
    add_test(NAME original_object_construction COMMAND object_construction_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/generic-object-constructor/fixtures.txt)
    set_tests_properties(original_object_construction PROPERTIES SKIP_RETURN_CODE 77)
    find_path(HALO2_TEST_JSON_INCLUDE nlohmann/json.hpp)
    if(HALO2_TEST_JSON_INCLUDE)
        add_executable(object_preparation_pressure_tests tests/object_preparation_pressure_tests.cpp)
        target_include_directories(object_preparation_pressure_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(object_preparation_pressure_tests PRIVATE halo2_systems)
        add_test(NAME original_object_preparation_pressure COMMAND object_preparation_pressure_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/object-preparation-pressure/controls-measured-v2/report.json)
        set_tests_properties(original_object_preparation_pressure PROPERTIES SKIP_RETURN_CODE 77)
    endif()
    add_executable(unit_action_dispatch_tests tests/unit_action_dispatch_tests.cpp)
    target_link_libraries(unit_action_dispatch_tests PRIVATE halo2_systems)
    add_test(NAME original_unit_action_dispatch COMMAND unit_action_dispatch_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-action-dispatch/native/fixtures.tsv)
    set_tests_properties(original_unit_action_dispatch PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_constructor_tail_tests tests/unit_constructor_tail_tests.cpp)
    target_link_libraries(unit_constructor_tail_tests PRIVATE halo2_systems)
    target_compile_definitions(unit_constructor_tail_tests PRIVATE HALO2_CONSTRUCTOR_TAIL_CACHE=1)
    add_test(NAME original_unit_constructor_tail COMMAND unit_constructor_tail_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-constructor-tail/native-fixtures.tsv)
    set_tests_properties(original_unit_constructor_tail PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_unit_constructor_tail COMMAND unit_constructor_tail_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-constructor-tail/native-fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
    set_tests_properties(retail_unit_constructor_tail PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_unit_constructor_tail_original_operands COMMAND unit_constructor_tail_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-constructor-tail/native-fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    set_tests_properties(retail_unit_constructor_tail_original_operands PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_alternate_request_tests tests/unit_alternate_request_tests.cpp)
    target_link_libraries(unit_alternate_request_tests PRIVATE halo2_systems)
    target_compile_definitions(unit_alternate_request_tests PRIVATE HALO2_ALTERNATE_REQUEST_CACHE=1)
    add_test(NAME original_unit_alternate_request COMMAND unit_alternate_request_tests
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-alternate-request/native-fixtures.tsv")
    set_tests_properties(original_unit_alternate_request PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_unit_alternate_request COMMAND unit_alternate_request_tests
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-alternate-request/native-fixtures.tsv"
        "${PROJECT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(retail_unit_alternate_request PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_unit_alternate_request_verified_image COMMAND unit_alternate_request_tests
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-alternate-request/native-fixtures.tsv"
        "${PROJECT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${PROJECT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03")
    set_tests_properties(retail_unit_alternate_request_verified_image PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(animation_async_constructor_tests tests/animation_async_constructor_tests.cpp)
    target_link_libraries(animation_async_constructor_tests PRIVATE halo2_systems halo2_native_synchronization)
    add_test(NAME original_animation_async_constructor COMMAND animation_async_constructor_tests
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/animation-async-construction/native-fixtures.tsv")
    set_tests_properties(original_animation_async_constructor PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(animation_async_worker_tests tests/animation_async_worker_tests.cpp)
    target_link_libraries(animation_async_worker_tests PRIVATE halo2_systems halo2_native_synchronization)
    add_test(NAME original_animation_async_worker COMMAND animation_async_worker_tests
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/animation-async-worker/native-fixtures.tsv"
        "${PROJECT_SOURCE_DIR}/assets/disc/maps")
    set_tests_properties(original_animation_async_worker PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(animation_async_native_worker_tests tests/animation_async_native_worker_tests.cpp)
    target_link_libraries(animation_async_native_worker_tests PRIVATE halo2_systems halo2_native_synchronization)
    add_test(NAME original_animation_async_native_worker COMMAND animation_async_native_worker_tests
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/animation-async-native-worker/native-fixtures.tsv"
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/animation-async-construction/native-fixtures.tsv"
        "${PROJECT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(original_animation_async_native_worker PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(native_execution_gate_tests tests/native_execution_gate_tests.cpp)
    target_link_libraries(native_execution_gate_tests PRIVATE halo2_native_synchronization)
    add_test(NAME native_execution_gate COMMAND native_execution_gate_tests)
    add_executable(sound_cache_worker_gate_tests tests/sound_cache_worker_gate_tests.cpp)
    target_link_libraries(sound_cache_worker_gate_tests PRIVATE halo2_systems halo2_native_synchronization)
    add_test(NAME original_sound_cache_worker_gate COMMAND sound_cache_worker_gate_tests
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/sound-cache-request-native/fixtures.tsv"
        "${PROJECT_SOURCE_DIR}/assets/disc/maps/shared.map")
    set_tests_properties(original_sound_cache_worker_gate PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(sound_create_cache_gate_tests tests/sound_create_cache_gate_tests.cpp)
    target_link_libraries(sound_create_cache_gate_tests PRIVATE halo2_systems halo2_native_synchronization)
    add_test(NAME original_sound_create_cache_gate COMMAND sound_create_cache_gate_tests
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/sound-create-cache-production-20261009/frozen/evidence/fixtures.tsv"
        "${PROJECT_SOURCE_DIR}/assets/disc/maps/shared.map")
    set_tests_properties(original_sound_create_cache_gate PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(animation_residency_capacity_tests tests/animation_residency_capacity_tests.cpp)
    target_link_libraries(animation_residency_capacity_tests PRIVATE halo2_systems)
    add_test(NAME original_animation_residency_capacity COMMAND animation_residency_capacity_tests
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/animation-residency/native-fixtures.tsv")
    set_tests_properties(original_animation_residency_capacity PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_pose_requests_tests tests/unit_pose_requests_tests.cpp)
    target_link_libraries(unit_pose_requests_tests PRIVATE halo2_systems)
    target_compile_definitions(unit_pose_requests_tests PRIVATE HALO2_POSE_REQUEST_CACHE=1)
    add_test(NAME original_unit_pose_requests COMMAND unit_pose_requests_tests
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-pose-requests/native-fixtures.tsv")
    set_tests_properties(original_unit_pose_requests PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_unit_pose_requests COMMAND unit_pose_requests_tests
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-pose-requests/native-fixtures.tsv"
        "${PROJECT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(retail_unit_pose_requests PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_unit_pose_requests_verified_image COMMAND unit_pose_requests_tests
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-pose-requests/native-fixtures.tsv"
        "${PROJECT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${PROJECT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03")
    set_tests_properties(retail_unit_pose_requests_verified_image PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_subchannel_caller_tests tests/unit_subchannel_caller_tests.cpp)
    target_link_libraries(unit_subchannel_caller_tests PRIVATE halo2_systems)
    target_compile_definitions(unit_subchannel_caller_tests PRIVATE HALO2_SUBCHANNEL_CALLER_CACHE=1)
    add_test(NAME original_unit_subchannel_caller COMMAND unit_subchannel_caller_tests
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-subchannel-caller/native-fixtures.tsv")
    set_tests_properties(original_unit_subchannel_caller PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_unit_subchannel_caller COMMAND unit_subchannel_caller_tests
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-subchannel-caller/native-fixtures.tsv"
        "${PROJECT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(retail_unit_subchannel_caller PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_unit_subchannel_caller_original_operands COMMAND unit_subchannel_caller_tests
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-subchannel-caller/native-fixtures.tsv"
        "${PROJECT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${PROJECT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03")
    set_tests_properties(retail_unit_subchannel_caller_original_operands PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_channel_request_tests tests/unit_channel_request_tests.cpp)
    target_link_libraries(unit_channel_request_tests PRIVATE halo2_systems)
    target_compile_definitions(unit_channel_request_tests PRIVATE HALO2_CHANNEL_REQUEST_CACHE=1)
    add_test(NAME original_unit_channel_request COMMAND unit_channel_request_tests
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-channel-request/native-fixtures.tsv")
    set_tests_properties(original_unit_channel_request PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_unit_channel_request COMMAND unit_channel_request_tests
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-channel-request/native-fixtures.tsv"
        "${PROJECT_SOURCE_DIR}/assets/disc/maps/lockout.map")
    set_tests_properties(retail_unit_channel_request PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_unit_channel_request_original_operands COMMAND unit_channel_request_tests
        "${PROJECT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-channel-request/native-fixtures.tsv"
        "${PROJECT_SOURCE_DIR}/assets/disc/maps/lockout.map"
        "${PROJECT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03")
    set_tests_properties(retail_unit_channel_request_original_operands PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_local_transition_tests tests/unit_local_transition_tests.cpp)
    target_link_libraries(unit_local_transition_tests PRIVATE halo2_systems)
    target_compile_definitions(unit_local_transition_tests PRIVATE HALO2_LOCAL_TRANSITION_CACHE=1)
    add_test(NAME original_unit_local_transition COMMAND unit_local_transition_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-local-transition/native-fixtures.tsv)
    set_tests_properties(original_unit_local_transition PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_unit_local_transition COMMAND unit_local_transition_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-local-transition/native-fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
    set_tests_properties(retail_unit_local_transition PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_unit_local_transition_original_operands COMMAND unit_local_transition_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/spawn-source/unit-local-transition/native-fixtures.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    set_tests_properties(retail_unit_local_transition_original_operands PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(unit_zoom_hud_tests PRIVATE halo2_systems)
    add_test(NAME original_unit_zoom_hud COMMAND unit_zoom_hud_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/scope-presentation/controls-measured/fixtures.tsv)
    set_tests_properties(original_unit_zoom_hud PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(unit_scope_camera_tests PRIVATE halo2_systems)
    add_test(NAME original_unit_scope_camera COMMAND unit_scope_camera_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/scope-camera/measured-frozen/fixtures.tsv)
    set_tests_properties(original_unit_scope_camera PROPERTIES SKIP_RETURN_CODE 77)
    find_path(HALO2_TEST_JSON_INCLUDE nlohmann/json.hpp)
    if(HALO2_TEST_JSON_INCLUDE)
        add_executable(full_scoreboard_painter_tests tests/full_scoreboard_painter_tests.cpp)
        target_include_directories(full_scoreboard_painter_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(full_scoreboard_painter_tests PRIVATE halo2_cache)
        add_test(NAME original_full_scoreboard_painter COMMAND full_scoreboard_painter_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/title-update-1.5/scoreboard-painter/measured/report.json
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/fonts/conduit-13)
        set_tests_properties(original_full_scoreboard_painter PROPERTIES SKIP_RETURN_CODE 77)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/asset-sources.tsv")
            file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/assets/asset-sources.tsv" HALO2_SCOREBOARD_ASSET_ROWS REGEX "^vista\t")
            if(HALO2_SCOREBOARD_ASSET_ROWS)
                list(GET HALO2_SCOREBOARD_ASSET_ROWS 0 HALO2_SCOREBOARD_VISTA_ROW)
                string(REGEX REPLACE "^vista\t" "" HALO2_SCOREBOARD_VISTA_ASSETS "${HALO2_SCOREBOARD_VISTA_ROW}")
                add_test(NAME vista_full_scoreboard_artwork COMMAND full_scoreboard_painter_tests
                    ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/title-update-1.5/scoreboard-painter/measured/report.json
                    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/fonts/conduit-13
                    "${HALO2_SCOREBOARD_VISTA_ASSETS}/fonts/conduit-13")
                add_test(NAME vista_hud_shield_alignment COMMAND hud_vista_artwork_tests
                    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps "${HALO2_SCOREBOARD_VISTA_ASSETS}")
                add_test(NAME vista_hud_crosshairs COMMAND hud_vista_crosshair_tests
                    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps "${HALO2_SCOREBOARD_VISTA_ASSETS}")
                set_tests_properties(vista_hud_crosshairs PROPERTIES SKIP_RETURN_CODE 77)
                add_test(NAME vista_complete_scoreboard_gui COMMAND scoreboard_vista_gui_tests
                    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
                    ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
                    "${HALO2_SCOREBOARD_VISTA_ASSETS}")
                set_tests_properties(vista_complete_scoreboard_gui PROPERTIES SKIP_RETURN_CODE 77)
                set_tests_properties(vista_full_scoreboard_artwork vista_hud_shield_alignment PROPERTIES SKIP_RETURN_CODE 77)
            endif()
        endif()
        add_executable(full_scoreboard_assets_tests tests/full_scoreboard_assets_tests.cpp)
        target_include_directories(full_scoreboard_assets_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(full_scoreboard_assets_tests PRIVATE halo2_cache)
        add_test(NAME original_full_scoreboard_assets COMMAND full_scoreboard_assets_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/title-update-1.5/scoreboard-providers/measured/report.json
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/title-update-1.5/full-scoreboard/measured/report.json
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/title-update-1.5/scoreboard-painter/measured/report.json)
        set_tests_properties(original_full_scoreboard_assets PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(mcc_scoreboard_locale_tests tests/mcc_scoreboard_locale_tests.cpp)
        target_link_libraries(mcc_scoreboard_locale_tests PRIVATE halo2_cache)
        set(HALO2_MCC_SCOREBOARD_DONOR "" CACHE FILEPATH "Optional owned MCC map for scoreboard locale validation")
        set(HALO2_MCC_SCOREBOARD_REFERENCE "" CACHE FILEPATH "Optional matching original Xbox map for offline scoreboard validation")
        add_test(NAME mcc_scoreboard_locale COMMAND mcc_scoreboard_locale_tests
            "${HALO2_MCC_SCOREBOARD_DONOR}" "${HALO2_MCC_SCOREBOARD_REFERENCE}")
        set_tests_properties(mcc_scoreboard_locale PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
        add_executable(unit_scope_camera_data_tests tests/unit_scope_camera_data_tests.cpp)
        target_include_directories(unit_scope_camera_data_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(unit_scope_camera_data_tests PRIVATE halo2_cache)
        add_test(NAME original_unit_scope_camera_import COMMAND unit_scope_camera_data_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/scope-camera/audit-frozen/constants.json
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
        set_tests_properties(original_unit_scope_camera_import PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(full_scoreboard_providers_tests tests/full_scoreboard_providers_tests.cpp)
        target_include_directories(full_scoreboard_providers_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(full_scoreboard_providers_tests PRIVATE halo2_systems)
        add_test(NAME original_full_scoreboard_providers COMMAND full_scoreboard_providers_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/title-update-1.5/scoreboard-providers/measured/report.json)
        set_tests_properties(original_full_scoreboard_providers PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(team_full_scoreboard_tests tests/team_full_scoreboard_tests.cpp)
        target_include_directories(team_full_scoreboard_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(team_full_scoreboard_tests PRIVATE halo2_systems)
        add_test(NAME original_team_full_scoreboard COMMAND team_full_scoreboard_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/team_full_scoreboard_original.json
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/title-update-1.5/team-full-scoreboard/report.json
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/title-update-1.5/team-full-scoreboard/painter.json)
        set_tests_properties(original_team_full_scoreboard PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(full_scoreboard_tests tests/full_scoreboard_tests.cpp)
        add_executable(result_capture_tests tests/result_capture_tests.cpp
            src/gameplay/multiplayer/source_result_capture.cpp src/gameplay/multiplayer/source_result_statistics.cpp)
        target_include_directories(result_capture_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        add_test(NAME original_result_capture COMMAND result_capture_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/result_capture_original.json
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/result_statistics_original.json)
        add_executable(result_retention_tests tests/result_retention_tests.cpp src/gameplay/multiplayer/source_result_retention.cpp)
        add_executable(result_event_tests tests/result_event_tests.cpp src/gameplay/multiplayer/source_result_event.cpp)
        target_include_directories(result_event_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        add_test(NAME original_result_event COMMAND result_event_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/game-variants/king-recording-event-source-20261012/complete/report.json)
        add_executable(score_event_tests tests/score_event_tests.cpp src/gameplay/multiplayer/source_score_event.cpp)
        target_include_directories(score_event_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        add_test(NAME original_score_event COMMAND score_event_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/game-variants/score-events-20261012/complete/report.json)
        add_executable(king_visual_tests tests/king_visual_tests.cpp src/gameplay/multiplayer/source_king_visual.cpp)
        target_include_directories(king_visual_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        add_test(NAME original_king_visual COMMAND king_visual_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/game-variants/king-visual-consumers-source-20261012/complete/report.json
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/king_visual_team_colors_original.json)
        add_executable(king_boundary_tests tests/king_boundary_tests.cpp src/gameplay/multiplayer/source_king_boundary.cpp)
        target_include_directories(king_boundary_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        add_test(NAME original_king_boundary COMMAND king_boundary_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/game-variants/king-hill-emission-source-20261012/complete/report.json)
        add_executable(player_departure_tests tests/player_departure_tests.cpp
            src/gameplay/units/source_player_departure.cpp src/gameplay/multiplayer/source_result_retention.cpp src/ui/hud/full_scoreboard.cpp)
        target_include_directories(player_departure_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        add_test(NAME original_player_departure COMMAND player_departure_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/player_departure_original.json)
        add_executable(player_profile_tests tests/player_profile_tests.cpp src/gameplay/units/source_player_profile.cpp)
        target_include_directories(player_profile_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        add_test(NAME original_player_profile COMMAND player_profile_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/player_profile_original.json)
        add_executable(player_lives_tests tests/player_lives_tests.cpp src/gameplay/units/source_player_lives.cpp)
        target_include_directories(player_lives_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        add_test(NAME original_player_lives COMMAND player_lives_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/player_lives_original.json)
        add_executable(ctf_initialization_tests tests/ctf_initialization_tests.cpp src/gameplay/multiplayer/source_ctf.cpp)
        target_include_directories(ctf_initialization_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        add_test(NAME original_ctf_initialization COMMAND ctf_initialization_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/ctf_initialization_original.json)
        add_executable(ctf_activation_tu15_tests tests/ctf_activation_tu15_tests.cpp
            tests/ctf_activation_test_support_generated.cpp src/gameplay/multiplayer/source_ctf_activation_tu15_generated.cpp)
        target_include_directories(ctf_activation_tu15_tests PRIVATE include src tests ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(ctf_activation_tu15_tests PRIVATE OpenSSL::Crypto)
        if(MSVC)
            target_compile_options(ctf_activation_tu15_tests PRIVATE /fp:strict)
        else()
            target_compile_options(ctf_activation_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
        endif()
        add_test(NAME original_ctf_activation_tu15 COMMAND ctf_activation_tu15_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/ctf_activation_original_tu15.json)
        add_executable(ctf_configuration_tu15_tests tests/ctf_configuration_tu15_tests.cpp
            src/gameplay/multiplayer/source_ctf_configuration_tu15_generated.cpp
            src/gameplay/multiplayer/source_ctf_activation_tu15_generated.cpp)
        target_include_directories(ctf_configuration_tu15_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(ctf_configuration_tu15_tests PRIVATE OpenSSL::Crypto)
        if(MSVC)
            target_compile_options(ctf_configuration_tu15_tests PRIVATE /fp:strict)
        else()
            target_compile_options(ctf_configuration_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
        endif()
        add_test(NAME original_ctf_configuration_tu15 COMMAND ctf_configuration_tu15_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/ctf_configuration_original_tu15.json)
        add_executable(king_engine_tests tests/king_engine_tests.cpp src/gameplay/multiplayer/source_king.cpp src/gameplay/multiplayer/slayer.cpp)
        add_executable(king_replication_tests tests/king_replication_tests.cpp
            src/gameplay/multiplayer/source_king_replication.cpp src/gameplay/multiplayer/source_king.cpp src/gameplay/multiplayer/source_game_over.cpp src/audio/sound_classes.cpp)
        target_include_directories(king_replication_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        if(MSVC)
            target_compile_options(king_replication_tests PRIVATE /fp:strict)
        else()
            target_compile_options(king_replication_tests PRIVATE -fno-fast-math -ffp-contract=off)
        endif()
        add_test(NAME original_king_replication COMMAND king_replication_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/king_replication_original.json)
        add_executable(king_wire_tests tests/king_wire_tests.cpp src/gameplay/multiplayer/source_king_wire.cpp)
        target_include_directories(king_wire_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        add_test(NAME original_king_wire COMMAND king_wire_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/king_wire_original.json)
        add_executable(engine_entity_tests tests/engine_entity_tests.cpp
            src/gameplay/multiplayer/source_engine_entity.cpp src/gameplay/multiplayer/source_king_replication.cpp src/gameplay/multiplayer/source_king.cpp
            src/gameplay/multiplayer/source_game_over.cpp src/audio/sound_classes.cpp)
        target_include_directories(engine_entity_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        if(MSVC)
            target_compile_options(engine_entity_tests PRIVATE /fp:strict)
        else()
            target_compile_options(engine_entity_tests PRIVATE -fno-fast-math -ffp-contract=off)
        endif()
        add_test(NAME original_engine_entity COMMAND engine_entity_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/engine_entity_original.json)
        add_executable(engine_entry_tests tests/engine_entry_tests.cpp
            src/gameplay/multiplayer/source_engine_entry.cpp src/gameplay/multiplayer/source_engine_entity.cpp src/gameplay/multiplayer/source_king_replication.cpp
            src/gameplay/multiplayer/source_king.cpp src/gameplay/multiplayer/source_game_over.cpp src/audio/sound_classes.cpp)
        target_include_directories(engine_entry_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        if(MSVC)
            target_compile_options(engine_entry_tests PRIVATE /fp:strict)
        else()
            target_compile_options(engine_entry_tests PRIVATE -fno-fast-math -ffp-contract=off)
        endif()
        add_test(NAME original_engine_entry COMMAND engine_entry_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/engine_entry_original.json)
        add_executable(engine_entry_decode_tests tests/engine_entry_decode_tests.cpp
            src/gameplay/multiplayer/source_engine_entry_decode.cpp src/gameplay/multiplayer/source_engine_entity.cpp src/gameplay/multiplayer/source_king_wire.cpp
            src/gameplay/multiplayer/source_king_replication.cpp src/gameplay/multiplayer/source_king.cpp src/audio/sound_classes.cpp)
        target_include_directories(engine_entry_decode_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        if(MSVC)
            target_compile_options(engine_entry_decode_tests PRIVATE /fp:strict)
        else()
            target_compile_options(engine_entry_decode_tests PRIVATE -fno-fast-math -ffp-contract=off)
        endif()
        add_test(NAME original_engine_entry_decode COMMAND engine_entry_decode_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/engine_entry_decode_original.json)
        add_executable(king_callback_tests tests/king_callback_tests.cpp src/gameplay/multiplayer/source_king.cpp src/gameplay/multiplayer/source_result_statistics.cpp)
        target_include_directories(king_callback_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        if(MSVC)
            target_compile_options(king_callback_tests PRIVATE /fp:strict)
        else()
            target_compile_options(king_callback_tests PRIVATE -fno-fast-math -ffp-contract=off)
        endif()
        add_test(NAME original_king_callbacks COMMAND king_callback_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/king_callbacks_original.json)
        target_include_directories(king_engine_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        if(MSVC)
            target_compile_options(king_engine_tests PRIVATE /fp:strict)
        else()
            target_compile_options(king_engine_tests PRIVATE -fno-fast-math -ffp-contract=off)
        endif()
        add_test(NAME original_king_engine COMMAND king_engine_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/king_engine_original.json
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/king_scoring_original.json
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/king_world_original.json)
        add_executable(round_end_tests tests/round_end_tests.cpp)
        target_include_directories(round_end_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(round_end_tests PRIVATE halo2_systems)
        add_test(NAME original_round_end COMMAND round_end_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/round_end_original.json)
        add_executable(object_storage_release_tests tests/object_storage_release_tests.cpp)
        target_include_directories(object_storage_release_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(object_storage_release_tests PRIVATE halo2_systems)
        add_test(NAME original_object_storage_release COMMAND object_storage_release_tests
            ${CMAKE_SOURCE_DIR}/tests/fixtures/object_storage_release_original.json)
        add_executable(player_unit_tests tests/player_unit_tests.cpp src/gameplay/units/source_player_unit.cpp)
        target_include_directories(player_unit_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        add_test(NAME original_player_unit COMMAND player_unit_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/player_unit_original.json)
        add_executable(round_reset_tests tests/round_reset_tests.cpp src/gameplay/multiplayer/source_round_reset.cpp)
        target_include_directories(round_reset_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        add_test(NAME original_round_reset COMMAND round_reset_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/round_reset_original.json)
        add_executable(sound_source_lifecycle_tests tests/sound_source_lifecycle_tests.cpp)
        target_include_directories(sound_source_lifecycle_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(sound_source_lifecycle_tests PRIVATE halo2_cache)
        add_test(NAME original_sound_source_lifecycle COMMAND sound_source_lifecycle_tests
            ${CMAKE_SOURCE_DIR}/tests/fixtures/sound_source_lifecycle_original.json)
        add_executable(round_lifecycle_tests tests/round_lifecycle_tests.cpp)
        target_include_directories(round_lifecycle_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(round_lifecycle_tests PRIVATE halo2_systems)
        add_test(NAME original_round_lifecycle COMMAND round_lifecycle_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/round_lifecycle_original.json
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/round_lifecycle_cap_original.json
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/round_preparation_original.json)
        target_include_directories(result_retention_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(result_retention_tests PRIVATE PkgConfig::AVCODEC)
        add_test(NAME original_result_retention COMMAND result_retention_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/result_retention_original.json)
        add_executable(result_capture_data_tests tests/result_capture_data_tests.cpp)
        target_include_directories(result_capture_data_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(result_capture_data_tests PRIVATE halo2_cache)
        add_test(NAME original_result_capture_import COMMAND result_capture_data_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/result_capture_original.json
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
            "$ENV{HOME}/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
        set_tests_properties(original_result_capture_import PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(result_statistics_tests tests/result_statistics_tests.cpp
            src/gameplay/multiplayer/source_result_statistics.cpp src/gameplay/multiplayer/source_game_end.cpp)
        target_include_directories(result_statistics_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        add_test(NAME original_result_statistics COMMAND result_statistics_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/result_statistics_original.json)
        add_executable(result_statistics_data_tests tests/result_statistics_data_tests.cpp)
        target_include_directories(result_statistics_data_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(result_statistics_data_tests PRIVATE halo2_cache)
        add_test(NAME original_result_statistics_import COMMAND result_statistics_data_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/result_statistics_original.json
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
            "$ENV{HOME}/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
        set_tests_properties(original_result_statistics_import PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(session_game_over_tests tests/session_game_over_tests.cpp
            src/gameplay/multiplayer/source_session_game_over.cpp src/gameplay/multiplayer/source_game_over.cpp src/audio/sound_classes.cpp)
        target_include_directories(session_game_over_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        add_test(NAME original_session_game_over COMMAND session_game_over_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/session_game_over_original.json)
        add_executable(game_over_tests tests/game_over_tests.cpp src/gameplay/multiplayer/source_game_over.cpp src/audio/sound_classes.cpp)
        target_include_directories(game_over_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        add_test(NAME original_game_over COMMAND game_over_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/game_over_original.json)
        add_executable(game_over_data_tests tests/game_over_data_tests.cpp)
        target_include_directories(game_over_data_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(game_over_data_tests PRIVATE halo2_cache)
        add_test(NAME original_game_over_import COMMAND game_over_data_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/game_over_original.json
            ${CMAKE_CURRENT_SOURCE_DIR}
            "$ENV{HOME}/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
        set_tests_properties(original_game_over_import PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(game_end_tests tests/game_end_tests.cpp src/gameplay/multiplayer/source_game_end.cpp src/gameplay/multiplayer/source_result_statistics.cpp src/ui/hud/full_scoreboard.cpp)
        target_include_directories(game_end_tests PRIVATE include ${HALO2_TEST_JSON_INCLUDE})
        add_test(NAME original_game_end COMMAND game_end_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/game_end_original.json)
        target_include_directories(full_scoreboard_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(full_scoreboard_tests PRIVATE halo2_systems)
        add_test(NAME original_full_scoreboard COMMAND full_scoreboard_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/title-update-1.5/full-scoreboard/measured/report.json)
        set_tests_properties(original_full_scoreboard PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(full_scoreboard_data_tests tests/full_scoreboard_data_tests.cpp)
        target_include_directories(full_scoreboard_data_tests PRIVATE ${HALO2_TEST_JSON_INCLUDE})
        target_link_libraries(full_scoreboard_data_tests PRIVATE halo2_cache)
        add_test(NAME original_full_scoreboard_import COMMAND full_scoreboard_data_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/title-update-1.5/full-scoreboard/measured/report.json
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
        set_tests_properties(original_full_scoreboard_import PROPERTIES SKIP_RETURN_CODE 77)
    endif()
    add_executable(unit_zoom_tests tests/unit_zoom_tests.cpp)
    target_link_libraries(unit_zoom_tests PRIVATE halo2_systems)
    add_test(NAME original_unit_zoom COMMAND unit_zoom_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/unit-zoom/measured-frozen/fixtures.tsv)
    set_tests_properties(original_unit_zoom PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_zoom_data_tests tests/unit_zoom_data_tests.cpp)
    target_link_libraries(unit_zoom_data_tests PRIVATE halo2_systems)
    add_test(NAME original_unit_zoom_import COMMAND unit_zoom_data_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/unit-zoom/measured-frozen/fixtures.tsv)
    set_tests_properties(original_unit_zoom_import PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_multiplayer_kbm_runtime COMMAND multiplayer_kbm_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_multiplayer_kbm_runtime PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 90)
    add_executable(frontend_text_style_tests tests/frontend_text_style_tests.cpp)
    add_executable(frontend_ui_assets_tests tests/frontend_ui_assets_tests.cpp)
    add_executable(mcc_menu_import_tests tests/mcc_menu_import_tests.cpp)
    target_link_libraries(mcc_menu_import_tests PRIVATE halo2_renderer)
    add_test(NAME mcc_classic_menu_import COMMAND mcc_menu_import_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/mcc-classic-menus
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/mcc-classic-hud)
    set_tests_properties(mcc_classic_menu_import PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(frontend_ui_assets_tests PRIVATE halo2_cache)
    add_test(NAME frontend_ui_assets COMMAND frontend_ui_assets_tests)
    add_test(NAME retail_frontend_ui_assets COMMAND frontend_ui_assets_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/vista-menu-ui-20261009)
    set_tests_properties(retail_frontend_ui_assets PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_ui_assets_painter_tests tests/frontend_ui_assets_painter_tests.cpp)
    add_executable(frontend_error_dialog_tests tests/frontend_error_dialog_tests.cpp)
    add_executable(frontend_mouse_prompt_tests tests/frontend_mouse_prompt_tests.cpp)
    target_link_libraries(frontend_mouse_prompt_tests PRIVATE halo2_systems)
    add_test(NAME retail_frontend_mouse_prompts COMMAND frontend_mouse_prompt_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map")
    set_tests_properties(retail_frontend_mouse_prompts PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(frontend_error_dialog_tests PRIVATE halo2_systems)
    add_test(NAME retail_frontend_error_dialog COMMAND frontend_error_dialog_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_frontend_error_dialog PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_menu_layout_tests tests/frontend_menu_layout_tests.cpp)
    target_link_libraries(frontend_menu_layout_tests PRIVATE halo2_systems)
    add_test(NAME retail_frontend_menu_layout COMMAND frontend_menu_layout_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map")
    set_tests_properties(retail_frontend_menu_layout PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_frontend_menu_layout_vista_art COMMAND frontend_menu_layout_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map"
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/menu-ui/vista)
    set_tests_properties(retail_frontend_menu_layout_vista_art PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(frontend_ui_assets_painter_tests PRIVATE halo2_systems)
    add_test(NAME retail_frontend_ui_assets_painter COMMAND frontend_ui_assets_painter_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/vista-menu-ui-20261009)
    set_tests_properties(retail_frontend_ui_assets_painter PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_font_sampling_painter_tests tests/frontend_font_sampling_painter_tests.cpp)
    target_link_libraries(frontend_font_sampling_painter_tests PRIVATE halo2_systems)
    add_test(NAME retail_frontend_font_sampling_painter COMMAND frontend_font_sampling_painter_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/menu-ui/vista/fonts)
    set_tests_properties(retail_frontend_font_sampling_painter PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(renderer_upload_batch_tests tests/renderer_upload_batch_tests.cpp)
    add_test(NAME renderer_upload_batch_contract COMMAND renderer_upload_batch_tests)
    add_executable(frontend_font_sampling_tests tests/frontend_font_sampling_tests.cpp)
    add_executable(vista_prompt_sampling_tests tests/vista_prompt_sampling_tests.cpp)
    target_link_libraries(vista_prompt_sampling_tests PRIVATE halo2_cache)
    add_test(NAME vista_prompt_sampling COMMAND vista_prompt_sampling_tests
        auto
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/mcc-classic-menus)
    set_tests_properties(vista_prompt_sampling PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(frontend_font_sampling_tests PRIVATE halo2_cache)
    add_test(NAME frontend_font_sampling COMMAND frontend_font_sampling_tests)
    add_executable(mcc_authored_prompt_sampling_tests tests/mcc_authored_prompt_sampling_tests.cpp)
    target_link_libraries(mcc_authored_prompt_sampling_tests PRIVATE halo2_cache)
    add_test(NAME mcc_authored_prompt_sampling COMMAND mcc_authored_prompt_sampling_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/imported
        ${CMAKE_CURRENT_SOURCE_DIR}/resources/presentation/prompts)
    set_tests_properties(mcc_authored_prompt_sampling PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_frontend_font_sampling COMMAND frontend_font_sampling_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/fonts
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/vista-asset-inventory-20261009/extracted/maps/fonts)
    set_tests_properties(retail_frontend_font_sampling PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_extension_catalog_tests tests/frontend_extension_catalog_tests.cpp)
    add_executable(frontend_vista_settings_tests tests/frontend_vista_settings_tests.cpp)
    add_executable(gameplay_fov_tests tests/gameplay_fov_tests.cpp)
    target_link_libraries(gameplay_fov_tests PRIVATE halo2_cache)
    add_test(NAME mcc_gameplay_fov COMMAND gameplay_fov_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mcc_gameplay_fov.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mcc_first_person_fov.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_split_first_person_fov.tsv)
    target_link_libraries(frontend_vista_settings_tests PRIVATE halo2_renderer)
    add_dependencies(frontend_vista_settings_tests halo2_shaders)
    add_test(NAME retail_frontend_vista_settings COMMAND frontend_vista_settings_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map"
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/vista-settings-20261010/source/source.tsv
        ${CMAKE_CURRENT_BINARY_DIR}/vista-settings-test.cfg)
    set_tests_properties(retail_frontend_vista_settings PROPERTIES SKIP_RETURN_CODE 77)
    set(HALO2_MCC_FRONTEND_TEST_CACHE "auto" CACHE PATH
        "Imported MCC cache root for frontend submenu regression (auto uses assets/mcc-source.tsv)")
    add_executable(frontend_mcc_submenus_tests tests/frontend_mcc_submenus_tests.cpp)
    target_link_libraries(frontend_mcc_submenus_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME mcc_frontend_submenus COMMAND frontend_mcc_submenus_tests
        "${HALO2_MCC_FRONTEND_TEST_CACHE}" ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}/mcc-submenus.json)
    set_tests_properties(mcc_frontend_submenus PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(frontend_mcc_match_handoff_tests tests/frontend_mcc_match_handoff_tests.cpp)
    target_link_libraries(frontend_mcc_match_handoff_tests PRIVATE halo2_systems)
    target_include_directories(frontend_mcc_match_handoff_tests PRIVATE src)
    add_test(NAME mcc_frontend_match_handoff COMMAND frontend_mcc_match_handoff_tests
        "${HALO2_MCC_FRONTEND_TEST_CACHE}" ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(mcc_frontend_match_handoff PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(multiplayer_asset_loader_tests tests/multiplayer_asset_loader_tests.cpp)
    target_link_libraries(multiplayer_asset_loader_tests PRIVATE halo2_systems)
    add_test(NAME multiplayer_asset_loader_contract COMMAND multiplayer_asset_loader_tests
        "${HALO2_MCC_GAMEPLAY_FONTS}/../h2_maps_win64_dx11")
    set_tests_properties(multiplayer_asset_loader_contract PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(frontend_vista_transition_assets_tests tests/frontend_vista_transition_assets_tests.cpp)
    target_link_libraries(frontend_vista_transition_assets_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json OpenSSL::Crypto)
    add_test(NAME retail_frontend_vista_transition_assets COMMAND frontend_vista_transition_assets_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map"
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_transition_assets.json)
    set_tests_properties(retail_frontend_vista_transition_assets PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_vista_presentation_tests tests/frontend_vista_presentation_tests.cpp)
    target_link_libraries(frontend_vista_presentation_tests PRIVATE halo2_systems)
    add_test(NAME retail_frontend_vista_presentation COMMAND frontend_vista_presentation_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map")
    set_tests_properties(retail_frontend_vista_presentation PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(frontend_extension_catalog_tests PRIVATE halo2_cache)
    add_test(NAME retail_frontend_extension_catalog COMMAND frontend_extension_catalog_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_frontend_extension_catalog PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(frontend_text_style_tests PRIVATE halo2_systems)
    add_test(NAME retail_frontend_text_style COMMAND frontend_text_style_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/profile-regression/frontend-text-style-original.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_frontend_text_style PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_text_setup_tests tests/frontend_text_setup_tests.cpp)
    target_link_libraries(frontend_text_setup_tests PRIVATE halo2_systems)
    add_test(NAME original_frontend_text_setup COMMAND frontend_text_setup_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/frontend_text_setup_original.tsv)
    set_tests_properties(original_frontend_text_setup PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_typewriter_tests tests/frontend_typewriter_tests.cpp)
    target_link_libraries(frontend_typewriter_tests PRIVATE halo2_cache)
    add_test(NAME original_frontend_typewriter COMMAND frontend_typewriter_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/frontend_typewriter_original.tsv)
    set_tests_properties(original_frontend_typewriter PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_typewriter_painter_tests tests/frontend_typewriter_painter_tests.cpp)
    target_link_libraries(frontend_typewriter_painter_tests PRIVATE halo2_systems)
    add_test(NAME retail_frontend_typewriter_painter COMMAND frontend_typewriter_painter_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_frontend_typewriter_painter PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_small_text_layout_tests tests/frontend_small_text_layout_tests.cpp)
    target_link_libraries(frontend_small_text_layout_tests PRIVATE halo2_systems)
    add_test(NAME original_frontend_small_text_layout COMMAND frontend_small_text_layout_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/frontend_small_text_layout_original.tsv)
    set_tests_properties(original_frontend_small_text_layout PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_widget_lifetime_tests tests/frontend_widget_lifetime_tests.cpp)
    target_link_libraries(frontend_widget_lifetime_tests PRIVATE halo2_systems)
    add_test(NAME retail_frontend_widget_lifetime COMMAND frontend_widget_lifetime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_frontend_widget_lifetime PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_signin_tests tests/frontend_signin_tests.cpp)
    add_executable(frontend_lobby_tests tests/frontend_lobby_tests.cpp)
    add_executable(frontend_team_tests tests/frontend_team_tests.cpp)
    target_link_libraries(frontend_team_tests PRIVATE halo2_systems)
    add_test(NAME original_frontend_team_picker COMMAND frontend_team_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/frontend_team_original.tsv)
    set_tests_properties(original_frontend_team_picker PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(local_profile_team_tests tests/local_profile_team_tests.cpp)
    target_link_libraries(local_profile_team_tests PRIVATE halo2_systems)
    add_test(NAME original_local_profile_team COMMAND local_profile_team_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/local_profile_team_original.tsv)
    add_executable(slayer_lobby_vehicle_tests tests/slayer_lobby_vehicle_tests.cpp)
    target_link_libraries(slayer_lobby_vehicle_tests PRIVATE halo2_systems)
    add_test(NAME retail_slayer_lobby_vehicle COMMAND slayer_lobby_vehicle_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_slayer_lobby_vehicle PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(frontend_lobby_tests PRIVATE halo2_systems)
    add_test(NAME retail_frontend_lobby COMMAND frontend_lobby_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_frontend_lobby PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_lobby_exit_tests tests/frontend_lobby_exit_tests.cpp)
    target_link_libraries(frontend_lobby_exit_tests PRIVATE halo2_systems)
    add_test(NAME original_frontend_lobby_exit COMMAND frontend_lobby_exit_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/frontend_lobby_exit_original.tsv)
    set_tests_properties(original_frontend_lobby_exit PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_lobby_policy_tests tests/frontend_lobby_policy_tests.cpp)
    add_executable(vista_lobby_countdown_tests tests/vista_lobby_countdown_tests.cpp)
    target_link_libraries(vista_lobby_countdown_tests PRIVATE halo2_systems)
    add_test(NAME original_vista_lobby_countdown COMMAND vista_lobby_countdown_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_countdown_original.tsv)
    add_executable(vista_session_countdown_tests tests/vista_session_countdown_tests.cpp)
    target_link_libraries(vista_session_countdown_tests PRIVATE halo2_systems)
    add_test(NAME original_vista_session_countdown COMMAND vista_session_countdown_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_session_countdown_original.tsv)
    add_executable(vista_session_commit_tests tests/vista_session_commit_tests.cpp)
    target_link_libraries(vista_session_commit_tests PRIVATE halo2_systems)
    add_test(NAME original_vista_session_commit COMMAND vista_session_commit_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_session_commit_original.tsv)
    add_executable(vista_announcer_queue_tests tests/vista_announcer_queue_tests.cpp)
    target_link_libraries(vista_announcer_queue_tests PRIVATE halo2_systems)
    add_test(NAME original_vista_announcer_queue COMMAND vista_announcer_queue_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_announcer_queue_original.tsv)
    add_executable(vista_audio_framing_tests tests/vista_audio_framing_tests.cpp)
    target_link_libraries(vista_audio_framing_tests PRIVATE halo2_systems)
    add_test(NAME original_vista_audio_framing COMMAND vista_audio_framing_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_audio_framing_original.tsv
        "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/shared.map")
    set_tests_properties(original_vista_audio_framing PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(vista_loading_tests tests/vista_loading_tests.cpp src/ui/vista/vista_loading.cpp)
    target_include_directories(vista_loading_tests PRIVATE include)
    add_test(NAME original_vista_loading COMMAND vista_loading_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_loading_v2_original.tsv)
    add_executable(vista_lifecycle_tests tests/vista_lifecycle_tests.cpp src/ui/vista/vista_lifecycle.cpp)
    target_include_directories(vista_lifecycle_tests PRIVATE include)
    add_test(NAME original_vista_lifecycle COMMAND vista_lifecycle_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lifecycle_original.tsv)
    add_executable(vista_session_readiness_tests tests/vista_session_readiness_tests.cpp src/ui/vista/vista_session_readiness.cpp)
    target_include_directories(vista_session_readiness_tests PRIVATE include)
    if(NOT MSVC)
        set_source_files_properties(src/ui/vista/vista_session_readiness.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
    endif()
    add_test(NAME original_vista_session_readiness COMMAND vista_session_readiness_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_session_readiness_original.tsv)
    add_executable(vista_frontend_readiness_tests tests/vista_frontend_readiness_tests.cpp src/ui/vista/vista_frontend_readiness.cpp)
    target_include_directories(vista_frontend_readiness_tests PRIVATE include)
    target_link_libraries(vista_frontend_readiness_tests PRIVATE nlohmann_json::nlohmann_json)
    if(NOT MSVC)
        set_source_files_properties(src/ui/vista/vista_frontend_readiness.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
    endif()
    add_test(NAME original_vista_frontend_readiness COMMAND vista_frontend_readiness_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_frontend_readiness_original.json)
    add_executable(vista_menu_return_observer_tests tests/vista_menu_return_observer_tests.cpp src/ui/vista/vista_menu_return_observer.cpp)
    target_include_directories(vista_menu_return_observer_tests PRIVATE include)
    add_test(NAME original_vista_menu_return_observer COMMAND vista_menu_return_observer_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_menu_return_observer_original.tsv)
    add_executable(vista_slayer_publication_tests tests/vista_slayer_publication_tests.cpp src/ui/vista/vista_slayer_publication.cpp)
    target_include_directories(vista_slayer_publication_tests PRIVATE include)
    add_test(NAME original_vista_slayer_publication COMMAND vista_slayer_publication_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_slayer_publication_original.tsv)
    add_executable(vista_session_birth_tests tests/vista_session_birth_tests.cpp src/ui/vista/vista_session_birth.cpp)
    target_include_directories(vista_session_birth_tests PRIVATE include)
    add_test(NAME original_vista_session_birth COMMAND vista_session_birth_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_session_birth_original.tsv)
    set_tests_properties(original_vista_menu_return_observer original_vista_slayer_publication original_vista_session_birth
        PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(frontend_lobby_policy_tests PRIVATE halo2_systems)
    add_test(NAME retail_frontend_lobby_policy COMMAND frontend_lobby_policy_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/frontend_lobby_header_original.tsv)
    add_executable(frontend_lobby_roster_tests tests/frontend_lobby_roster_tests.cpp)
    target_link_libraries(frontend_lobby_roster_tests PRIVATE halo2_cache)
    add_test(NAME retail_frontend_lobby_roster COMMAND frontend_lobby_roster_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/frontend_lobby_roster_original.tsv)
    add_executable(frontend_lobby_game_tests tests/frontend_lobby_game_tests.cpp)
    target_link_libraries(frontend_lobby_game_tests PRIVATE halo2_cache)
    add_test(NAME retail_frontend_lobby_game COMMAND frontend_lobby_game_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/frontend-lobby-game-original.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_frontend_lobby_game PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_lobby_setup_tests tests/frontend_lobby_setup_tests.cpp)
    target_link_libraries(frontend_lobby_setup_tests PRIVATE halo2_cache)
    add_test(NAME retail_frontend_lobby_setup COMMAND frontend_lobby_setup_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/frontend_lobby_setup_original.tsv)
    add_executable(frontend_lobby_maps_tests tests/frontend_lobby_maps_tests.cpp)
    target_link_libraries(frontend_lobby_maps_tests PRIVATE halo2_cache)
    add_test(NAME retail_frontend_lobby_maps COMMAND frontend_lobby_maps_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/frontend_lobby_maps_original.tsv)
    set_tests_properties(retail_frontend_lobby_maps PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_multiplayer_maps_tests tests/frontend_multiplayer_maps_tests.cpp)
    add_executable(frontend_custom_maps_tests tests/frontend_custom_maps_tests.cpp)
    target_link_libraries(frontend_custom_maps_tests PRIVATE halo2_cache)
    add_test(NAME frontend_custom_map_path_guards COMMAND frontend_custom_maps_tests)
    set(HALO2_FRONTEND_CUSTOM_MAP_CACHE "" CACHE PATH "Prepared custom menu cache for actual catalog binding checks")
    add_test(NAME frontend_custom_map_catalog_binding COMMAND frontend_custom_maps_tests "${HALO2_FRONTEND_CUSTOM_MAP_CACHE}")
    set_tests_properties(frontend_custom_map_catalog_binding PROPERTIES SKIP_RETURN_CODE 77)
    target_include_directories(frontend_multiplayer_maps_tests PRIVATE include)
    add_test(NAME frontend_multiplayer_catalog_selection COMMAND frontend_multiplayer_maps_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/frontend_lobby_maps_original.tsv)
    add_executable(frontend_lobby_ready_tests tests/frontend_lobby_ready_tests.cpp)
    target_link_libraries(frontend_lobby_ready_tests PRIVATE halo2_cache)
    add_test(NAME retail_frontend_lobby_ready COMMAND frontend_lobby_ready_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/frontend_lobby_ready_original.tsv)
    add_executable(frontend_lobby_warning_tests tests/frontend_lobby_warning_tests.cpp)
    target_link_libraries(frontend_lobby_warning_tests PRIVATE halo2_cache)
    add_test(NAME retail_frontend_lobby_warning COMMAND frontend_lobby_warning_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/frontend_lobby_warning_original.tsv)
    add_executable(menu_title_fog_state_tests tests/menu_title_fog_state_tests.cpp)
    target_link_libraries(menu_title_fog_state_tests PRIVATE halo2_cache)
    add_test(NAME retail_menu_title_fog_state COMMAND menu_title_fog_state_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/menu_title_fog_state_original.tsv)
    set_tests_properties(retail_menu_title_fog_state PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_appearance_tests tests/frontend_appearance_tests.cpp)
    target_link_libraries(frontend_appearance_tests PRIVATE halo2_systems)
    add_test(NAME retail_frontend_appearance COMMAND frontend_appearance_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/profile-regression/frontend-appearance-original.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_frontend_appearance PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_emblem_tests tests/frontend_emblem_tests.cpp)
    target_link_libraries(frontend_emblem_tests PRIVATE halo2_systems)
    add_test(NAME retail_frontend_emblem COMMAND frontend_emblem_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/customization/frontend-emblem-inputs-original.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/customization/frontend-emblem-live-program.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_frontend_emblem PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(player_ui_emblem_gpu_tests tests/player_ui_emblem_gpu_tests.cpp)
    target_link_libraries(player_ui_emblem_gpu_tests PRIVATE halo2_renderer)
    add_dependencies(player_ui_emblem_gpu_tests halo2_shaders)
    add_test(NAME retail_player_ui_emblem_gpu COMMAND player_ui_emblem_gpu_tests
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/player-ui-emblem-results)
    set_tests_properties(retail_player_ui_emblem_gpu PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 60)
    add_test(NAME retail_player_ui_emblem_gpu_mcc COMMAND player_ui_emblem_gpu_tests
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/player-ui-emblem-mcc-results --mcc)
    set_tests_properties(retail_player_ui_emblem_gpu_mcc PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 60)
    add_executable(player_armor_emblem_tests tests/player_armor_emblem_tests.cpp)
    add_executable(player_armor_emblem_gpu_tests tests/player_armor_emblem_gpu_tests.cpp)
    target_link_libraries(player_armor_emblem_gpu_tests PRIVATE halo2_renderer)
    add_dependencies(player_armor_emblem_gpu_tests halo2_shaders)
    target_link_libraries(player_armor_emblem_tests PRIVATE halo2_cache)
    add_test(NAME retail_player_armor_emblem COMMAND player_armor_emblem_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/player_armor_emblem_original.tsv)
    set(ARMOR_EMBLEM_CAPTURE_SHADERS)
    foreach(shader player_armor_emblem_capture.vert)
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader}.spv
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
            COMMAND ${GLSLANG_VALIDATOR} -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/${shader}
                -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader}.spv
            DEPENDS tests/${shader} shaders/combiner.glsl shaders/texture_program.glsl
                shaders/world_depth_fragment.glsl shaders/source_depth_storage.glsl VERBATIM)
        list(APPEND ARMOR_EMBLEM_CAPTURE_SHADERS ${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader}.spv)
    endforeach()
    add_custom_target(player_armor_emblem_capture_shaders DEPENDS ${ARMOR_EMBLEM_CAPTURE_SHADERS})
    add_executable(player_armor_emblem_framebuffer_gpu_tests tests/player_armor_emblem_framebuffer_gpu_tests.cpp)
    target_link_libraries(player_armor_emblem_framebuffer_gpu_tests PRIVATE Vulkan::Vulkan)
    add_dependencies(player_armor_emblem_framebuffer_gpu_tests player_armor_emblem_capture_shaders halo2_shaders)
    add_test(NAME retail_player_armor_emblem_framebuffer_gpu COMMAND player_armor_emblem_framebuffer_gpu_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/player_armor_emblem_framebuffer
        ${CMAKE_CURRENT_BINARY_DIR}/shaders ${CMAKE_CURRENT_BINARY_DIR}/armor-emblem-framebuffer-results)
    set_tests_properties(retail_player_armor_emblem_framebuffer_gpu PROPERTIES TIMEOUT 60 RUN_SERIAL TRUE)
    add_executable(frontend_menu_weapon_tests tests/frontend_menu_weapon_tests.cpp)
    target_link_libraries(frontend_menu_weapon_tests PRIVATE halo2_cache)
    add_test(NAME retail_frontend_menu_weapon_extern COMMAND frontend_menu_weapon_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map)
    set_tests_properties(retail_frontend_menu_weapon_extern PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(frontend_signin_tests PRIVATE halo2_systems)
    add_test(NAME retail_frontend_signin COMMAND frontend_signin_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/profile-regression/frontend-signin-original.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_frontend_signin PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_pose_capture_tests tests/frontend_pose_capture_tests.cpp)
    target_link_libraries(frontend_pose_capture_tests PRIVATE halo2_systems)
    add_executable(frontend_model_locals_tests tests/frontend_model_locals_tests.cpp)
    target_link_libraries(frontend_model_locals_tests PRIVATE halo2_cache)
    add_test(NAME retail_frontend_model_primary COMMAND frontend_model_locals_tests --primary
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/profile-regression/frontend-model-primary-original.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_frontend_model_primary PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_frontend_model_locals COMMAND frontend_model_locals_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/profile-regression/frontend-model-grip-original.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_frontend_model_locals PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME retail_frontend_model_initial_locals COMMAND frontend_model_locals_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/profile-regression/frontend-model-grip-deferred-original.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_frontend_model_initial_locals PROPERTIES SKIP_RETURN_CODE 77)
    foreach(species spartan elite)
        add_test(NAME retail_frontend_model_visible_${species}_grip COMMAND frontend_model_locals_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/profile-regression/frontend-model-visible-${species}-grip-original.tsv
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps --visible)
        set_tests_properties(retail_frontend_model_visible_${species}_grip PROPERTIES SKIP_RETURN_CODE 77)
    endforeach()
    add_executable(frontend_model_lighting_tests tests/frontend_model_lighting_tests.cpp)
    add_executable(frontend_model_shader_tests tests/frontend_model_shader_tests.cpp)
    target_link_libraries(frontend_model_shader_tests PRIVATE halo2_cache)
    add_test(NAME retail_frontend_model_shader COMMAND frontend_model_shader_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/customization/frontend-model-shader-original.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map)
    set_tests_properties(retail_frontend_model_shader PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_model_geometry_tests tests/frontend_model_geometry_tests.cpp)
    target_link_libraries(frontend_model_geometry_tests PRIVATE halo2_cache)
    add_test(NAME retail_frontend_model_geometry COMMAND frontend_model_geometry_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/customization/frontend-model-geometry-original.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map)
    set_tests_properties(retail_frontend_model_geometry PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(frontend_model_lighting_tests PRIVATE halo2_cache)
    add_test(NAME retail_frontend_model_lighting COMMAND frontend_model_lighting_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/customization/cinematic-lighting-original.tsv)
    add_executable(frontend_model_root_tests tests/frontend_model_root_tests.cpp)
    target_link_libraries(frontend_model_root_tests PRIVATE halo2_cache)
    add_test(NAME retail_frontend_model_roots COMMAND frontend_model_root_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/profile-regression/frontend-model-roots-original.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_frontend_model_roots PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_model_ik_tests tests/frontend_model_ik_tests.cpp)
    target_link_libraries(frontend_model_ik_tests PRIVATE halo2_cache)
    add_test(NAME retail_frontend_model_ik COMMAND frontend_model_ik_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/profile-regression
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_frontend_model_ik PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(frontend_model_tests tests/frontend_model_tests.cpp)
    target_link_libraries(frontend_model_tests PRIVATE halo2_systems OpenSSL::Crypto)
    add_test(NAME retail_frontend_models COMMAND frontend_model_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/profile-regression/model-camera-original.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/profile-regression/model-animation-original.tsv
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    set_tests_properties(retail_frontend_models PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(menu_music_tests tests/menu_music_tests.cpp)
    target_link_libraries(menu_music_tests PRIVATE halo2_systems)
    add_test(NAME retail_menu_music COMMAND menu_music_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/profile-regression/menu-music-original.tsv)
    set_tests_properties(retail_menu_music PROPERTIES SKIP_RETURN_CODE 77)
    target_link_libraries(frontend_tests PRIVATE halo2_systems)
    add_test(NAME frontend_bounds COMMAND frontend_tests)
    add_executable(frontend_sound_slots_tests tests/frontend_sound_slots_tests.cpp)
    target_link_libraries(frontend_sound_slots_tests PRIVATE halo2_systems)
    add_test(NAME original_frontend_sound_slots COMMAND frontend_sound_slots_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/halo2.exe")
    set_tests_properties(original_frontend_sound_slots PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(unit_locomotion_tests tests/unit_locomotion_tests.cpp)
    target_link_libraries(unit_locomotion_tests PRIVATE halo2_systems)
    add_test(NAME retail_unit_locomotion COMMAND unit_locomotion_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/unit-locomotion-fixtures/fixtures.tsv)
    set_tests_properties(retail_unit_locomotion PROPERTIES SKIP_RETURN_CODE 77)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_library(menu_window_probe SHARED tests/menu_window_probe.cpp)
        target_link_libraries(menu_window_probe PRIVATE SDL2::SDL2 Vulkan::Vulkan ${CMAKE_DL_LIBS})
        find_program(HALO2_XVFB_RUN xvfb-run)
        if(HALO2_XVFB_RUN)
            add_test(NAME player_armor_emblem_gpu COMMAND ${HALO2_XVFB_RUN} -a
                $<TARGET_FILE:player_armor_emblem_gpu_tests> ${CMAKE_CURRENT_SOURCE_DIR}
                ${CMAKE_CURRENT_BINARY_DIR}/armor-emblem-gpu)
            add_test(NAME player_armor_emblem_runtime COMMAND ${HALO2_XVFB_RUN} -a
                $<TARGET_FILE:player_armor_emblem_runtime_tests> ${CMAKE_CURRENT_SOURCE_DIR}
                ${CMAKE_CURRENT_BINARY_DIR}/armor-emblem-runtime)
            set_tests_properties(player_armor_emblem_gpu player_armor_emblem_runtime
                PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120 ENVIRONMENT "SDL_AUDIODRIVER=dummy")
        endif()
        if(HALO2_XVFB_RUN AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map")
            add_test(NAME native_menu_match_window COMMAND ${HALO2_XVFB_RUN} -a
                ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/test_menu_window_flow.py
                --runtime $<TARGET_FILE:halo2_native> --probe $<TARGET_FILE:menu_window_probe>
                --maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
                --output ${CMAKE_CURRENT_BINARY_DIR}/menu-window-flow)
            set_tests_properties(native_menu_match_window PROPERTIES TIMEOUT 200)
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/asset-sources.tsv")
                add_test(NAME native_solo_splitscreen_countdown COMMAND ${HALO2_XVFB_RUN} -a
                    ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/test_solo_splitscreen_flow.py
                    --launcher $<TARGET_FILE:halo2_title_launcher> --probe $<TARGET_FILE:menu_window_probe>
                    --output ${CMAKE_CURRENT_BINARY_DIR}/solo-splitscreen-flow --mute --retry-surface-formats)
                set_tests_properties(native_solo_splitscreen_countdown PROPERTIES TIMEOUT 200)
            endif()
        endif()
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map")
        add_executable(menu_camera_tests tests/menu_camera_tests.cpp)
        target_link_libraries(menu_camera_tests PRIVATE halo2_systems)
        add_test(NAME retail_menu_camera COMMAND menu_camera_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        add_executable(menu_camera_bank_tests tests/menu_camera_bank_tests.cpp)
        target_link_libraries(menu_camera_bank_tests PRIVATE halo2_systems)
        add_test(NAME retail_menu_camera_bank COMMAND menu_camera_bank_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/menu_camera_bank_original.tsv)
        add_executable(menu_world_depth_tests tests/menu_world_depth_tests.cpp)
        target_link_libraries(menu_world_depth_tests PRIVATE halo2_systems)
        add_test(NAME retail_menu_world_depth COMMAND menu_world_depth_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/menu_world_depth_original.tsv)
        add_executable(menu_world_source_geometry_tests tests/menu_world_source_geometry_tests.cpp)
        target_link_libraries(menu_world_source_geometry_tests PRIVATE halo2_cache)
        add_test(NAME retail_menu_world_source_geometry COMMAND menu_world_source_geometry_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-vertex-stage3)
        set_tests_properties(retail_menu_world_source_geometry PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(menu_world_source_binding_tests tests/menu_world_source_binding_tests.cpp)
        target_link_libraries(menu_world_source_binding_tests PRIVATE halo2_cache)
        add_test(NAME retail_menu_world_source_binding COMMAND menu_world_source_binding_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-vertex-stage3)
        set_tests_properties(retail_menu_world_source_binding PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(menu_world_source_pass_tests tests/menu_world_source_pass_tests.cpp)
        target_link_libraries(menu_world_source_pass_tests PRIVATE halo2_cache)
        add_test(NAME retail_menu_world_source_pass COMMAND menu_world_source_pass_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-source-pass-resolution-original.tsv
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-vertex-stage3)
        set_tests_properties(retail_menu_world_source_pass PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(menu_world_source_storage_tests tests/menu_world_source_storage_tests.cpp)
        target_link_libraries(menu_world_source_storage_tests PRIVATE halo2_menu_world_storage)
        add_test(NAME retail_menu_world_source_storage COMMAND menu_world_source_storage_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-vertex-stage3)
        set_tests_properties(retail_menu_world_source_storage PROPERTIES SKIP_RETURN_CODE 77)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json")
            set_tests_properties(retail_menu_world_source_storage PROPERTIES ENVIRONMENT
                "VK_LAYER_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d;LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib:$ENV{LD_LIBRARY_PATH}")
        endif()
        add_executable(menu_world_vertex_source_tests tests/menu_world_vertex_source_tests.cpp)
        target_link_libraries(menu_world_vertex_source_tests PRIVATE halo2_cache)
        add_test(NAME retail_menu_world_vertex_source COMMAND menu_world_vertex_source_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-vertex-straight-line-v2
            --extension ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-vertex-extension-fixtures)
        set_tests_properties(retail_menu_world_vertex_source PROPERTIES SKIP_RETURN_CODE 77)
        add_test(NAME original_effect_light_vertex_source COMMAND menu_world_vertex_source_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-vertex-straight-line-v2
            --sphere ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer-shading/world-effects-goal-20261011/light-raw-bindings-oracle-v2
            --sphere-map ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
        add_executable(menu_world_fragment_tests tests/menu_world_fragment_tests.cpp)
        target_link_libraries(menu_world_fragment_tests PRIVATE halo2_cache)
        add_test(NAME retail_menu_world_fragment COMMAND menu_world_fragment_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/menu_world_fragment_original.tsv
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit)
        set_tests_properties(retail_menu_world_fragment PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(menu_world_bitmap_import_tests tests/menu_world_bitmap_import_tests.cpp)
        target_link_libraries(menu_world_bitmap_import_tests PRIVATE halo2_cache)
        add_test(NAME retail_menu_world_bitmap_import COMMAND ${CMAKE_COMMAND}
            -DTEST_BINARY=$<TARGET_FILE:menu_world_bitmap_import_tests>
            -DMAP=${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
            -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-bitmap-import-fixtures/1335
            -DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/menu-world-bitmap-import-results
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_menu_world_bitmap_import.cmake)
        set_tests_properties(retail_menu_world_bitmap_import PROPERTIES
            SKIP_REGULAR_EXPRESSION "SKIP: bitmap import fixtures" TIMEOUT 120)
        add_executable(xemu_bc2_texture_tests tests/xemu_bc2_texture_tests.cpp)
        target_link_libraries(xemu_bc2_texture_tests PRIVATE halo2_cache)
        add_test(NAME retail_frontend_bc2_storage COMMAND xemu_bc2_texture_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-bc2-original-oracle
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map)
        set_tests_properties(retail_frontend_bc2_storage PROPERTIES SKIP_RETURN_CODE 77)
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_bitmap_decode.comp.spv
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
            COMMAND ${GLSLANG_VALIDATOR} -V --target-env vulkan1.3
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/menu_world_bitmap_decode.comp
                -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_bitmap_decode.comp.spv
            COMMAND ${SPIRV_VALIDATOR} --target-env vulkan1.3
                ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_bitmap_decode.comp.spv
            DEPENDS tests/menu_world_bitmap_decode.comp VERBATIM)
        add_custom_target(menu_world_bitmap_decode_shader
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_bitmap_decode.comp.spv)
        add_executable(menu_world_bitmap_decode_tests tests/menu_world_bitmap_decode_tests.cpp)
        target_link_libraries(menu_world_bitmap_decode_tests PRIVATE halo2_cache Vulkan::Vulkan)
        add_dependencies(menu_world_bitmap_decode_tests menu_world_bitmap_decode_shader)
        add_executable(xemu_bc2_texture_gpu_tests tests/xemu_bc2_texture_gpu_tests.cpp)
        target_link_libraries(xemu_bc2_texture_gpu_tests PRIVATE halo2_cache Vulkan::Vulkan)
        add_dependencies(xemu_bc2_texture_gpu_tests menu_world_bitmap_decode_shader)
        add_test(NAME retail_frontend_bc2_gpu_storage COMMAND ${CMAKE_COMMAND}
            -DTEST_BINARY=$<TARGET_FILE:xemu_bc2_texture_gpu_tests>
            -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-bc2-original-oracle
            -DSHADER=${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_bitmap_decode.comp.spv
            -DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/frontend-bc2-storage-results
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_xemu_bc2_texture_gpu.cmake)
        set_tests_properties(retail_frontend_bc2_gpu_storage PROPERTIES
            SKIP_REGULAR_EXPRESSION "SKIP: BC2" TIMEOUT 120 RUN_SERIAL TRUE)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json")
            set_tests_properties(retail_frontend_bc2_gpu_storage PROPERTIES ENVIRONMENT
                "VK_LAYER_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d;LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib:$ENV{LD_LIBRARY_PATH}")
        endif()
        add_test(NAME retail_menu_world_bitmap_decode COMMAND ${CMAKE_COMMAND}
            -DTEST_BINARY=$<TARGET_FILE:menu_world_bitmap_decode_tests>
            -DMAP=${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
            -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-bitmap-import-fixtures/1335
            -DSHADER=${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_bitmap_decode.comp.spv
            -DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/menu-world-bitmap-decode-results
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_menu_world_bitmap_decode.cmake)
        set_tests_properties(retail_menu_world_bitmap_decode PROPERTIES
            SKIP_REGULAR_EXPRESSION "SKIP: bitmap decode" TIMEOUT 120 RUN_SERIAL TRUE)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json")
            set_tests_properties(retail_menu_world_bitmap_decode PROPERTIES ENVIRONMENT
                "VK_LAYER_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d;LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib:$ENV{LD_LIBRARY_PATH}")
        endif()
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_constant_expansion.comp.spv
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
            COMMAND ${GLSLANG_VALIDATOR} -V --target-env vulkan1.3 -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/menu_world_constant_expansion.comp
                -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_constant_expansion.comp.spv
            COMMAND ${SPIRV_VALIDATOR} --target-env vulkan1.3
                ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_constant_expansion.comp.spv
            DEPENDS tests/menu_world_constant_expansion.comp shaders/combiner.glsl VERBATIM)
        add_custom_target(menu_world_constant_expansion_shader
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_constant_expansion.comp.spv)
        add_executable(menu_world_constant_expansion_tests tests/menu_world_constant_expansion_tests.cpp)
        target_include_directories(menu_world_constant_expansion_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_link_libraries(menu_world_constant_expansion_tests PRIVATE halo2_cache Vulkan::Vulkan)
        add_dependencies(menu_world_constant_expansion_tests menu_world_constant_expansion_shader)
        add_test(NAME retail_menu_world_constant_expansion COMMAND ${CMAKE_COMMAND}
            -DTEST_BINARY=$<TARGET_FILE:menu_world_constant_expansion_tests>
            -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-fragment-constant-expansion-float/manifest.tsv
            -DSHADER=${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_constant_expansion.comp.spv
            -DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/menu-world-constant-expansion-results
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_menu_world_constant_expansion.cmake)
        set_tests_properties(retail_menu_world_constant_expansion PROPERTIES
            SKIP_REGULAR_EXPRESSION "SKIP: constant fixtures" TIMEOUT 120 RUN_SERIAL TRUE)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json")
            set_tests_properties(retail_menu_world_constant_expansion PROPERTIES ENVIRONMENT
                "VK_LAYER_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d;LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib:$ENV{LD_LIBRARY_PATH}")
        endif()
        add_executable(menu_world_raster_tests tests/menu_world_raster_tests.cpp)
        add_executable(menu_world_fog_import_tests tests/menu_world_fog_import_tests.cpp)
        target_link_libraries(menu_world_fog_import_tests PRIVATE halo2_cache PkgConfig::AVCODEC)
        add_test(NAME retail_menu_world_fog_import COMMAND ${CMAKE_COMMAND}
            -DTEST_BINARY=$<TARGET_FILE:menu_world_fog_import_tests>
            -DMAP=${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
            -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/native-fragment-gate-original-images
            -DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/menu-world-fog-import-results
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_menu_world_fog_import.cmake)
        target_link_libraries(menu_world_raster_tests PRIVATE halo2_cache)
        add_test(NAME retail_menu_world_raster COMMAND menu_world_raster_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/menu_world_raster_original.tsv)
        add_executable(menu_world_entry_stencil_tests tests/menu_world_entry_stencil_tests.cpp)
        target_link_libraries(menu_world_entry_stencil_tests PRIVATE halo2_cache)
        add_test(NAME retail_menu_world_entry_stencil COMMAND menu_world_entry_stencil_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/menu_world_entry_stencil_original.tsv)
        if(TARGET menu_world_vertex_catalog)
            add_executable(menu_world_fragment_raster_tests tests/menu_world_fragment_raster_tests.cpp)
            target_link_libraries(menu_world_fragment_raster_tests PRIVATE halo2_menu_world_storage Vulkan::Vulkan PkgConfig::AVCODEC)
            target_include_directories(menu_world_fragment_raster_tests PRIVATE ${MENU_WORLD_VERTEX_CATALOG_DIR})
            add_dependencies(menu_world_fragment_raster_tests menu_world_vertex_catalog menu_world_geometry_shader menu_world_fragment_shader)
            add_test(NAME retail_menu_world_fragment_raster COMMAND ${CMAKE_COMMAND}
                -DTEST_BINARY=$<TARGET_FILE:menu_world_fragment_raster_tests>
                -DMAP=${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
                -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/native-fragment-raster-fixtures/1335
                -DGEOMETRY_SHADER=${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world.geom.spv
                -DFRAGMENT_SHADER=${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world.frag.spv
                -DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/menu-world-fragment-raster-results
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_menu_world_fragment_raster.cmake)
            set_tests_properties(retail_menu_world_fragment_raster PROPERTIES
                SKIP_REGULAR_EXPRESSION "SKIP: fragment raster fixtures" TIMEOUT 180 RUN_SERIAL TRUE)
            add_test(NAME retail_menu_world_fragment_raster_import COMMAND ${CMAKE_COMMAND}
                -DTEST_BINARY=$<TARGET_FILE:menu_world_fragment_raster_tests>
                -DMAP=${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
                -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/native-fragment-raster-fixtures/1335
                -DNATIVE_IMPORT_FIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-bitmap-import-fixtures/1335
                -DGEOMETRY_SHADER=${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world.geom.spv
                -DFRAGMENT_SHADER=${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world.frag.spv
                -DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/menu-world-fragment-raster-import-results
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_menu_world_fragment_raster.cmake)
            set_tests_properties(retail_menu_world_fragment_raster_import PROPERTIES
                SKIP_REGULAR_EXPRESSION "SKIP: fragment raster fixtures" TIMEOUT 180 RUN_SERIAL TRUE)
            add_test(NAME retail_menu_world_fragment_raster_carrier COMMAND ${CMAKE_COMMAND}
                -DTEST_BINARY=$<TARGET_FILE:menu_world_fragment_raster_tests>
                -DMAP=${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
                -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/native-fragment-raster-fixtures/1448
                -DGEOMETRY_SHADER=${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world.geom.spv
                -DFRAGMENT_SHADER=${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world.frag.spv
                -DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/menu-world-fragment-raster-carrier-results
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_menu_world_fragment_raster.cmake)
            set_tests_properties(retail_menu_world_fragment_raster_carrier PROPERTIES
                SKIP_REGULAR_EXPRESSION "SKIP: fragment raster fixtures" TIMEOUT 180 RUN_SERIAL TRUE)
            add_test(NAME retail_menu_world_fragment_raster_fog COMMAND ${CMAKE_COMMAND}
                -DTEST_BINARY=$<TARGET_FILE:menu_world_fragment_raster_tests>
                -DMAP=${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
                -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/native-fragment-raster-fixtures/1880
                -DGEOMETRY_SHADER=${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world.geom.spv
                -DFRAGMENT_SHADER=${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world.frag.spv
                -DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/menu-world-fragment-raster-fog-results
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_menu_world_fragment_raster.cmake)
            set_tests_properties(retail_menu_world_fragment_raster_fog PROPERTIES
                SKIP_REGULAR_EXPRESSION "SKIP: fragment raster fixtures" TIMEOUT 180 RUN_SERIAL TRUE)
            add_test(NAME retail_menu_world_fragment_raster_fog_import COMMAND ${CMAKE_COMMAND}
                -DTEST_BINARY=$<TARGET_FILE:menu_world_fragment_raster_tests>
                -DMAP=${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
                -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/native-fragment-raster-fixtures/1880
                -DNATIVE_IMPORT_FIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/native-fragment-gate-original-images
                -DGEOMETRY_SHADER=${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world.geom.spv
                -DFRAGMENT_SHADER=${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world.frag.spv
                -DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/menu-world-fragment-raster-fog-import-results
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_menu_world_fragment_raster.cmake)
            set_tests_properties(retail_menu_world_fragment_raster_fog_import PROPERTIES
                SKIP_REGULAR_EXPRESSION "SKIP: fragment raster fixtures" TIMEOUT 180 RUN_SERIAL TRUE)
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json")
                set_tests_properties(retail_menu_world_fragment_raster retail_menu_world_fragment_raster_import retail_menu_world_fragment_raster_carrier retail_menu_world_fragment_raster_fog retail_menu_world_fragment_raster_fog_import PROPERTIES ENVIRONMENT
                    "VK_LAYER_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d;LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib:$ENV{LD_LIBRARY_PATH}")
            endif()
            add_executable(menu_world_vertex_catalog_tests tests/menu_world_vertex_catalog_tests.cpp)
            target_link_libraries(menu_world_vertex_catalog_tests PRIVATE halo2_cache)
            target_include_directories(menu_world_vertex_catalog_tests PRIVATE ${MENU_WORLD_VERTEX_CATALOG_DIR})
            add_dependencies(menu_world_vertex_catalog_tests menu_world_vertex_catalog)
            add_test(NAME retail_menu_world_vertex_catalog COMMAND menu_world_vertex_catalog_tests
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
                ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-vertex-independent)
            set_tests_properties(retail_menu_world_vertex_catalog PROPERTIES SKIP_RETURN_CODE 77)
            add_executable(hologram_vertex_catalog_tests tests/hologram_vertex_catalog_tests.cpp)
            target_link_libraries(hologram_vertex_catalog_tests PRIVATE halo2_cache)
            target_include_directories(hologram_vertex_catalog_tests PRIVATE ${MENU_WORLD_VERTEX_CATALOG_DIR})
            add_dependencies(hologram_vertex_catalog_tests menu_world_vertex_catalog)
            add_test(NAME original_hologram_vertex_catalog COMMAND hologram_vertex_catalog_tests
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
            set_tests_properties(original_hologram_vertex_catalog PROPERTIES SKIP_RETURN_CODE 77)
            add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_vertex_catalog_capture.geom.spv
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
                COMMAND ${GLSLANG_VALIDATOR} -V --target-env vulkan1.3 -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
                    ${CMAKE_CURRENT_SOURCE_DIR}/tests/menu_world_vertex_catalog_capture.geom
                    -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_vertex_catalog_capture.geom.spv
                COMMAND ${SPIRV_VALIDATOR} --target-env vulkan1.3
                    ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_vertex_catalog_capture.geom.spv
                DEPENDS tests/menu_world_vertex_catalog_capture.geom shaders/world_depth_geometry.glsl VERBATIM)
            add_custom_target(menu_world_vertex_catalog_capture_shader
                DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_vertex_catalog_capture.geom.spv)
            add_executable(menu_world_vertex_catalog_gpu_tests tests/menu_world_vertex_catalog_gpu_tests.cpp)
            target_link_libraries(menu_world_vertex_catalog_gpu_tests PRIVATE halo2_menu_world_storage halo2_systems Vulkan::Vulkan)
            target_include_directories(menu_world_vertex_catalog_gpu_tests PRIVATE ${MENU_WORLD_VERTEX_CATALOG_DIR})
            add_dependencies(menu_world_vertex_catalog_gpu_tests menu_world_vertex_catalog menu_world_vertex_catalog_capture_shader)
            add_executable(pstencil_projected_vertex_attributes_gpu_tests tests/pstencil/projected_vertex_attributes_gpu_tests.cpp)
            target_link_libraries(pstencil_projected_vertex_attributes_gpu_tests PRIVATE halo2_menu_world_storage halo2_systems Vulkan::Vulkan)
            target_include_directories(pstencil_projected_vertex_attributes_gpu_tests PRIVATE ${MENU_WORLD_VERTEX_CATALOG_DIR})
            add_dependencies(pstencil_projected_vertex_attributes_gpu_tests menu_world_vertex_catalog menu_world_vertex_catalog_capture_shader)
            add_test(NAME retail_menu_world_vertex_catalog_gpu COMMAND ${CMAKE_COMMAND}
                -DTEST_BINARY=$<TARGET_FILE:menu_world_vertex_catalog_gpu_tests>
                -DMAP=${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
                -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-vertex-independent
                -DATTACHMENTS=${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-depth-attachment-fixtures
                -DCAPTURE_SHADER=${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_vertex_catalog_capture.geom.spv
                -DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/menu-world-vertex-catalog-results
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_menu_world_vertex_catalog_gpu.cmake)
            set_tests_properties(retail_menu_world_vertex_catalog_gpu PROPERTIES
                SKIP_REGULAR_EXPRESSION "SKIP: catalog fixtures" TIMEOUT 180 RUN_SERIAL TRUE)
            add_test(NAME original_effect_light_vertex_gpu COMMAND ${CMAKE_COMMAND}
                -DTEST_BINARY=$<TARGET_FILE:menu_world_vertex_catalog_gpu_tests>
                -DMAP=${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
                -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer-shading/world-effects-goal-20261011/light-raw-bindings-oracle-v2
                -DREFERENCES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer-shading/world-effects-goal-20261011/light-vertex-reference-v2
                -DCAPTURE_SHADER=${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_vertex_catalog_capture.geom.spv
                -DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/effect-light-vertex-results
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_effect_light_vertex_gpu.cmake)
            set_tests_properties(original_effect_light_vertex_gpu PROPERTIES
                SKIP_REGULAR_EXPRESSION "SKIP: sphere shader" TIMEOUT 180 RUN_SERIAL TRUE)
            add_test(NAME original_tu15_skinned_light_vertex_gpu COMMAND ${CMAKE_COMMAND}
                -DTEST_BINARY=$<TARGET_FILE:menu_world_vertex_catalog_gpu_tests>
                -DMAP=${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
                -DREFERENCES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/shader-parity-20261013/skinned-light-reference-v2
                -DCAPTURE_SHADER=${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_vertex_catalog_capture.geom.spv
                -DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/skinned-light-vertex-results
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_skinned_light_vertex_gpu.cmake)
            set_tests_properties(original_tu15_skinned_light_vertex_gpu PROPERTIES
                SKIP_REGULAR_EXPRESSION "SKIP: local model shader" TIMEOUT 180 RUN_SERIAL TRUE)
            add_test(NAME original_tu15_campaign_skinned_light_vertex_gpu COMMAND ${CMAKE_COMMAND}
                -DTEST_BINARY=$<TARGET_FILE:menu_world_vertex_catalog_gpu_tests>
                -DMAP=${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/01b_spacestation.map
                -DREFERENCES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/shader-parity-20261013/skinned-light-reference-v2
                -DCAPTURE_SHADER=${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_world_vertex_catalog_capture.geom.spv
                -DEXPECTED_CLASSES=5
                -DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/campaign-skinned-light-vertex-results
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_skinned_light_vertex_gpu.cmake)
            set_tests_properties(original_tu15_campaign_skinned_light_vertex_gpu PROPERTIES
                SKIP_REGULAR_EXPRESSION "SKIP: local model shader" TIMEOUT 180 RUN_SERIAL TRUE)
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json")
                set_tests_properties(retail_menu_world_vertex_catalog_gpu original_effect_light_vertex_gpu original_tu15_skinned_light_vertex_gpu original_tu15_campaign_skinned_light_vertex_gpu PROPERTIES ENVIRONMENT
                    "VK_LAYER_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d;LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib:$ENV{LD_LIBRARY_PATH}")
            endif()
        endif()
        add_executable(menu_bsp_resident_tests tests/menu_bsp_resident_tests.cpp)
        target_link_libraries(menu_bsp_resident_tests PRIVATE halo2_cache)
        add_test(NAME retail_menu_bsp_resident COMMAND menu_bsp_resident_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/menu_bsp_resident_original.tsv)
        set_tests_properties(retail_menu_bsp_resident PROPERTIES SKIP_RETURN_CODE 77)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/xemu-capture/title-projection-original.bin")
            add_executable(menu_projection_tests tests/menu_projection_tests.cpp)
            target_link_libraries(menu_projection_tests PRIVATE halo2_systems)
            add_test(NAME retail_menu_projection COMMAND menu_projection_tests
                ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/xemu-capture/title-projection-original.bin)
            if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/xemu-capture/title-projection-display-original.bin")
                add_test(NAME retail_menu_projection_display COMMAND menu_projection_tests
                    ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/xemu-capture/title-projection-original.bin
                    ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/xemu-capture/title-projection-display-original.bin)
            endif()
        endif()
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/xemu-capture/title-carrier-fog-original.bin")
            add_executable(menu_carrier_fog_tests tests/menu_carrier_fog_tests.cpp)
            target_link_libraries(menu_carrier_fog_tests PRIVATE halo2_cache)
            add_test(NAME retail_menu_carrier_fog COMMAND menu_carrier_fog_tests
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
                ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/xemu-capture/title-carrier-fog-original.bin)
        endif()
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/xemu-capture/title-lighting-original.bin")
            add_executable(menu_lighting_tests tests/menu_lighting_tests.cpp)
            target_link_libraries(menu_lighting_tests PRIVATE halo2_cache)
            add_test(NAME retail_menu_lighting COMMAND menu_lighting_tests
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
                ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/xemu-capture/title-lighting-original.bin)
        endif()
        add_test(NAME retail_frontend COMMAND frontend_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_fog.comp.spv
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
            COMMAND ${GLSLANG_VALIDATOR} -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/menu_fog.comp
                -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_fog.comp.spv
            DEPENDS tests/menu_fog.comp shaders/vertex_program.glsl shaders/render_view.glsl shaders/combiner.glsl VERBATIM)
        add_custom_target(menu_fog_test_shader DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_fog.comp.spv)
        add_executable(menu_fog_gpu_tests tests/menu_fog_gpu_tests.cpp)
        target_link_libraries(menu_fog_gpu_tests PRIVATE halo2_cache Vulkan::Vulkan)
        target_compile_definitions(menu_fog_gpu_tests PRIVATE
            HALO2_MENU_FOG_TEST_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_fog.comp.spv")
        add_dependencies(menu_fog_gpu_tests menu_fog_test_shader)
        add_test(NAME retail_menu_fog_gpu COMMAND menu_fog_gpu_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/fog_composite.comp.spv
            COMMAND ${GLSLANG_VALIDATOR} -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/fog_composite.comp
                -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/fog_composite.comp.spv
            DEPENDS tests/fog_composite.comp shaders/combiner.glsl shaders/fog_color.glsl VERBATIM)
        add_custom_target(fog_composite_test_shader DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/fog_composite.comp.spv)
        add_executable(fog_composite_gpu_tests tests/fog_composite_gpu_tests.cpp)
        target_link_libraries(fog_composite_gpu_tests PRIVATE halo2_cache Vulkan::Vulkan)
        target_compile_definitions(fog_composite_gpu_tests PRIVATE
            HALO2_FOG_COMPOSITE_TEST_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/fog_composite.comp.spv")
        add_dependencies(fog_composite_gpu_tests fog_composite_test_shader)
        add_test(NAME retail_fog_composite_gpu COMMAND fog_composite_gpu_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_depth_alias.comp.spv
            COMMAND ${GLSLANG_VALIDATOR} -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/menu_depth_alias.comp
                -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_depth_alias.comp.spv
            DEPENDS tests/menu_depth_alias.comp shaders/particle_depth.glsl shaders/fog_color.glsl VERBATIM)
        add_custom_target(menu_depth_alias_test_shader DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_depth_alias.comp.spv)
        add_executable(menu_depth_alias_gpu_tests tests/menu_depth_alias_gpu_tests.cpp)
        target_link_libraries(menu_depth_alias_gpu_tests PRIVATE halo2_cache Vulkan::Vulkan)
        target_compile_definitions(menu_depth_alias_gpu_tests PRIVATE
            HALO2_MENU_DEPTH_ALIAS_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_depth_alias.comp.spv")
        add_dependencies(menu_depth_alias_gpu_tests menu_depth_alias_test_shader)
        add_test(NAME retail_menu_depth_alias_gpu COMMAND menu_depth_alias_gpu_tests)
        set(HALO2_MENU_WORLD_DEPTH_ATTACHMENT_SHADERS
            menu_world_depth_attachment.vert
            menu_world_depth_attachment_fullscreen.vert
            menu_world_depth_attachment_seed.frag
            menu_world_depth_attachment_alias.frag
            menu_world_depth_attachment_helper.frag
            menu_world_depth_attachment_helper_precise.frag
            menu_world_depth_attachment_fog_visibility.frag)
        set(HALO2_MENU_WORLD_DEPTH_ATTACHMENT_SPV)
        foreach(shader IN LISTS HALO2_MENU_WORLD_DEPTH_ATTACHMENT_SHADERS)
            set(output "${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader}.spv")
            add_custom_command(OUTPUT "${output}"
                COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
                COMMAND ${GLSLANG_VALIDATOR} -V --target-env vulkan1.3
                    -I${CMAKE_CURRENT_SOURCE_DIR}/shaders -I${CMAKE_CURRENT_SOURCE_DIR}/tests
                    ${CMAKE_CURRENT_SOURCE_DIR}/tests/${shader} -o "${output}"
                DEPENDS tests/${shader} tests/menu_world_depth_attachment_helper_body.glsl
                    shaders/particle_depth.glsl shaders/world_depth_fragment.glsl shaders/source_depth_storage.glsl shaders/fog_color.glsl
                VERBATIM)
            list(APPEND HALO2_MENU_WORLD_DEPTH_ATTACHMENT_SPV "${output}")
        endforeach()
        add_custom_target(menu_world_depth_attachment_shaders
            DEPENDS ${HALO2_MENU_WORLD_DEPTH_ATTACHMENT_SPV}
                ${CMAKE_CURRENT_BINARY_DIR}/shaders/fog.vert.spv
                ${CMAKE_CURRENT_BINARY_DIR}/shaders/fog.frag.spv)
        add_executable(menu_world_depth_attachment_tests tests/menu_world_depth_attachment_tests.cpp)
        target_link_libraries(menu_world_depth_attachment_tests PRIVATE Vulkan::Vulkan)
        target_compile_definitions(menu_world_depth_attachment_tests PRIVATE
            HALO2_MENU_WORLD_DEPTH_ATTACHMENT_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
        add_dependencies(menu_world_depth_attachment_tests menu_world_depth_attachment_shaders)
        add_test(NAME retail_menu_world_depth_attachment COMMAND ${CMAKE_COMMAND}
            -DTEST_BINARY=$<TARGET_FILE:menu_world_depth_attachment_tests>
            -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-depth-attachment-fixtures
            -DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/world-depth-attachment-results
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_menu_world_depth_attachment.cmake)
        set_tests_properties(retail_menu_world_depth_attachment PROPERTIES
            SKIP_REGULAR_EXPRESSION "SKIP: attachment fixtures" TIMEOUT 180 RUN_SERIAL TRUE)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json")
            set_tests_properties(retail_menu_world_depth_attachment PROPERTIES ENVIRONMENT
                "VK_LAYER_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d;LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib:$ENV{LD_LIBRARY_PATH}")
        endif()
        add_test(NAME retail_menu_world_depth_full_fog COMMAND ${CMAKE_COMMAND}
            -DTEST_BINARY=$<TARGET_FILE:menu_world_depth_attachment_tests>
            -DFIXTURES=${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-depth-attachment-fixtures
            -DFULL_FOG=${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-depth-full-fog-fixtures
            -DOUTPUT_ROOT=${CMAKE_CURRENT_BINARY_DIR}/world-depth-full-fog-results
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_menu_world_depth_attachment.cmake)
        set_tests_properties(retail_menu_world_depth_full_fog PROPERTIES
            SKIP_REGULAR_EXPRESSION "SKIP: attachment fixtures" TIMEOUT 180 RUN_SERIAL TRUE)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json")
            set_tests_properties(retail_menu_world_depth_full_fog PROPERTIES ENVIRONMENT
                "VK_LAYER_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d;LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib:$ENV{LD_LIBRARY_PATH}")
        endif()
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/xemu-capture/title-eye-fog-pixels.bin")
            add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_eye_fog.comp.spv
                COMMAND ${GLSLANG_VALIDATOR} -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
                    ${CMAKE_CURRENT_SOURCE_DIR}/tests/menu_eye_fog.comp
                    -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_eye_fog.comp.spv
                DEPENDS tests/menu_eye_fog.comp shaders/combiner.glsl shaders/fog_color.glsl VERBATIM)
            add_custom_target(eye_fog_test_shader DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_eye_fog.comp.spv)
            add_executable(menu_eye_fog_gpu_tests tests/menu_eye_fog_gpu_tests.cpp)
            target_link_libraries(menu_eye_fog_gpu_tests PRIVATE halo2_cache Vulkan::Vulkan)
            target_compile_definitions(menu_eye_fog_gpu_tests PRIVATE HALO2_EYE_FOG_TEST_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_eye_fog.comp.spv")
            add_dependencies(menu_eye_fog_gpu_tests eye_fog_test_shader)
            add_test(NAME retail_menu_eye_fog_gpu COMMAND menu_eye_fog_gpu_tests
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
                ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/xemu-capture/title-eye-fog-pixels.bin)
        endif()
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/patchy_fog.comp.spv
            COMMAND ${GLSLANG_VALIDATOR} -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/patchy_fog.comp
                -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/patchy_fog.comp.spv
            DEPENDS tests/patchy_fog.comp shaders/combiner.glsl shaders/patchy_color.glsl VERBATIM)
        add_custom_target(patchy_test_shader DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/patchy_fog.comp.spv)
        add_executable(patchy_fog_gpu_tests tests/patchy_fog_gpu_tests.cpp)
        target_link_libraries(patchy_fog_gpu_tests PRIVATE halo2_cache Vulkan::Vulkan)
        target_compile_definitions(patchy_fog_gpu_tests PRIVATE HALO2_PATCHY_TEST_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/patchy_fog.comp.spv")
        add_dependencies(patchy_fog_gpu_tests patchy_test_shader)
        add_test(NAME retail_patchy_fog_gpu COMMAND patchy_fog_gpu_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        add_executable(bsp_patchy_depth_gpu_tests tests/bsp_patchy_depth_gpu_tests.cpp)
        target_link_libraries(bsp_patchy_depth_gpu_tests PRIVATE halo2_cache Vulkan::Vulkan)
        add_dependencies(bsp_patchy_depth_gpu_tests halo2_shaders)
        add_test(NAME tu15_bsp_patchy_depth_gpu COMMAND bsp_patchy_depth_gpu_tests ${CMAKE_CURRENT_BINARY_DIR}/shaders)
        add_executable(bsp_patchy_depth_tests tests/bsp_patchy_depth_tests.cpp)
        target_link_libraries(bsp_patchy_depth_tests PRIVATE halo2_cache)
        add_test(NAME tu15_bsp_patchy_depth COMMAND bsp_patchy_depth_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_bsp_patchy_depth.tsv)
        set_tests_properties(tu15_bsp_patchy_depth PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(patchy_fog_quad_tests tests/patchy_fog_quad_tests.cpp)
        target_link_libraries(patchy_fog_quad_tests PRIVATE halo2_cache)
        add_test(NAME original_patchy_fog_quad COMMAND patchy_fog_quad_tests --compare
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_haze_quad.inputs.bin
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_haze_quad.original.bin)
        set_tests_properties(original_patchy_fog_quad PROPERTIES SKIP_RETURN_CODE 77)
        add_test(NAME original_patchy_fog_direct_quad COMMAND patchy_fog_quad_tests --compare-direct
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_haze_direct_quad.inputs.bin
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/title_haze_direct_quad.original.bin)
        set_tests_properties(original_patchy_fog_direct_quad PROPERTIES SKIP_RETURN_CODE 77)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/xemu-capture/haze-fixture/manifest.json")
            add_executable(menu_haze_capture_gpu_tests tests/menu_haze_capture_gpu_tests.cpp)
            target_link_libraries(menu_haze_capture_gpu_tests PRIVATE halo2_cache Vulkan::Vulkan)
            target_compile_definitions(menu_haze_capture_gpu_tests PRIVATE HALO2_CAPTURE_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
            add_dependencies(menu_haze_capture_gpu_tests halo2_shaders)
            add_test(NAME retail_menu_haze_capture_gpu COMMAND menu_haze_capture_gpu_tests
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
                ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/xemu-capture/haze-fixture
                ${CMAKE_CURRENT_BINARY_DIR}/haze-capture-results)
        endif()
        add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_bloom.comp.spv
            COMMAND ${GLSLANG_VALIDATOR} -V -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/menu_bloom.comp
                -o ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_bloom.comp.spv
            DEPENDS tests/menu_bloom.comp shaders/combiner.glsl shaders/bloom_color.glsl VERBATIM)
        add_custom_target(bloom_test_shader DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_bloom.comp.spv)
        add_executable(menu_bloom_gpu_tests tests/menu_bloom_gpu_tests.cpp)
        target_link_libraries(menu_bloom_gpu_tests PRIVATE halo2_cache Vulkan::Vulkan)
        target_compile_definitions(menu_bloom_gpu_tests PRIVATE HALO2_BLOOM_TEST_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_bloom.comp.spv")
        add_dependencies(menu_bloom_gpu_tests bloom_test_shader)
        add_test(NAME retail_menu_bloom_gpu COMMAND menu_bloom_gpu_tests)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/profile-regression/popup-blur-fixture/3977-source0.rgba")
            add_executable(frontend_popup_blur_capture_gpu_tests tests/frontend_popup_blur_capture_gpu_tests.cpp)
            target_link_libraries(frontend_popup_blur_capture_gpu_tests PRIVATE halo2_cache Vulkan::Vulkan)
            add_dependencies(frontend_popup_blur_capture_gpu_tests halo2_shaders)
            add_test(NAME retail_frontend_popup_blur_capture_gpu COMMAND frontend_popup_blur_capture_gpu_tests
                ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/profile-regression/popup-blur-fixture
                ${CMAKE_CURRENT_BINARY_DIR}/shaders
                ${CMAKE_CURRENT_BINARY_DIR}/frontend-popup-blur-results)
        endif()
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/xemu-capture/bloom-fixture/manifest.json")
            add_executable(menu_bloom_capture_gpu_tests tests/menu_bloom_capture_gpu_tests.cpp)
            target_link_libraries(menu_bloom_capture_gpu_tests PRIVATE Vulkan::Vulkan halo2_cache)
            target_compile_definitions(menu_bloom_capture_gpu_tests PRIVATE HALO2_CAPTURE_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
            add_dependencies(menu_bloom_capture_gpu_tests halo2_shaders)
            add_test(NAME retail_menu_bloom_capture_gpu COMMAND menu_bloom_capture_gpu_tests
                ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/xemu-capture/bloom-fixture
                ${CMAKE_CURRENT_BINARY_DIR}/bloom-capture-results
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map)
        endif()
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/xemu-capture/title-filter-pixels.bin")
            add_executable(menu_scene_filter_gpu_tests tests/menu_scene_filter_gpu_tests.cpp)
            target_link_libraries(menu_scene_filter_gpu_tests PRIVATE halo2_cache Vulkan::Vulkan)
            target_compile_definitions(menu_scene_filter_gpu_tests PRIVATE HALO2_BLOOM_TEST_SHADER="${CMAKE_CURRENT_BINARY_DIR}/shaders/menu_bloom.comp.spv")
            add_dependencies(menu_scene_filter_gpu_tests bloom_test_shader)
            add_test(NAME retail_menu_scene_filter_gpu COMMAND menu_scene_filter_gpu_tests
                ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
                ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/xemu-capture/title-filter-pixels.bin)
        endif()
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/bink/intro_60.bik")
            add_test(NAME retail_movies COMMAND frontend_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps --movies)
        endif()
    endif()
    add_executable(systems_tests tests/systems_tests.cpp)
    add_executable(renderer_device_tests tests/renderer_device_tests.cpp)
    target_link_libraries(renderer_device_tests PRIVATE halo2_renderer)
    add_test(NAME native_renderer_device_contract COMMAND renderer_device_tests
        ${CMAKE_CURRENT_BINARY_DIR}/renderer-device-results)
    set_tests_properties(native_renderer_device_contract PROPERTIES RUN_SERIAL TRUE)
    add_executable(renderer_material_residency_tests tests/renderer_material_residency_tests.cpp)
    target_link_libraries(renderer_material_residency_tests PRIVATE halo2_renderer)
    add_test(NAME native_renderer_material_residency COMMAND renderer_material_residency_tests
        ${CMAKE_CURRENT_BINARY_DIR}/renderer-material-residency-results)
    set_tests_properties(native_renderer_material_residency PROPERTIES RUN_SERIAL TRUE)
    add_executable(frontend_post_raster_tests tests/frontend_post_raster_tests.cpp)
    target_include_directories(frontend_post_raster_tests PRIVATE include)
    add_test(NAME frontend_post_raster COMMAND frontend_post_raster_tests)
    add_executable(frontend_frame_sampling_tests tests/frontend_frame_sampling_tests.cpp)
    target_include_directories(frontend_frame_sampling_tests PRIVATE include)
    add_test(NAME frontend_frame_sampling COMMAND frontend_frame_sampling_tests)
    target_link_libraries(systems_tests PRIVATE halo2_systems)
    add_test(NAME native_systems COMMAND systems_tests)
    add_executable(campaign_tests tests/campaign_tests.cpp)
    add_test(NAME native_scripted_sound_location_kind COMMAND campaign_tests --scripted-sound-location)
    add_executable(campaign_cinematic_lighting_bridge_tu15_tests tests/campaign_cinematic_lighting_bridge_tu15_tests.cpp)
    target_link_libraries(campaign_cinematic_lighting_bridge_tu15_tests PRIVATE halo2_systems)
    add_executable(campaign_scripted_sound_point_tu15_tests tests/campaign_scripted_sound_point_tu15_tests.cpp)
    target_link_libraries(campaign_scripted_sound_point_tu15_tests PRIVATE halo2_systems)
    add_executable(campaign_scripted_audio_volume_tests tests/campaign_scripted_audio_volume_tests.cpp)
    target_link_libraries(campaign_scripted_audio_volume_tests PRIVATE halo2_systems)
    add_executable(campaign_scripted_audio_emitter_tests tests/campaign_scripted_audio_emitter_tests.cpp)
    target_link_libraries(campaign_scripted_audio_emitter_tests PRIVATE halo2_systems)
    add_executable(campaign_scripted_audio_timeline_tests tests/campaign_scripted_audio_timeline_tests.cpp)
    target_link_libraries(campaign_scripted_audio_timeline_tests PRIVATE halo2_systems)
    add_executable(script_sound_admission_tu15_tests tests/script_sound_admission_tu15_tests.cpp)
    target_link_libraries(script_sound_admission_tu15_tests PRIVATE halo2_systems)
    add_executable(campaign_scripted_audio_timing_tests tests/campaign_scripted_audio_timing_tests.cpp)
    target_link_libraries(campaign_scripted_audio_timing_tests PRIVATE halo2_systems)
    add_executable(campaign_scripted_audio_gain_tests tests/campaign_scripted_audio_gain_tests.cpp)
    target_link_libraries(campaign_scripted_audio_gain_tests PRIVATE halo2_systems)
    add_executable(campaign_camera_interpolation_tests tests/campaign_camera_interpolation_tests.cpp)
    target_link_libraries(campaign_camera_interpolation_tests PRIVATE halo2_systems)
    add_executable(campaign_screen_interpolation_tests tests/campaign_screen_interpolation_tests.cpp)
    target_link_libraries(campaign_screen_interpolation_tests PRIVATE halo2_systems)
    add_executable(campaign_camera_frame_tu15_tests tests/campaign_camera_frame_tu15_tests.cpp)
    target_link_libraries(campaign_camera_frame_tu15_tests PRIVATE halo2_systems)
    add_executable(campaign_independent_material_tests tests/campaign_independent_material_tests.cpp)
    target_link_libraries(campaign_independent_material_tests PRIVATE halo2_systems)
    add_executable(cinematic_animation_tests tests/cinematic_animation_tests.cpp)
    target_link_libraries(cinematic_animation_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_cinematic_animation_channels COMMAND cinematic_animation_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cinematic_animation_original.json)
    add_executable(cinematic_bloom_tests tests/cinematic_bloom_tests.cpp)
    target_link_libraries(cinematic_bloom_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_cinematic_bloom_tu15 COMMAND cinematic_bloom_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cinematic_bloom_tu15.json)
    add_test(NAME original_campaign_lighting_tu15 COMMAND frontend_model_lighting_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cinematic_lighting_tu15.tsv)
    add_executable(cinematic_object_function_tests tests/cinematic_object_function_tests.cpp)
    target_link_libraries(cinematic_object_function_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_cinematic_object_functions_tu15 COMMAND cinematic_object_function_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cinematic_object_functions_tu15.json)
    add_executable(campaign_frame_clock_tests tests/campaign_frame_clock_tests.cpp)
    target_link_libraries(campaign_frame_clock_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_executable(campaign_capture_boundary_tests tests/campaign_capture_boundary_tests.cpp)
    target_link_libraries(campaign_capture_boundary_tests PRIVATE halo2_systems)
    add_test(NAME native_campaign_capture_boundary COMMAND campaign_capture_boundary_tests)
    add_test(NAME original_campaign_frame_clock_tu15 COMMAND campaign_frame_clock_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_frame_clock_tu15.json)
    add_executable(campaign_object_attachment_tests tests/campaign_object_attachment_tests.cpp)
    target_link_libraries(campaign_object_attachment_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_campaign_object_attachment_tu15 COMMAND campaign_object_attachment_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_object_attachment_tu15.json)
    add_executable(cinematic_crossfade_tests tests/cinematic_crossfade_tests.cpp)
    target_link_libraries(cinematic_crossfade_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_cinematic_crossfade_tu15 COMMAND cinematic_crossfade_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cinematic_crossfade_tu15.json)
    add_executable(cinematic_crossfade_gpu_tests tests/cinematic_crossfade_gpu_tests.cpp)
    target_link_libraries(cinematic_crossfade_gpu_tests PRIVATE halo2_renderer)
    add_dependencies(cinematic_crossfade_gpu_tests halo2_shaders)
    add_executable(initialized_object_animation_tests tests/initialized_object_animation_tests.cpp)
    target_link_libraries(initialized_object_animation_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_initialized_object_animation_tu15 COMMAND initialized_object_animation_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/initialized_object_animation_tu15.json)
    add_executable(campaign_world_nodes_tests tests/campaign_world_nodes_tests.cpp)
    target_link_libraries(campaign_world_nodes_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_campaign_world_nodes_tu15 COMMAND campaign_world_nodes_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_world_nodes_tu15.json
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    add_executable(source_script_effect_tests tests/source_script_effect_tests.cpp)
    target_link_libraries(source_script_effect_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_script_marker_effect_tu15 COMMAND source_script_effect_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/script_marker_effect_tu15.json)
    add_executable(campaign_effect_graph_tests tests/campaign_effect_graph_tests.cpp)
    target_link_libraries(campaign_effect_graph_tests PRIVATE halo2_systems)
    add_executable(source_script_effect_kernel_tests tests/source_script_effect_kernel_tests.cpp)
    target_link_libraries(source_script_effect_kernel_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_executable(campaign_script_effect_runtime_tu15_tests tests/campaign_script_effect_runtime_tu15_tests.cpp)
    target_link_libraries(campaign_script_effect_runtime_tu15_tests PRIVATE halo2_systems)
    add_executable(source_campaign_unit_animation_time_tests tests/source_campaign_unit_animation_time_tests.cpp)
    target_link_libraries(source_campaign_unit_animation_time_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_campaign_unit_animation_time_tu15 COMMAND source_campaign_unit_animation_time_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_unit_animation_time_tu15.json)
    set_tests_properties(original_campaign_unit_animation_time_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(campaign_cinematic_source_runtime_tu15_tests tests/campaign_cinematic_source_runtime_tu15_tests.cpp)
    target_link_libraries(campaign_cinematic_source_runtime_tu15_tests PRIVATE halo2_systems)
    add_executable(source_campaign_camera_time_tests tests/source_campaign_camera_time_tests.cpp)
    target_link_libraries(source_campaign_camera_time_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_campaign_camera_time_tu15 COMMAND source_campaign_camera_time_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_camera_time_tu15.json)
    set_tests_properties(original_campaign_camera_time_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(campaign_cinematic_camera_runtime_tu15_tests tests/campaign_cinematic_camera_runtime_tu15_tests.cpp)
    target_link_libraries(campaign_cinematic_camera_runtime_tu15_tests PRIVATE halo2_systems)
    add_executable(campaign_point_camera_adoption_tu15_tests tests/campaign_point_camera_adoption_tu15_tests.cpp)
    target_link_libraries(campaign_point_camera_adoption_tu15_tests PRIVATE halo2_systems)
    add_executable(source_campaign_screen_bank_tests tests/source_campaign_screen_bank_tests.cpp)
    target_link_libraries(source_campaign_screen_bank_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_executable(campaign_cinematic_screen_runtime_tu15_tests tests/campaign_cinematic_screen_runtime_tu15_tests.cpp)
    target_link_libraries(campaign_cinematic_screen_runtime_tu15_tests PRIVATE halo2_systems)
    add_executable(campaign_cinematic_composition_runtime_tu15_tests tests/campaign_cinematic_composition_runtime_tu15_tests.cpp)
    target_link_libraries(campaign_cinematic_composition_runtime_tu15_tests PRIVATE halo2_systems)
    add_executable(source_object_function_slots_tests tests/source_object_function_slots_tests.cpp)
    target_link_libraries(source_object_function_slots_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_executable(campaign_cinematic_function_runtime_tu15_tests tests/campaign_cinematic_function_runtime_tu15_tests.cpp)
    target_link_libraries(campaign_cinematic_function_runtime_tu15_tests PRIVATE halo2_systems)
    add_executable(source_unit_emotion_tests tests/source_unit_emotion_tests.cpp)
    target_link_libraries(source_unit_emotion_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_executable(campaign_unit_cinematic_runtime_tu15_tests tests/campaign_unit_cinematic_runtime_tu15_tests.cpp)
    target_link_libraries(campaign_unit_cinematic_runtime_tu15_tests PRIVATE halo2_systems)
    add_executable(campaign_scripted_speech_tu15_tests tests/campaign_scripted_speech_tu15_tests.cpp)
    target_link_libraries(campaign_scripted_speech_tu15_tests PRIVATE halo2_systems)
    add_executable(campaign_session_identity_tu15_tests tests/campaign_session_identity_tu15_tests.cpp)
    target_link_libraries(campaign_session_identity_tu15_tests PRIVATE halo2_systems)
    add_executable(physics_scalar_store_tests tests/physics_scalar_store_tests.cpp)
    target_include_directories(physics_scalar_store_tests PRIVATE include src)
    add_test(NAME native_physics_scalar_store COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/tag_match_memory_budget.py
        --output-root @HALO2_TEST_SCRATCH@/guard --timeout 60 -- $<TARGET_FILE:physics_scalar_store_tests>)
    set_tests_properties(native_physics_scalar_store PROPERTIES TIMEOUT 90)
    add_executable(unit_speech_curves_tu15_tests tests/unit_speech_curves_tu15_tests.cpp)
    target_link_libraries(unit_speech_curves_tu15_tests PRIVATE halo2_physics_world_runtime)
    add_test(NAME original_unit_speech_curves_tu15 COMMAND unit_speech_curves_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_speech_curves_tu15.json)
    set_tests_properties(original_unit_speech_curves_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(campaign_completion_tu15_tests tests/campaign_completion_tu15_tests.cpp)
    target_link_libraries(campaign_completion_tu15_tests PRIVATE halo2_physics_world_runtime nlohmann_json::nlohmann_json)
    add_test(NAME original_campaign_completion_tu15 COMMAND campaign_completion_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_completion_tu15)
    set_tests_properties(original_campaign_completion_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_campaign_activation_tu15 COMMAND campaign_completion_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_activation_tu15)
    set_tests_properties(original_campaign_activation_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    # Standalone bounded cleanup owner; no World or Halo archive dependency.
    # The retained x87 caller/runtime backend currently requires GNU-style x86.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang" AND NOT MSVC AND
       CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64|i[3-6]86)$")
        add_executable(campaign_cleanup_tu15_tests tests/campaign_cleanup_tu15_tests.cpp
            src/campaign/source_campaign_activation_tu15_generated.cpp)
        target_include_directories(campaign_cleanup_tu15_tests PRIVATE include src)
        target_link_libraries(campaign_cleanup_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
        target_compile_options(campaign_cleanup_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
        add_test(NAME original_campaign_cleanup_tu15 COMMAND campaign_cleanup_tu15_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_cleanup_tu15)
        set_tests_properties(original_campaign_cleanup_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
        # Standalone original resource-stage owner; external callbacks are declared fixtures.
        add_executable(campaign_resource_stage_tu15_tests tests/campaign_resource_stage_tu15_tests.cpp
            src/campaign/source_campaign_activation_tu15_generated.cpp)
        target_include_directories(campaign_resource_stage_tu15_tests PRIVATE include src)
        target_link_libraries(campaign_resource_stage_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
        target_compile_options(campaign_resource_stage_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
        add_test(NAME original_campaign_resource_stage_tu15 COMMAND campaign_resource_stage_tu15_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_resource_stage_tu15)
        set_tests_properties(original_campaign_resource_stage_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
        # Complete row5 release owner; no external resource fixtures or World dependency.
        add_executable(campaign_resource_row5_tu15_tests tests/campaign_resource_row5_tu15_tests.cpp
            src/campaign/source_campaign_activation_tu15_generated.cpp)
        target_include_directories(campaign_resource_row5_tu15_tests PRIVATE include src)
        target_link_libraries(campaign_resource_row5_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
        target_compile_options(campaign_resource_row5_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
        add_test(NAME original_campaign_resource_row5_tu15 COMMAND campaign_resource_row5_tu15_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_resource_row5_tu15)
        set_tests_properties(original_campaign_resource_row5_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
        # Complete automatic placement scan; external predicates/factories are explicit fixtures.
        add_executable(campaign_scenario_scan_tu15_tests tests/campaign_scenario_scan_tu15_tests.cpp
            src/campaign/source_campaign_activation_tu15_generated.cpp)
        target_include_directories(campaign_scenario_scan_tu15_tests PRIVATE include src)
        target_link_libraries(campaign_scenario_scan_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
        target_compile_options(campaign_scenario_scan_tu15_tests PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)
        add_test(NAME original_campaign_scenario_scan_tu15 COMMAND campaign_scenario_scan_tu15_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_scenario_scan_tu15)
        set_tests_properties(original_campaign_scenario_scan_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    endif()
    add_executable(campaign_audio_tu15_tests tests/campaign_audio_tu15_tests.cpp)
    target_link_libraries(campaign_audio_tu15_tests PRIVATE halo2_physics_world_runtime
        halo2_input_startup_tu15 nlohmann_json::nlohmann_json OpenSSL::Crypto)
    add_test(NAME original_campaign_audio_tu15 COMMAND campaign_audio_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_audio_tu15)
    set_tests_properties(original_campaign_audio_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_campaign_audio_routing_tu15 COMMAND campaign_audio_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_audio_routing_tu15)
    set_tests_properties(original_campaign_audio_routing_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_campaign_sound_dsp_image_tu15 COMMAND campaign_audio_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_dsp_image_tu15)
    set_tests_properties(original_campaign_sound_dsp_image_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    # Standalone build of the same original image CPU module. This permits
    # its bounded allocation/upload fixtures without rebuilding the full world.
    add_executable(campaign_dsp_image_tu15_tests tests/campaign_audio_tu15_tests.cpp
        src/audio/dsp/source_sound_dsp_image_tu15_generated.cpp)
    target_compile_definitions(campaign_dsp_image_tu15_tests PRIVATE HALO2_DSP_IMAGE_ONLY_TEST=1)
    target_include_directories(campaign_dsp_image_tu15_tests PRIVATE include src)
    target_link_libraries(campaign_dsp_image_tu15_tests PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
    add_test(NAME original_sound_gp_upload_tu15 COMMAND campaign_dsp_image_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_gp_upload_tu15)
    set_tests_properties(original_sound_gp_upload_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_sound_gp_scratch_tu15 COMMAND campaign_dsp_image_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_gp_scratch_tu15)
    set_tests_properties(original_sound_gp_scratch_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_sound_gp_scratch_release_tu15 COMMAND campaign_dsp_image_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_gp_scratch_release_tu15)
    set_tests_properties(original_sound_gp_scratch_release_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_sound_gp_upload_callers_tu15 COMMAND campaign_dsp_image_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_gp_upload_callers_tu15)
    set_tests_properties(original_sound_gp_upload_callers_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_sound_gp_ep_append_bridge_tu15 COMMAND campaign_dsp_image_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_gp_ep_append_bridge_tu15)
    set_tests_properties(original_sound_gp_ep_append_bridge_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    set(HALO2_SOUND_REFERENCE_PYTHON "${CMAKE_CURRENT_SOURCE_DIR}/.tools/python/bin/python" CACHE FILEPATH
        "Python with Unicorn and Capstone for original sound CPU comparisons")
    add_test(NAME original_sound_ep_program_selector_tu15 COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_sound_ep_scratch_tu15.py
        --native $<TARGET_FILE:campaign_dsp_image_tu15_tests>
        --reference-python ${HALO2_SOUND_REFERENCE_PYTHON}
        --report ${CMAKE_CURRENT_BINARY_DIR}/sound-ep-original-report.json)
    set_tests_properties(original_sound_ep_program_selector_tu15 PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    add_executable(export_opening_facial_curves tools/export_opening_facial_curves.cpp)
    target_link_libraries(export_opening_facial_curves PRIVATE halo2_systems)
    add_executable(campaign_model_geometry_runtime_tu15_tests tests/campaign_model_geometry_runtime_tu15_tests.cpp)
    target_link_libraries(campaign_model_geometry_runtime_tu15_tests PRIVATE halo2_systems)
    add_executable(campaign_cloth_construction_tu15_tests tests/campaign_cloth_construction_tu15_tests.cpp)
    target_link_libraries(campaign_cloth_construction_tu15_tests PRIVATE halo2_systems)
    add_executable(campaign_scene_retirement_tu15_tests tests/campaign_scene_retirement_tu15_tests.cpp)
    target_link_libraries(campaign_scene_retirement_tu15_tests PRIVATE halo2_systems)
    add_executable(campaign_cinematic_environment_material_tests tests/campaign_cinematic_environment_material_tests.cpp)
    target_link_libraries(campaign_cinematic_environment_material_tests PRIVATE halo2_cache)
    add_executable(reach_model_float_positions_tests tests/reach_model_float_positions_tests.cpp)
    target_link_libraries(reach_model_float_positions_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_executable(campaign_cinematic_matte_material_tests tests/campaign_cinematic_matte_material_tests.cpp)
    target_link_libraries(campaign_cinematic_matte_material_tests PRIVATE halo2_cache)
    add_executable(campaign_lightmap_shadow_tu15_tests tests/campaign_lightmap_shadow_tu15_tests.cpp)
    target_link_libraries(campaign_lightmap_shadow_tu15_tests PRIVATE halo2_systems)
    add_executable(campaign_dof_recipe_tu15_tests tests/campaign_dof_recipe_tu15_tests.cpp)
    target_link_libraries(campaign_dof_recipe_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_executable(campaign_depth_of_field_tu15_tests tests/campaign_depth_of_field_tu15_tests.cpp)
    target_link_libraries(campaign_depth_of_field_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_executable(campaign_far_clip_tu15_tests tests/campaign_far_clip_tu15_tests.cpp)
    target_link_libraries(campaign_far_clip_tu15_tests PRIVATE halo2_systems)
    add_executable(source_cinematic_shadow_control_tu15_tests tests/source_cinematic_shadow_control_tu15_tests.cpp)
    target_link_libraries(source_cinematic_shadow_control_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    target_link_libraries(campaign_tests PRIVATE halo2_systems halo2_renderer)
    add_test(NAME campaign_checkpoints COMMAND campaign_tests)
    add_executable(cache_tests tests/cache_tests.cpp)
    target_link_libraries(cache_tests PRIVATE halo2_cache)
    add_test(NAME cache_bounds COMMAND cache_tests)
    add_executable(rendering_tests tests/rendering_tests.cpp)
    add_executable(shader_animation_tests tests/shader_animation_tests.cpp)
    target_link_libraries(shader_animation_tests PRIVATE halo2_cache)
    add_executable(light_volume_tests tests/light_volume_tests.cpp)
    target_link_libraries(light_volume_tests PRIVATE halo2_cache)
    target_link_libraries(rendering_tests PRIVATE halo2_cache)
    add_test(NAME rendering_programs COMMAND rendering_tests)
    add_executable(appearance_tests tests/appearance_tests.cpp)
    target_link_libraries(appearance_tests PRIVATE halo2_cache)
    add_executable(first_person_tests tests/first_person_tests.cpp)
    target_link_libraries(first_person_tests PRIVATE halo2_systems)
    add_test(NAME first_person_transforms COMMAND first_person_tests)
    add_executable(first_person_ik_tests tests/first_person_ik_tests.cpp)
    target_link_libraries(first_person_ik_tests PRIVATE halo2_cache)
    add_test(NAME first_person_marker_ik COMMAND first_person_ik_tests)
    add_executable(reference_player_tests tests/reference_player_tests.cpp)
    target_link_libraries(reference_player_tests PRIVATE halo2_cache)
    target_compile_definitions(reference_player_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    add_test(NAME original_player_reference COMMAND reference_player_tests)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/cyclotron.map")
        add_test(NAME retail_first_person_marker_ik COMMAND first_person_ik_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/cyclotron.map)
        add_test(NAME retail_reference_player_shader_bindings COMMAND reference_player_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/cyclotron.map)
    endif()
    add_test(NAME disc_parser COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_disc.py)
    add_test(NAME asset_source_setup COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_setup_assets.py)
    add_test(NAME vista_loading_preparation COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_vista_loading_preparation.py)
    add_test(NAME bundled_ui_assets COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_bundle_ui_assets.py)
    add_test(NAME vista_tag_conversion_boundaries COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_vista_tag_conversion.py)
    add_test(NAME mcc_gameplay_preparation COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_mcc_gameplay_preparation.py)
    add_custom_target(mcc_catalog_preparation_tests)
    add_test(NAME mcc_multiplayer_catalog_conversion COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_mcc_multiplayer_catalog.py)
    add_test(NAME mcc_menu_catalog_inventory COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_mcc_menu_catalog_inventory.py)
    add_test(NAME mcc_instant_charge_projection COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_mcc_instant_charge.py)
    add_test(NAME mcc_bitmap_correction_bundle COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_mcc_bitmap_correction_bundle.py)
    add_executable(mcc_cache_decode_tests tests/mcc_cache_decode_tests.cpp)
    target_link_libraries(mcc_cache_decode_tests PRIVATE halo2_cache ZLIB::ZLIB Threads::Threads)
    add_test(NAME mcc_cache_decode_bytes COMMAND mcc_cache_decode_tests)
    add_executable(mcc_cache_tests tests/mcc_cache_tests.cpp)
    target_link_libraries(mcc_cache_tests PRIVATE halo2_cache ZLIB::ZLIB)
    add_test(NAME native_mcc_cache COMMAND mcc_cache_tests)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_executable(native_vulkan_pipeline_cache_tests
            tests/native_vulkan_pipeline_cache_tests.cpp src/rendering/renderer/native_vulkan_pipeline_cache.cpp)
        target_include_directories(native_vulkan_pipeline_cache_tests PRIVATE ${Vulkan_INCLUDE_DIRS})
        target_link_libraries(native_vulkan_pipeline_cache_tests PRIVATE Threads::Threads)
        add_test(NAME native_vulkan_pipeline_cache COMMAND native_vulkan_pipeline_cache_tests)
        add_test(NAME native_vulkan_pipeline_cache_late_teardown COMMAND native_vulkan_pipeline_cache_tests --late-destroy)
        add_executable(mcc_texture_partition_cache_tests tests/mcc_texture_partition_cache_tests.cpp)
        target_link_libraries(mcc_texture_partition_cache_tests PRIVATE halo2_cache ZLIB::ZLIB Threads::Threads)
        target_link_options(mcc_texture_partition_cache_tests PRIVATE "-Wl,--wrap=inflate")
        add_test(NAME mcc_texture_partition_residency COMMAND mcc_texture_partition_cache_tests)
    endif()
    add_executable(mcc_resource_reader_tests tests/mcc_resource_reader_tests.cpp)
    target_link_libraries(mcc_resource_reader_tests PRIVATE halo2_cache ZLIB::ZLIB Threads::Threads)
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_options(mcc_resource_reader_tests PRIVATE
            "-Wl,--wrap=_ZN5halo216decode_mcc_cacheESt4spanIKSt4byteLm18446744073709551615EE"
            "-Wl,--wrap=pread" "-Wl,--wrap=fstat")
    endif()
    add_test(NAME mcc_resource_reader_contract COMMAND mcc_resource_reader_tests)
    set(HALO2_MCC_RESOURCE_CONTAINER "" CACHE FILEPATH "Optional owned MCC shared.map for resource range validation")
    add_test(NAME mcc_resource_reader_mcc_only COMMAND mcc_resource_reader_tests
        "${HALO2_MCC_RESOURCE_CONTAINER}")
    set_tests_properties(mcc_resource_reader_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(mcc_native_tex_bump_tests tests/mcc_native_tex_bump_tests.cpp)
    target_link_libraries(mcc_native_tex_bump_tests PRIVATE halo2_cache)
    add_executable(mcc_transparent_material_tests tests/mcc_transparent_material_tests.cpp)
    target_link_libraries(mcc_transparent_material_tests PRIVATE halo2_cache)
    add_executable(mcc_transparent_projection_tests tests/mcc_transparent_projection_tests.cpp)
    target_link_libraries(mcc_transparent_projection_tests PRIVATE halo2_cache)
    add_test(NAME mcc_transparent_projection_metadata COMMAND mcc_transparent_projection_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mcc_elongation_glass_tu15.json")
    add_executable(mcc_native_default_cube_tests tests/mcc_native_default_cube_tests.cpp)
    target_link_libraries(mcc_native_default_cube_tests PRIVATE halo2_cache)
    add_test(NAME mcc_native_default_cube_metadata COMMAND mcc_native_default_cube_tests)
    set(HALO2_MCC_NATIVE_DEFAULT_CUBE_MAPS "" CACHE PATH "Optional prepared MCC map directory for original vector-cube closure validation")
    add_test(NAME mcc_native_default_cube_cache COMMAND mcc_native_default_cube_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps" "${HALO2_MCC_NATIVE_DEFAULT_CUBE_MAPS}"
        "${CMAKE_CURRENT_BINARY_DIR}/mcc-native-default-cube-cache.json")
    set_tests_properties(mcc_native_default_cube_cache PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_test(NAME mcc_elongation_glass_correction COMMAND ${Python3_EXECUTABLE} -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_mcc_elongation_glass_correction.py)
    set_tests_properties(mcc_elongation_glass_correction PROPERTIES TIMEOUT 120)
    add_test(NAME mcc_earth_correction COMMAND ${Python3_EXECUTABLE} -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_mcc_earth_correction.py)
    set_tests_properties(mcc_earth_correction PROPERTIES TIMEOUT 120)
    set(HALO2_MCC_ELONGATION_GLASS_MAP "" CACHE FILEPATH "Optional prepared MCC Elongation for actual transparent BSP material validation")
    add_test(NAME mcc_elongation_transparent_material COMMAND mcc_transparent_material_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/elongation.map" "${HALO2_MCC_ELONGATION_GLASS_MAP}"
        "${CMAKE_CURRENT_BINARY_DIR}/mcc-elongation-glass-material.json")
    set_tests_properties(mcc_elongation_transparent_material PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(mcc_intro_luminous_tests tests/mcc_intro_luminous_tests.cpp)
    target_link_libraries(mcc_intro_luminous_tests PRIVATE halo2_cache)
    add_executable(mcc_intro_gauge_tests tests/mcc_intro_gauge_tests.cpp)
    target_link_libraries(mcc_intro_gauge_tests PRIVATE halo2_cache)
    set(HALO2_MCC_INTRO_LUMINOUS_MAP "" CACHE FILEPATH "Optional imported MCC introduction map for luminous material delivery")
    add_test(NAME mcc_intro_luminous_metadata COMMAND mcc_intro_luminous_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mcc_intro_luminous_tu15.json")
    add_test(NAME mcc_intro_luminous_cache COMMAND mcc_intro_luminous_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mcc_intro_luminous_tu15.json" "${HALO2_MCC_INTRO_LUMINOUS_MAP}")
    set_tests_properties(mcc_intro_luminous_cache PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_test(NAME mcc_intro_gauge_cache COMMAND mcc_intro_gauge_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mcc_intro_gauge_tu15.json" "${HALO2_MCC_INTRO_LUMINOUS_MAP}"
        "${CMAKE_CURRENT_BINARY_DIR}/mcc-intro-gauge-publication.json")
    set_tests_properties(mcc_intro_gauge_cache PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_test(NAME mcc_intro_current_cache COMMAND mcc_intro_gauge_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mcc_intro_current_tu15.json" "${HALO2_MCC_INTRO_LUMINOUS_MAP}")
    set_tests_properties(mcc_intro_current_cache PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    set(HALO2_MCC_TEX_BUMP_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional owned paired tex_bump conversion fixture")
    set(HALO2_MCC_TEX_BUMP_DONOR "" CACHE FILEPATH "Optional owned MCC map for native tex_bump material validation")
    add_test(NAME mcc_native_tex_bump_profiles COMMAND mcc_native_tex_bump_tests
        "${HALO2_MCC_TEX_BUMP_REFERENCE_FIXTURE}" "${HALO2_MCC_TEX_BUMP_DONOR}")
    set_tests_properties(mcc_native_tex_bump_profiles PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    set(HALO2_MCC_TEX_BUMP_ACTIVE_CAMO_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional owned paired tex_bump_active_camo conversion fixture")
    add_test(NAME mcc_native_tex_bump_active_camo_profiles COMMAND mcc_native_tex_bump_tests
        "${HALO2_MCC_TEX_BUMP_ACTIVE_CAMO_REFERENCE_FIXTURE}" "${HALO2_MCC_TEX_BUMP_DONOR}")
    set_tests_properties(mcc_native_tex_bump_active_camo_profiles PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    foreach(profile IN ITEMS DPRS_ENV ENV ENV_NO_DETAIL ILLUM_BLOOM)
        string(TOLOWER "${profile}" profile_name)
        set(HALO2_MCC_TEX_BUMP_${profile}_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional owned paired tex_bump_${profile_name} conversion fixture")
        add_test(NAME mcc_native_tex_bump_${profile_name}_profiles COMMAND mcc_native_tex_bump_tests
            "${HALO2_MCC_TEX_BUMP_${profile}_REFERENCE_FIXTURE}" "${HALO2_MCC_TEX_BUMP_DONOR}")
        set_tests_properties(mcc_native_tex_bump_${profile_name}_profiles PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    endforeach()
    set(HALO2_MCC_NATIVE_TEMPLATE_VARIANTS_FIXTURE "" CACHE FILEPATH "Optional owned native-template variant fixture")
    set(HALO2_MCC_NATIVE_TEMPLATE_VARIANTS_MAPS "" CACHE PATH "Optional owned MCC maps for native-template variant validation")
    add_test(NAME mcc_native_template_variants COMMAND mcc_native_tex_bump_tests
        "${HALO2_MCC_NATIVE_TEMPLATE_VARIANTS_FIXTURE}" "${HALO2_MCC_NATIVE_TEMPLATE_VARIANTS_MAPS}")
    set_tests_properties(mcc_native_template_variants PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME mcc_native_template_variants_generator COMMAND ${Python3_EXECUTABLE} -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p mcc_native_template_variants_generator_tests.py)
    add_executable(mcc_native_library_tests tests/mcc_native_library_tests.cpp)
    target_link_libraries(mcc_native_library_tests PRIVATE halo2_cache)
    add_test(NAME mcc_native_library_customization COMMAND mcc_native_library_tests)
    add_executable(mcc_native_library_snapshot_tests tests/mcc_native_library_snapshot_tests.cpp src/runtime/mcc_native_library.cpp)
    target_include_directories(mcc_native_library_snapshot_tests PRIVATE include)
    target_link_libraries(mcc_native_library_snapshot_tests PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
    add_test(NAME mcc_native_library_snapshots COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/mcc_native_library_snapshot_tests.py
        --test-executable $<TARGET_FILE:mcc_native_library_snapshot_tests> --repo-root ${CMAKE_CURRENT_SOURCE_DIR})
    if(CMAKE_EXPORT_COMPILE_COMMANDS AND UNIX AND NOT APPLE)
        add_test(NAME mcc_cache_owner_translation_units COMMAND ${Python3_EXECUTABLE} -B
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/mcc_cache_owner_compile.py
            --commands ${CMAKE_BINARY_DIR}/compile_commands.json --root ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
    set(HALO2_MCC_NATIVE_LIBRARY_DIRECTORY "" CACHE PATH "Optional owned retained native libraries for closure validation")
    add_test(NAME mcc_native_library_closure COMMAND mcc_native_library_tests "${HALO2_MCC_NATIVE_LIBRARY_DIRECTORY}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mcc_intro_gauge_tu15.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mcc_intro_current_tu15.json")
    set_tests_properties(mcc_native_library_closure PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_test(NAME mcc_native_library_closure_generator COMMAND ${Python3_EXECUTABLE} -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p mcc_native_library_closure_generator_tests.py)
    add_test(NAME mcc_native_library_onboarding COMMAND ${Python3_EXECUTABLE} -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_native_shader_closure_onboarding.py)
    add_test(NAME mcc_native_map_owners COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_mcc_native_map_owners.py)
    add_test(NAME mcc_native_map_owner_publication COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_mcc_native_map_owner_publication.py)
    add_test(NAME mcc_native_shader_owner_transaction COMMAND ${Python3_EXECUTABLE} -B -m unittest discover
        -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_mcc_native_shader_owner_transaction.py)
    add_executable(mcc_native_map_owner_tests tests/mcc_native_map_owner_tests.cpp src/assets/mcc/mcc_native_map_owner.cpp)
    target_include_directories(mcc_native_map_owner_tests PRIVATE include)
    target_link_libraries(mcc_native_map_owner_tests PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
    add_test(NAME native_mcc_map_owner_contract COMMAND mcc_native_map_owner_tests)
    add_test(NAME native_mcc_map_owner_producer_consumer COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mcc_native_map_owners.py --cpp-consumer $<TARGET_FILE:mcc_native_map_owner_tests>)
    add_executable(mcc_prepared_map_owner_tests tests/mcc_prepared_map_owner_tests.cpp
        src/assets/mcc_prepared_map_owner.cpp src/assets/mcc/mcc_native_map_owner.cpp)
    target_include_directories(mcc_prepared_map_owner_tests PRIVATE include src)
    target_link_libraries(mcc_prepared_map_owner_tests PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
    add_test(NAME mcc_prepared_map_owner_admission COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_mcc_prepared_map_owner_test.py $<TARGET_FILE:mcc_prepared_map_owner_tests>)
    add_executable(mcc_physics_tests tests/mcc_physics_tests.cpp)
    target_link_libraries(mcc_physics_tests PRIVATE halo2_cache)
    add_test(NAME native_mcc_physics COMMAND mcc_physics_tests)
    add_executable(mcc_runtime_delivery_tests tests/mcc_runtime_delivery_tests.cpp
        src/runtime/source_runtime_cache_tags.cpp src/runtime/source_runtime_memory.cpp)
    target_link_libraries(mcc_runtime_delivery_tests PRIVATE halo2_cache halo2_snapshot_storage)
    add_test(NAME mcc_runtime_delivery_contract COMMAND mcc_runtime_delivery_tests)
    add_executable(mcc_normalized_pointer_tests tests/mcc_normalized_pointer_tests.cpp)
    target_link_libraries(mcc_normalized_pointer_tests PRIVATE halo2_cache)
    add_test(NAME mcc_normalized_pointer_graphs COMMAND mcc_normalized_pointer_tests)
    add_executable(mcc_ordinary_pointer_tests tests/mcc_ordinary_pointer_tests.cpp)
    target_link_libraries(mcc_ordinary_pointer_tests PRIVATE halo2_cache)
    add_test(NAME mcc_ordinary_pointer_graphs COMMAND mcc_ordinary_pointer_tests)
    add_executable(mcc_sound_tag_layout_tests tests/mcc_sound_tag_layout_tests.cpp)
    target_link_libraries(mcc_sound_tag_layout_tests PRIVATE halo2_cache)
    add_test(NAME mcc_sound_tag_layouts COMMAND mcc_sound_tag_layout_tests)
    add_executable(mcc_sound_runtime_delivery_tests tests/mcc_sound_runtime_delivery_tests.cpp)
    target_link_libraries(mcc_sound_runtime_delivery_tests PRIVATE halo2_cache)
    add_test(NAME mcc_sound_runtime_delivery_contract COMMAND mcc_sound_runtime_delivery_tests)
    add_executable(mcc_sound_pipeline_coherence_tests tests/mcc_sound_pipeline_coherence_tests.cpp)
    target_link_libraries(mcc_sound_pipeline_coherence_tests PRIVATE halo2_cache OpenSSL::Crypto nlohmann_json::nlohmann_json)
    set(HALO2_MCC_SOUND_PIPELINE_MAP "" CACHE FILEPATH "Optional owned MCC Lockout, Cairo or Old Mombasa map for complete sound conversion validation")
    add_test(NAME mcc_sound_pipeline_coherence_mcc_only COMMAND mcc_sound_pipeline_coherence_tests
        "${HALO2_MCC_SOUND_PIPELINE_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/mcc-sound-pipeline-coherence.json")
    set_tests_properties(mcc_sound_pipeline_coherence_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 360)
    add_executable(mcc_sound_vanilla_reference_tests tests/mcc_sound_vanilla_reference_tests.cpp)
    target_link_libraries(mcc_sound_vanilla_reference_tests PRIVATE halo2_cache)
    add_test(NAME mcc_sound_vanilla_reference_contract COMMAND mcc_sound_vanilla_reference_tests)
    add_test(NAME mcc_sound_vanilla_reference_manifest COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_sound_vanilla_reference.py --check)
    add_executable(mcc_sound_timing_reference_tests tests/mcc_sound_timing_reference_tests.cpp)
    target_link_libraries(mcc_sound_timing_reference_tests PRIVATE halo2_cache)
    add_test(NAME mcc_sound_timing_reference_contract COMMAND mcc_sound_timing_reference_tests)
    add_executable(mcc_sound_timing_cache_tests tests/mcc_sound_timing_cache_tests.cpp)
    target_link_libraries(mcc_sound_timing_cache_tests PRIVATE halo2_cache)
    set(HALO2_MCC_SOUND_TIMING_MAP "" CACHE FILEPATH "Optional imported MCC Lockout map for complete timing and resource conversion checks")
    add_test(NAME mcc_sound_timing_cache_conversion COMMAND mcc_sound_timing_cache_tests "${HALO2_MCC_SOUND_TIMING_MAP}")
    set_tests_properties(mcc_sound_timing_cache_conversion PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
    add_test(NAME mcc_sound_timing_reference_manifest COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_sound_timing_reference.py --check)
    add_test(NAME mcc_sound_timing_reference_generator COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/mcc_sound_timing_reference_generator_tests.py)
    add_executable(mcc_sound_stored_time_reference_tests tests/mcc_sound_stored_time_reference_tests.cpp)
    target_link_libraries(mcc_sound_stored_time_reference_tests PRIVATE halo2_cache Threads::Threads)
    add_test(NAME mcc_sound_stored_time_reference_contract COMMAND mcc_sound_stored_time_reference_tests)
    add_test(NAME mcc_sound_stored_time_reference_manifest COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_sound_stored_time_reference.py --check)
    add_test(NAME mcc_sound_stored_time_reference_generator COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/mcc_sound_stored_time_generator_tests.py)
    add_executable(mcc_sound_permutation_time_reference_tests tests/mcc_sound_permutation_time_reference_tests.cpp)
    target_link_libraries(mcc_sound_permutation_time_reference_tests PRIVATE halo2_cache Threads::Threads)
    add_test(NAME mcc_sound_permutation_time_reference_contract COMMAND mcc_sound_permutation_time_reference_tests)
    add_test(NAME mcc_sound_permutation_time_reference_manifest COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_sound_permutation_time_reference.py --check)
    add_test(NAME mcc_sound_permutation_time_reference_generator COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/mcc_sound_permutation_time_generator_tests.py)
    add_executable(mcc_sound_class_reference_tests tests/mcc_sound_class_reference_tests.cpp)
    target_link_libraries(mcc_sound_class_reference_tests PRIVATE halo2_cache)
    add_test(NAME mcc_sound_class_reference_contract COMMAND mcc_sound_class_reference_tests)
    add_test(NAME mcc_sound_class_reference_intro COMMAND mcc_sound_class_reference_tests
        --mcc "${HALO2_MCC_INTRO_LUMINOUS_MAP}")
    set_tests_properties(mcc_sound_class_reference_intro PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_test(NAME mcc_sound_class_reference_manifest COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_sound_class_reference.py --check)
    add_test(NAME mcc_sound_class_reference_generator COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/generate_mcc_sound_class_reference_tests.py)
    set(HALO2_MCC_SOUND_DELIVERY_DONOR "" CACHE FILEPATH "Optional owned MCC map for native sound metadata validation")
    if(HALO2_MCC_SOUND_DELIVERY_DONOR)
        add_test(NAME mcc_sound_runtime_delivery_mcc_only COMMAND mcc_sound_runtime_delivery_tests
            "${HALO2_MCC_SOUND_DELIVERY_DONOR}")
        set_tests_properties(mcc_sound_runtime_delivery_mcc_only PROPERTIES TIMEOUT 360)
        add_test(NAME mcc_sound_vanilla_reference_mcc_only COMMAND mcc_sound_vanilla_reference_tests
            "${HALO2_MCC_SOUND_DELIVERY_DONOR}")
        set_tests_properties(mcc_sound_vanilla_reference_mcc_only PROPERTIES TIMEOUT 120)
    endif()
    add_executable(mcc_sound_native_readiness_tests tests/mcc_sound_native_readiness_tests.cpp)
    target_link_libraries(mcc_sound_native_readiness_tests PRIVATE halo2_physics_world_runtime)
    set(HALO2_MCC_SOUND_READINESS_FIXTURE "" CACHE FILEPATH "Optional original CPU sound readiness fixture")
    add_test(NAME mcc_sound_native_readiness COMMAND mcc_sound_native_readiness_tests
        "${HALO2_MCC_SOUND_READINESS_FIXTURE}")
    set_tests_properties(mcc_sound_native_readiness PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    add_executable(mcc_gameplay_scalar_tests tests/mcc_gameplay_scalar_tests.cpp)
    target_link_libraries(mcc_gameplay_scalar_tests PRIVATE halo2_cache)
    add_test(NAME mcc_gameplay_scalar_profiles COMMAND mcc_gameplay_scalar_tests)
    add_executable(destruction_lan_identity_tu15_tests tests/destruction_lan_identity_tu15_tests.cpp)
    target_link_libraries(destruction_lan_identity_tu15_tests PRIVATE halo2_systems)
    set(HALO2_DESTRUCTION_MCC_MAPS "" CACHE PATH "Prepared MCC maps with native banks for destruction admission regression")
    add_test(NAME destruction_lan_identity_tu15 COMMAND destruction_lan_identity_tu15_tests
        "${HALO2_DESTRUCTION_MCC_MAPS}/coagulation.map" "${HALO2_DESTRUCTION_MCC_MAPS}"
        "${CMAKE_CURRENT_BINARY_DIR}/destruction-lan-identity.json")
    set_tests_properties(destruction_lan_identity_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(mcc_destruction_effect_scalar_tests tests/mcc_destruction_effect_scalar_tests.cpp)
    add_executable(mcc_destruction_model_scalar_tests EXCLUDE_FROM_ALL
        tests/mcc_destruction_model_scalar_tests.cpp src/assets/mcc/mcc_destruction_model_scalar.cpp)
    target_include_directories(mcc_destruction_model_scalar_tests PRIVATE include)
    target_link_libraries(mcc_destruction_model_scalar_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME mcc_destruction_model_scalar_profiles COMMAND mcc_destruction_model_scalar_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/mcc-destruction-model-scalar-profile.json")
    add_executable(mcc_destruction_model_cache_tests EXCLUDE_FROM_ALL tests/mcc_destruction_model_scalar_tests.cpp)
    target_compile_definitions(mcc_destruction_model_cache_tests PRIVATE HALO2_DESTRUCTION_MODEL_CACHE_CHECK)
    target_link_libraries(mcc_destruction_model_cache_tests PRIVATE halo2_cache)
    add_executable(mcc_destruction_hunter_contract_tests EXCLUDE_FROM_ALL
        tests/mcc_destruction_hunter_contract_tests.cpp src/assets/mcc/mcc_destruction_model_contract.cpp)
    target_include_directories(mcc_destruction_hunter_contract_tests PRIVATE include)
    target_link_libraries(mcc_destruction_hunter_contract_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME mcc_destruction_hunter_contract COMMAND mcc_destruction_hunter_contract_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/mcc-destruction-hunter-contract-profile.json")
    add_executable(mcc_destruction_hunter_cache_tests EXCLUDE_FROM_ALL tests/mcc_destruction_hunter_contract_tests.cpp)
    target_compile_definitions(mcc_destruction_hunter_cache_tests PRIVATE HALO2_DESTRUCTION_HUNTER_CACHE_CHECK)
    target_link_libraries(mcc_destruction_hunter_cache_tests PRIVATE halo2_cache)
    add_executable(mcc_destruction_model_reference_tests EXCLUDE_FROM_ALL
        tests/mcc_destruction_model_reference_tests.cpp src/assets/mcc/mcc_destruction_model_reference.cpp)
    target_include_directories(mcc_destruction_model_reference_tests PRIVATE include)
    target_link_libraries(mcc_destruction_model_reference_tests PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME mcc_destruction_model_reference COMMAND mcc_destruction_model_reference_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/mcc-destruction-model-reference-profile.json")
    add_executable(mcc_destruction_model_reference_cache_tests EXCLUDE_FROM_ALL tests/mcc_destruction_model_reference_tests.cpp)
    target_compile_definitions(mcc_destruction_model_reference_cache_tests PRIVATE HALO2_DESTRUCTION_MODEL_REFERENCE_CACHE_CHECK)
    target_link_libraries(mcc_destruction_model_reference_cache_tests PRIVATE halo2_cache)
    if(EXISTS "${HALO2_DESTRUCTION_MCC_MAPS}/04b_floodlab.map")
        foreach(flood_model_map 04b_floodlab 07b_forerunnership)
            add_test(NAME destruction_model_reference_cache_${flood_model_map} COMMAND mcc_destruction_model_reference_cache_tests
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/mcc-destruction-model-reference-profile.json"
                "${HALO2_DESTRUCTION_MCC_MAPS}/${flood_model_map}.map" "${HALO2_DESTRUCTION_MCC_MAPS}")
            set_tests_properties(destruction_model_reference_cache_${flood_model_map} PROPERTIES TIMEOUT 180)
        endforeach()
    endif()
    if(EXISTS "${HALO2_DESTRUCTION_MCC_MAPS}/01b_spacestation.map")
        foreach(hunter_map 01b_spacestation 03a_oldmombasa 05b_deltatowers 07a_highcharity 08a_deltacliffs 08b_deltacontrol)
            add_test(NAME destruction_hunter_cache_${hunter_map} COMMAND mcc_destruction_hunter_cache_tests
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/mcc-destruction-hunter-contract-profile.json"
                "${HALO2_DESTRUCTION_MCC_MAPS}/${hunter_map}.map" "${HALO2_DESTRUCTION_MCC_MAPS}")
            set_tests_properties(destruction_hunter_cache_${hunter_map} PROPERTIES TIMEOUT 180)
        endforeach()
    endif()
    target_link_libraries(mcc_destruction_effect_scalar_tests PRIVATE halo2_cache)
    target_compile_definitions(mcc_destruction_effect_scalar_tests PRIVATE HALO2_DESTRUCTION_CACHE_CHECK)
    add_test(NAME mcc_destruction_effect_scalar_profiles COMMAND mcc_destruction_effect_scalar_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/mcc-destruction-effect-scalar-profiles.json)
    add_test(NAME mcc_turret_effect_scalar_profiles COMMAND mcc_destruction_effect_scalar_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/mcc-turret-effect-scalar-profiles.json)
    add_test(NAME mcc_gameplay_scalar_profile_manifest COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_gameplay_scalar_profiles.py --check)
    add_executable(mcc_gameplay_reference_tests tests/mcc_gameplay_reference_tests.cpp)
    target_link_libraries(mcc_gameplay_reference_tests PRIVATE halo2_cache)
    add_test(NAME mcc_gameplay_reference_profiles COMMAND mcc_gameplay_reference_tests)
    add_executable(mcc_map_gameplay_vanilla_tests
        tests/mcc_map_gameplay_vanilla_tests.cpp src/assets/mcc/mcc_map_gameplay_vanilla.cpp)
    target_include_directories(mcc_map_gameplay_vanilla_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(mcc_map_gameplay_vanilla_tests PRIVATE
        nlohmann_json::nlohmann_json OpenSSL::Crypto ZLIB::ZLIB)
    set(HALO2_MCC_MAP_GAMEPLAY_FIXTURE "" CACHE FILEPATH "Optional authenticated owned map gameplay fixture")
    add_test(NAME mcc_map_gameplay_vanilla_profiles COMMAND mcc_map_gameplay_vanilla_tests
        "${HALO2_MCC_MAP_GAMEPLAY_FIXTURE}")
    set_tests_properties(mcc_map_gameplay_vanilla_profiles PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
    add_executable(mcc_multiplayer_weapon_table_tests tests/mcc_multiplayer_weapon_table_tests.cpp)
    target_link_libraries(mcc_multiplayer_weapon_table_tests PRIVATE halo2_cache)
    add_test(NAME mcc_multiplayer_weapon_table COMMAND mcc_multiplayer_weapon_table_tests)
    set(HALO2_MCC_MULTIPLAYER_WEAPON_DONOR "" CACHE FILEPATH "Optional owned MCC multiplayer map for vanilla weapon table validation")
    add_test(NAME mcc_multiplayer_weapon_table_mcc_only COMMAND mcc_multiplayer_weapon_table_tests
        --mcc-only "${HALO2_MCC_MULTIPLAYER_WEAPON_DONOR}")
    set_tests_properties(mcc_multiplayer_weapon_table_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(mcc_bsp_instance_transform_tests tests/mcc_bsp_instance_transform_tests.cpp)
    target_link_libraries(mcc_bsp_instance_transform_tests PRIVATE halo2_cache)
    add_test(NAME mcc_bsp_instance_transform COMMAND mcc_bsp_instance_transform_tests)
    add_executable(mcc_hud_content_tests tests/mcc_hud_content_tests.cpp)
    add_executable(mcc_looping_sound_tests tests/mcc_looping_sound_tests.cpp)
    target_link_libraries(mcc_looping_sound_tests PRIVATE halo2_systems)
    set(HALO2_MCC_LOOPING_SOUND_DONOR "" CACHE FILEPATH "Optional owned MCC map for looping-sound metadata validation")
    add_test(NAME mcc_looping_sound_layout COMMAND mcc_looping_sound_tests "${HALO2_MCC_LOOPING_SOUND_DONOR}")
    add_test(NAME xbox_looping_sound_layout COMMAND mcc_looping_sound_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map)
    set_tests_properties(mcc_looping_sound_layout xbox_looping_sound_layout PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    set(HALO2_MCC_HUD_DONOR "" CACHE FILEPATH "Optional owned MCC map for complete HUD/resource validation")
    add_executable(mcc_hud_bitmap_geometry_tests tests/mcc_hud_bitmap_geometry_tests.cpp)
    target_link_libraries(mcc_hud_bitmap_geometry_tests PRIVATE halo2_cache)
    add_test(NAME mcc_hud_bitmap_logical_geometry COMMAND mcc_hud_bitmap_geometry_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/shared.map "${HALO2_MCC_HUD_DONOR}")
    set_tests_properties(mcc_hud_bitmap_logical_geometry PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME mcc_hud_bitmap_geometry_manifest COMMAND ${Python3_EXECUTABLE} -B
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_hud_bitmap_geometry.py --check)
    target_link_libraries(mcc_hud_content_tests PRIVATE halo2_cache OpenSSL::Crypto nlohmann_json::nlohmann_json)
    add_test(NAME mcc_hud_content_resources COMMAND mcc_hud_content_tests "${HALO2_MCC_HUD_DONOR}" --negative)
    set_tests_properties(mcc_hud_content_resources PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(mcc_hud_reference_tests tests/mcc_hud_reference_tests.cpp)
    target_link_libraries(mcc_hud_reference_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME mcc_hud_reference_contract COMMAND mcc_hud_reference_tests)
    add_test(NAME mcc_hud_reference_manifest COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_hud_reference.py --check)
    add_test(NAME mcc_hud_reference_generator COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/mcc_hud_reference_generator_tests.py)
    set(HALO2_XBOX_HUD_REFERENCE "" CACHE FILEPATH "Optional offline Xbox Lockout map for HUD reference validation")
    add_test(NAME mcc_hud_reference_paired COMMAND mcc_hud_reference_tests
        "${HALO2_MCC_HUD_DONOR}" "${HALO2_XBOX_HUD_REFERENCE}")
    set_tests_properties(mcc_hud_reference_paired PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME mcc_hud_reference_constructors COMMAND mcc_hud_reference_tests
        "${HALO2_MCC_HUD_DONOR}" "${HALO2_XBOX_HUD_REFERENCE}" --constructors)
    set_tests_properties(mcc_hud_reference_constructors PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME mcc_hud_reference_geometry COMMAND mcc_hud_reference_tests
        "${HALO2_MCC_HUD_DONOR}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/shared.map" --geometry)
    set_tests_properties(mcc_hud_reference_geometry PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(mcc_hud_shader_reference_tests tests/mcc_hud_shader_reference_tests.cpp)
    target_link_libraries(mcc_hud_shader_reference_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME mcc_hud_shader_reference_contract COMMAND mcc_hud_shader_reference_tests)
    add_test(NAME mcc_hud_shader_reference_manifest COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_hud_shader_reference.py --check)
    add_test(NAME mcc_hud_shader_reference_generator COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/mcc_hud_shader_reference_generator_tests.py)
    add_test(NAME mcc_hud_shader_reference_mcc_only COMMAND mcc_hud_shader_reference_tests
        "${HALO2_MCC_HUD_DONOR}")
    set_tests_properties(mcc_hud_shader_reference_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(mcc_native_hud_bootstrap_tests tests/mcc_native_hud_bootstrap_tests.cpp)
    target_link_libraries(mcc_native_hud_bootstrap_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME mcc_native_hud_bootstrap_contract COMMAND mcc_native_hud_bootstrap_tests)
    add_test(NAME mcc_native_hud_bootstrap_manifest COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_native_hud_bootstrap.py --check)
    add_test(NAME mcc_native_hud_bootstrap_generator COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/mcc_native_hud_bootstrap_generator_tests.py)
    add_test(NAME mcc_native_hud_bootstrap_mcc_only COMMAND mcc_native_hud_bootstrap_tests
        "${HALO2_MCC_HUD_DONOR}")
    set_tests_properties(mcc_native_hud_bootstrap_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(mcc_bsp_vertex_layout_tests tests/mcc_bsp_vertex_layout_tests.cpp)
    target_link_libraries(mcc_bsp_vertex_layout_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME mcc_bsp_vertex_layout_contract COMMAND mcc_bsp_vertex_layout_tests)
    set(HALO2_MCC_GEOMETRY_DONOR "" CACHE FILEPATH "Optional owned MCC map for raw BSP vertex conversion")
    set(HALO2_XBOX_GEOMETRY_REFERENCE "" CACHE FILEPATH "Optional offline original map for paired BSP vertex validation")
    add_test(NAME mcc_bsp_vertex_layout_mcc_only COMMAND mcc_bsp_vertex_layout_tests
        "${HALO2_MCC_GEOMETRY_DONOR}")
    set_tests_properties(mcc_bsp_vertex_layout_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME mcc_bsp_vertex_layout_paired COMMAND mcc_bsp_vertex_layout_tests
        "${HALO2_MCC_GEOMETRY_DONOR}" "${HALO2_XBOX_GEOMETRY_REFERENCE}")
    set_tests_properties(mcc_bsp_vertex_layout_paired PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(mcc_model_vertex_layout_tests tests/mcc_model_vertex_layout_tests.cpp)
    add_executable(mcc_model_node_map_tests tests/mcc_model_node_map_tests.cpp src/runtime/source_runtime_resources.cpp)
    target_link_libraries(mcc_model_node_map_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME mcc_model_node_map_contract COMMAND mcc_model_node_map_tests)
    set(HALO2_MCC_MODEL_NODE_MAP_CACHE "" CACHE FILEPATH "Owned MCC Cairo map for native node-map delivery validation")
    add_test(NAME mcc_model_node_map_actual COMMAND mcc_model_node_map_tests
        "${HALO2_MCC_MODEL_NODE_MAP_CACHE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mcc_gun_hinge_node_map_tu15.json"
        "${CMAKE_CURRENT_BINARY_DIR}/mcc-model-node-map.json")
    set_tests_properties(mcc_model_node_map_actual PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180
        ENVIRONMENT "XDG_CACHE_HOME=@HALO2_TEST_SCRATCH@/cache")
    target_link_libraries(mcc_model_vertex_layout_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME mcc_model_vertex_layout_contract COMMAND mcc_model_vertex_layout_tests)
    set(HALO2_MCC_MODEL_DONOR "" CACHE FILEPATH "Optional owned MCC Lockout map for model stream conversion")
    add_executable(mcc_bitmap_residency_tests tests/mcc_bitmap_residency_tests.cpp)
    target_link_libraries(mcc_bitmap_residency_tests PRIVATE halo2_cache)
    add_test(NAME mcc_bitmap_shared_residency COMMAND mcc_bitmap_residency_tests "${HALO2_MCC_MODEL_DONOR}")
    set_tests_properties(mcc_bitmap_shared_residency PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME mcc_model_vertex_layout_mcc_only COMMAND mcc_model_vertex_layout_tests
        "${HALO2_MCC_MODEL_DONOR}")
    set_tests_properties(mcc_model_vertex_layout_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(mcc_opening_model_vertex_layout_tests tests/mcc_opening_model_vertex_layout_tests.cpp)
    target_link_libraries(mcc_opening_model_vertex_layout_tests PRIVATE halo2_cache)
    set(HALO2_MCC_OPENING_MODEL_DONOR "" CACHE FILEPATH "Owned MCC introduction map for transparent rigid-boned model regression")
    add_test(NAME mcc_opening_transparent_rigid_model COMMAND mcc_opening_model_vertex_layout_tests
        "${HALO2_MCC_OPENING_MODEL_DONOR}")
    set_tests_properties(mcc_opening_transparent_rigid_model PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(mcc_skinned_model_vertex_layout_tests tests/mcc_skinned_model_vertex_layout_tests.cpp)
    target_link_libraries(mcc_skinned_model_vertex_layout_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME mcc_skinned_model_vertex_layout_contract COMMAND mcc_skinned_model_vertex_layout_tests)
    add_test(NAME mcc_skinned_model_vertex_layout_mcc_only COMMAND mcc_skinned_model_vertex_layout_tests
        "${HALO2_MCC_MODEL_DONOR}")
    set_tests_properties(mcc_skinned_model_vertex_layout_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(gameplay_font_source_tests tests/gameplay_font_source_tests.cpp)
    target_link_libraries(gameplay_font_source_tests PRIVATE halo2_cache)
    add_test(NAME gameplay_font_source_contract COMMAND gameplay_font_source_tests)
    add_test(NAME gameplay_font_reference_manifest COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_gameplay_font_reference.py --check)
    add_test(NAME gameplay_font_reference_generator COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/gameplay_font_reference_generator_tests.py)
    set(HALO2_MCC_GAMEPLAY_FONTS "" CACHE PATH "Optional owned MCC halo2/h2_fonts directory")
    set(HALO2_XBOX_GAMEPLAY_FONTS "" CACHE PATH "Optional offline Xbox font reference directory")
    add_test(NAME gameplay_font_source_paired COMMAND gameplay_font_source_tests
        "${HALO2_MCC_GAMEPLAY_FONTS}" "${HALO2_XBOX_GAMEPLAY_FONTS}")
    set_tests_properties(gameplay_font_source_paired PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(mcc_bsp_instance_bounds_tests tests/mcc_bsp_instance_bounds_tests.cpp)
    target_link_libraries(mcc_bsp_instance_bounds_tests PRIVATE halo2_cache)
    add_test(NAME mcc_bsp_instance_bounds COMMAND mcc_bsp_instance_bounds_tests)
    add_test(NAME mcc_bsp_instance_bounds_manifest COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_bsp_instance_bounds.py --check)
    set(HALO2_MCC_BSP_BOUNDS_MAP "" CACHE FILEPATH "Owned MCC Old Mombasa map for stored bounds validation")
    set(HALO2_XBOX_BSP_BOUNDS_MAP "" CACHE FILEPATH "Optional offline Xbox Old Mombasa reference for stored bounds validation")
    add_test(NAME mcc_bsp_instance_bounds_paired COMMAND mcc_bsp_instance_bounds_tests
        "${HALO2_MCC_BSP_BOUNDS_MAP}" "${HALO2_XBOX_BSP_BOUNDS_MAP}")
    set_tests_properties(mcc_bsp_instance_bounds_paired PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_test(NAME mcc_bsp_instance_bounds_mcc_only COMMAND mcc_bsp_instance_bounds_tests
        --mcc-only "${HALO2_MCC_BSP_BOUNDS_MAP}")
    set_tests_properties(mcc_bsp_instance_bounds_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(mcc_bsp_collision_reference_tests tests/mcc_bsp_collision_reference_tests.cpp)
    target_link_libraries(mcc_bsp_collision_reference_tests PRIVATE halo2_cache)
    add_test(NAME mcc_bsp_collision_reference COMMAND mcc_bsp_collision_reference_tests)
    set(HALO2_MCC_BSP_COLLISION_DONOR "" CACHE FILEPATH "Optional owned MCC Old Mombasa map for collision validation")
    set(HALO2_MCC_BSP_COLLISION_REFERENCE "" CACHE FILEPATH "Optional owned original Old Mombasa reference map")
    add_test(NAME mcc_bsp_collision_reference_paired COMMAND mcc_bsp_collision_reference_tests
        "${HALO2_MCC_BSP_COLLISION_DONOR}" "${HALO2_MCC_BSP_COLLISION_REFERENCE}")
    set_tests_properties(mcc_bsp_collision_reference_paired PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_test(NAME mcc_bsp_collision_reference_mcc_only COMMAND mcc_bsp_collision_reference_tests
        --mcc-only "${HALO2_MCC_BSP_COLLISION_DONOR}")
    set_tests_properties(mcc_bsp_collision_reference_mcc_only PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(mcc_gameplay_unit_scalar_tests tests/mcc_gameplay_unit_scalar_tests.cpp)
    target_link_libraries(mcc_gameplay_unit_scalar_tests PRIVATE halo2_cache)
    add_test(NAME mcc_gameplay_unit_scalar_profiles COMMAND mcc_gameplay_unit_scalar_tests)
    add_test(NAME mcc_gameplay_unit_scalar_manifest COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_gameplay_unit_scalar_profiles.py --check)
    add_test(NAME mcc_script_signature_profile COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_mcc_script_layout.py --check)
    add_executable(mcc_loading_inputs_tests tests/mcc_loading_inputs_tests.cpp)
    target_link_libraries(mcc_loading_inputs_tests PRIVATE halo2_cache)
    add_test(NAME mcc_loading_inputs COMMAND mcc_loading_inputs_tests)
    add_test(NAME mcc_loading_presentation_import COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mcc_loading_presentation.py)
    add_test(NAME macos_loading_converter_policy COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/macos/test_loading_converter.py)
    add_executable(mcc_loading_presentation_tests tests/mcc_loading_presentation_tests.cpp)
    target_link_libraries(mcc_loading_presentation_tests PRIVATE halo2_renderer)
    add_test(NAME mcc_loading_presentation_assets COMMAND mcc_loading_presentation_tests
        "${HALO2_MCC_FRONTEND_TEST_CACHE}" ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(mcc_loading_presentation_assets PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_executable(mcc_gameplay_content_tests tests/mcc_gameplay_content_tests.cpp)
    target_link_libraries(mcc_gameplay_content_tests PRIVATE halo2_systems)
    add_executable(mcc_campaign_text_tests tests/mcc_campaign_text_tests.cpp)
    target_link_libraries(mcc_campaign_text_tests PRIVATE halo2_systems halo2_renderer)
    add_test(NAME mcc_opening_campaign_text COMMAND mcc_campaign_text_tests "${HALO2_MCC_OPENING_MODEL_DONOR}")
    set_tests_properties(mcc_opening_campaign_text PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME mcc_source_distribution COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mcc_source_distribution.py)
    add_test(NAME native_shader_source_distribution COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_shader_sources.py)
    add_executable(mcc_audio_tests tests/mcc_audio_tests.cpp)
    target_link_libraries(mcc_audio_tests PRIVATE halo2_cache)
    add_test(NAME native_mcc_opus COMMAND mcc_audio_tests)
    add_executable(mcc_preserved_shaders_tests tests/mcc_preserved_shaders_tests.cpp)
    target_link_libraries(mcc_preserved_shaders_tests PRIVATE halo2_cache)
    add_test(NAME native_mcc_preserved_shaders COMMAND mcc_preserved_shaders_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/zanzibar.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/imported/mcc-zanzibar-restored-20261013/halo2/h2_maps_win64_dx11/zanzibar.map)
    set_tests_properties(native_mcc_preserved_shaders PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME preserved_vista_match_loading COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_preserve_vista_loading.py)
    add_test(NAME rendering_audit_bounds COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_CURRENT_SOURCE_DIR}/tests -p test_rendering_audit.py)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map")
        add_executable(multiplayer_shading_tests tests/multiplayer_shading_tests.cpp)
        target_link_libraries(multiplayer_shading_tests PRIVATE halo2_renderer Vulkan::Vulkan)
        target_compile_definitions(multiplayer_shading_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
        add_dependencies(multiplayer_shading_tests halo2_shaders)
        add_test(NAME multiplayer_baked_shading COMMAND multiplayer_shading_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_BINARY_DIR}/shading-test)
        add_executable(multiplayer_resize_tests tests/multiplayer_resize_tests.cpp)
        target_link_libraries(multiplayer_resize_tests PRIVATE halo2_renderer Vulkan::Vulkan)
        add_test(NAME multiplayer_effect_resize COMMAND multiplayer_resize_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_BINARY_DIR}/resize-test)
        add_executable(native_view_history_tests tests/native_view_history_tests.cpp)
        target_link_libraries(native_view_history_tests PRIVATE halo2_renderer)
        target_compile_definitions(native_view_history_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        add_test(NAME native_view_history COMMAND native_view_history_tests ${CMAKE_CURRENT_BINARY_DIR}/native-view-history-test)
        set_tests_properties(native_view_history PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(object_lighting_gpu_tests tests/object_lighting_gpu_tests.cpp)
        target_link_libraries(object_lighting_gpu_tests PRIVATE halo2_renderer)
        add_test(NAME object_lighting_view_isolation COMMAND object_lighting_gpu_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_BINARY_DIR}/object-lighting-test)
        add_executable(camouflage_material_gpu_tests tests/camouflage_material_gpu_tests.cpp)
        add_executable(projectile_glow_gpu_tests tests/projectile_glow_gpu_tests.cpp)
        target_compile_definitions(projectile_glow_gpu_tests PRIVATE HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
        target_compile_definitions(camouflage_material_gpu_tests PRIVATE HALO2_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/shaders")
        target_link_libraries(projectile_glow_gpu_tests PRIVATE halo2_renderer)
        add_test(NAME projectile_glow_material_banks COMMAND projectile_glow_gpu_tests "${HALO2_ROCKET_GHOST_MCC_MAP}")
        set_tests_properties(projectile_glow_material_banks PROPERTIES SKIP_RETURN_CODE 77)
        target_link_libraries(camouflage_material_gpu_tests PRIVATE halo2_renderer)
        add_test(NAME camouflage_material_owner_isolation COMMAND camouflage_material_gpu_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_BINARY_DIR}/camouflage-material-test)
        set_tests_properties(camouflage_material_owner_isolation PROPERTIES SKIP_RETURN_CODE 77)
        add_test(NAME camouflage_object_owner_isolation COMMAND camouflage_material_gpu_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_BINARY_DIR}/camouflage-object-test)
        set_tests_properties(camouflage_object_owner_isolation PROPERTIES SKIP_RETURN_CODE 77
            ENVIRONMENT "HALO2_TEST_CAMOUFLAGE_OBJECT_ALIASES=1")
        add_executable(camouflage_render_tests tests/camouflage_render_tests.cpp)
        target_link_libraries(camouflage_render_tests PRIVATE halo2_renderer)
        add_test(NAME camouflage_scene_composition COMMAND camouflage_render_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map ${CMAKE_CURRENT_BINARY_DIR}/camouflage-composition)
        set_tests_properties(camouflage_scene_composition PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(equipment_render_tests tests/equipment_render_tests.cpp)
        target_link_libraries(equipment_render_tests PRIVATE halo2_renderer)
        add_test(NAME mcc_equipment_renderer_gpu COMMAND equipment_render_tests
            "${HALO2_MCC_EQUIPMENT_MAP}" ${CMAKE_CURRENT_BINARY_DIR}/equipment-model-captures
            ${CMAKE_CURRENT_BINARY_DIR}/equipment-model-render.json)
        set_tests_properties(mcc_equipment_renderer_gpu PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
        add_test(NAME native_mcc_equipment_render COMMAND equipment_live_render_tests
            "${HALO2_MCC_EQUIPMENT_MAP}" ${CMAKE_CURRENT_BINARY_DIR}/equipment-live-captures
            ${CMAKE_CURRENT_BINARY_DIR}/equipment-live-render.json)
        set_tests_properties(native_mcc_equipment_render PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 1200)
        add_executable(shader_state_gpu_tests tests/shader_state_gpu_tests.cpp)
        target_link_libraries(shader_state_gpu_tests PRIVATE halo2_renderer)
        add_test(NAME shader_state_color_delivery COMMAND shader_state_gpu_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_BINARY_DIR}/shader-state-test)
        set_tests_properties(shader_state_color_delivery PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(source_machine_gpu_tests tests/source_machine_gpu_tests.cpp)
        target_link_libraries(source_machine_gpu_tests PRIVATE halo2_renderer)
        add_test(NAME source_machine_gpu_update COMMAND source_machine_gpu_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps
            ${CMAKE_CURRENT_BINARY_DIR}/source-machine-test)
        add_executable(source_prop_gpu_tests tests/source_prop_gpu_tests.cpp)
        target_link_libraries(source_prop_gpu_tests PRIVATE halo2_renderer)
        add_test(NAME source_prop_gpu_lighting COMMAND source_prop_gpu_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_BINARY_DIR}/source-prop-test)
        add_executable(world_material_tests tests/world_material_tests.cpp)
        target_link_libraries(world_material_tests PRIVATE halo2_cache)
        add_executable(explosive_prop_shader_tests tests/explosive_prop_shader_tests.cpp)
        target_link_libraries(explosive_prop_shader_tests PRIVATE halo2_cache)
        set(HALO2_EXPLOSIVE_PROP_MCC_MAPS "" CACHE PATH "Prepared MCC maps for explosive prop shader validation")
        add_test(NAME retail_explosive_prop_shaders COMMAND explosive_prop_shader_tests
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
        add_test(NAME mcc_explosive_prop_shaders COMMAND explosive_prop_shader_tests
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps" "${HALO2_EXPLOSIVE_PROP_MCC_MAPS}")
        set_tests_properties(mcc_explosive_prop_shaders PROPERTIES SKIP_RETURN_CODE 77)
        add_test(NAME retail_multiplayer_materials COMMAND world_material_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps)
        add_test(NAME retail_multiplayer_material_lods COMMAND world_material_tests --lower-lods
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps)
        add_test(NAME retail_multiplayer_model_material_lods COMMAND world_material_tests --model-lods
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps)
        add_executable(object_lighting_tests tests/object_lighting_tests.cpp)
        target_link_libraries(object_lighting_tests PRIVATE halo2_cache)
        add_test(NAME retail_object_lighting COMMAND object_lighting_tests)
        set_tests_properties(retail_object_lighting PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_shadow_tests tests/source_shadow_tests.cpp)
        target_link_libraries(source_shadow_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_shadow COMMAND source_shadow_tests)
        set_tests_properties(retail_source_shadow PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_machine_scene_tests tests/source_machine_scene_tests.cpp)
        set(HALO2_MCC_MACHINE_RESOURCE_MAP "" CACHE FILEPATH "Imported MCC FloodZone map for deferred machine resource validation")
        add_executable(mcc_machine_frustum_resource_tests tests/mcc_machine_frustum_resource_tests.cpp)
        target_link_libraries(mcc_machine_frustum_resource_tests PRIVATE halo2_renderer)
        add_test(NAME mcc_machine_frustum_resources COMMAND mcc_machine_frustum_resource_tests "${HALO2_MCC_MACHINE_RESOURCE_MAP}")
        set_tests_properties(mcc_machine_frustum_resources PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(source_machine_portal_tests tests/source_machine_portal_tests.cpp)
        target_link_libraries(source_machine_portal_tests PRIVATE halo2_renderer)
        add_test(NAME retail_source_machine_portal COMMAND source_machine_portal_tests)
        set_tests_properties(retail_source_machine_portal PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(source_machine_scene_tests PRIVATE halo2_renderer)
        add_test(NAME retail_source_machine_scene COMMAND source_machine_scene_tests)
        set_tests_properties(retail_source_machine_scene PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_object_lod_scene_tests tests/source_object_lod_scene_tests.cpp)
        target_link_libraries(source_object_lod_scene_tests PRIVATE halo2_renderer)
        add_test(NAME retail_source_object_lod_scene COMMAND source_object_lod_scene_tests)
        set_tests_properties(retail_source_object_lod_scene PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_object_light_views_tests tests/source_object_light_views_tests.cpp)
        target_link_libraries(source_object_light_views_tests PRIVATE halo2_renderer)
        add_test(NAME retail_source_object_light_views COMMAND source_object_light_views_tests)
        set_tests_properties(retail_source_object_light_views PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_object_shader_geometry_tests tests/source_object_shader_geometry_tests.cpp)
        target_link_libraries(source_object_shader_geometry_tests PRIVATE halo2_renderer)
        add_test(NAME retail_source_object_shader_geometry COMMAND source_object_shader_geometry_tests)
        set_tests_properties(retail_source_object_shader_geometry PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_prop_scene_tests tests/source_prop_scene_tests.cpp)
        target_link_libraries(source_prop_scene_tests PRIVATE halo2_renderer)
        add_test(NAME retail_source_prop_scene COMMAND source_prop_scene_tests)
        add_executable(source_prop_main_view_tu15_tests tests/source_prop_main_view_tu15_tests.cpp)
        target_link_libraries(source_prop_main_view_tu15_tests PRIVATE halo2_renderer)
        add_test(NAME native_tu15_prop_main_view COMMAND source_prop_main_view_tu15_tests
            "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
        set_tests_properties(native_tu15_prop_main_view PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)

        set_tests_properties(retail_source_prop_scene PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_scenery_scene_tests tests/source_scenery_scene_tests.cpp)
        add_executable(source_scenery_shader_tests tests/source_scenery_shader_tests.cpp)
        target_link_libraries(source_scenery_shader_tests PRIVATE halo2_renderer)
        add_test(NAME retail_source_scenery_shader COMMAND source_scenery_shader_tests)
        set_tests_properties(retail_source_scenery_shader PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(source_scenery_scene_tests PRIVATE halo2_renderer)
        add_test(NAME retail_source_scenery_scene COMMAND source_scenery_scene_tests)
        set_tests_properties(retail_source_scenery_scene PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_scenery_gpu_tests tests/source_scenery_gpu_tests.cpp)
        target_link_libraries(source_scenery_gpu_tests PRIVATE halo2_renderer)
        add_test(NAME source_scenery_gpu_lighting COMMAND source_scenery_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/source-scenery-gpu-captures)
        set_tests_properties(source_scenery_gpu_lighting PROPERTIES
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} SKIP_RETURN_CODE 77)
        add_executable(source_bsp_gpu_tests tests/source_bsp_gpu_tests.cpp)
        target_link_libraries(source_bsp_gpu_tests PRIVATE halo2_renderer)
        add_test(NAME source_bsp_gpu_visibility COMMAND source_bsp_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/source-bsp-gpu-captures)
        set_tests_properties(source_bsp_gpu_visibility PROPERTIES
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} SKIP_RETURN_CODE 77)
        add_executable(source_stipple_gpu_tests tests/source_stipple_gpu_tests.cpp)
        target_link_libraries(source_stipple_gpu_tests PRIVATE halo2_renderer)
        add_test(NAME source_stipple_gpu_pixels COMMAND source_stipple_gpu_tests
            ${CMAKE_CURRENT_BINARY_DIR}/source-stipple-gpu-captures)
        set_tests_properties(source_stipple_gpu_pixels PROPERTIES
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} SKIP_RETURN_CODE 77)
        add_executable(source_bsp_shader_scene_tests tests/source_bsp_shader_scene_tests.cpp)
        target_link_libraries(source_bsp_shader_scene_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_bsp_shader_scene COMMAND source_bsp_shader_scene_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps)
        set_tests_properties(retail_source_bsp_shader_scene PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_prt_tests tests/source_prt_tests.cpp)
        target_link_libraries(source_prt_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_prt COMMAND source_prt_tests)
        set_tests_properties(retail_source_prt PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(mcc_prt_vertex_stream_tests tests/mcc_prt_vertex_stream_tests.cpp)
        target_link_libraries(mcc_prt_vertex_stream_tests PRIVATE halo2_cache)
        set(HALO2_MCC_PRT_MAP_DIR "" CACHE PATH "Prepared imported MCC maps for paired original PRT stream validation")
        if(EXISTS "${HALO2_MCC_PRT_MAP_DIR}/03b_newmombasa.map")
            add_test(NAME mcc_prt_vertex_stream COMMAND mcc_prt_vertex_stream_tests
                "${HALO2_MCC_PRT_MAP_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps")
            set_tests_properties(mcc_prt_vertex_stream PROPERTIES TIMEOUT 180)
        endif()
        add_executable(source_device_tests tests/source_device_tests.cpp)
        target_link_libraries(source_device_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_devices COMMAND source_device_tests)
        set_tests_properties(retail_source_devices PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_ai_actor_tests tests/source_ai_actor_tests.cpp)
        add_executable(source_ai_reference_tests tests/source_ai_reference_tests.cpp)
        target_link_libraries(source_ai_reference_tests PRIVATE halo2_systems)
        add_executable(source_ai_import_tests tests/source_ai_import_tests.cpp)
        target_link_libraries(source_ai_import_tests PRIVATE halo2_systems)
        add_executable(source_unit_control_tests tests/source_unit_control_tests.cpp)
        target_link_libraries(source_unit_control_tests PRIVATE halo2_systems)
        add_executable(source_ai_actor_binding_tests tests/source_ai_actor_binding_tests.cpp)
        target_link_libraries(source_ai_actor_binding_tests PRIVATE halo2_systems)
        add_executable(source_ai_actor_lifecycle_tests tests/source_ai_actor_lifecycle_tests.cpp)
        target_link_libraries(source_ai_actor_lifecycle_tests PRIVATE halo2_systems)
        add_executable(source_ai_actor_control_tests tests/source_ai_actor_control_tests.cpp)
        target_link_libraries(source_ai_actor_control_tests PRIVATE halo2_systems)
        add_executable(source_ai_actor_membership_tests tests/source_ai_actor_membership_tests.cpp)
        target_link_libraries(source_ai_actor_membership_tests PRIVATE halo2_systems)
        target_link_libraries(source_ai_actor_tests PRIVATE halo2_systems)
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND Python3_Interpreter_FOUND)
            add_test(NAME retail_source_ai_actor COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/Testing/SourceAiActor" --timeout 60
                -- $<TARGET_FILE:source_ai_actor_tests>
                "${CMAKE_SOURCE_DIR}/tests/fixtures/source_ai_actor_ctor_base.md")
            set_tests_properties(retail_source_ai_actor PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
            add_test(NAME retail_source_ai_references COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/Testing/SourceAiReferences" --timeout 60
                -- $<TARGET_FILE:source_ai_reference_tests>
                "${CMAKE_SOURCE_DIR}/tests/fixtures/source_ai_references_base.md")
            set_tests_properties(retail_source_ai_references PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
            add_test(NAME retail_source_ai_import COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/Testing/SourceAiImport" --timeout 60
                -- $<TARGET_FILE:source_ai_import_tests>
                "${CMAKE_SOURCE_DIR}/assets/disc/maps/01a_tutorial.map"
                "${CMAKE_SOURCE_DIR}/assets/disc/maps"
                "${CMAKE_SOURCE_DIR}/assets/disc/default.xbe")
            set_tests_properties(retail_source_ai_import PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
            add_test(NAME retail_source_unit_control COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/Testing/SourceUnitControl" --timeout 60
                -- $<TARGET_FILE:source_unit_control_tests>
                "${CMAKE_SOURCE_DIR}/tests/fixtures/source_unit_control_base.md"
                "${CMAKE_SOURCE_DIR}/tests/fixtures/source_unit_seats_base.md")
            set_tests_properties(retail_source_unit_control PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
            add_test(NAME retail_source_ai_actor_binding COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/Testing/SourceAiActorBinding" --timeout 60
                -- $<TARGET_FILE:source_ai_actor_binding_tests>
                "${CMAKE_SOURCE_DIR}/tests/fixtures/source_ai_actor_binding_base.md")
            set_tests_properties(retail_source_ai_actor_binding PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
            add_test(NAME retail_source_ai_actor_binding_lifecycle COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/Testing/SourceAiActorBindingLifecycle" --timeout 60
                -- $<TARGET_FILE:source_ai_actor_binding_tests>
                "${CMAKE_SOURCE_DIR}/tests/fixtures/source_ai_actor_binding_lifecycle_base.md")
            set_tests_properties(retail_source_ai_actor_binding_lifecycle PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
            add_test(NAME retail_source_ai_actor_lifecycle COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/Testing/SourceAiActorLifecycle" --timeout 60
                -- $<TARGET_FILE:source_ai_actor_lifecycle_tests>
                "${CMAKE_SOURCE_DIR}/tests/fixtures/source_ai_actor_lifecycle_base.md")
            set_tests_properties(retail_source_ai_actor_lifecycle PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
            add_test(NAME retail_source_ai_actor_commands_lifecycle COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/Testing/SourceAiActorCommandsLifecycle" --timeout 60
                -- $<TARGET_FILE:source_ai_actor_lifecycle_tests>
                "${CMAKE_SOURCE_DIR}/tests/fixtures/source_ai_actor_commands_lifecycle_base.md")
            set_tests_properties(retail_source_ai_actor_commands_lifecycle PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
            add_test(NAME retail_source_ai_actor_lifecycle_source_owned COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/Testing/SourceAiActorLifecycleSourceOwned" --timeout 60
                -- $<TARGET_FILE:source_ai_actor_lifecycle_tests>
                "${CMAKE_SOURCE_DIR}/tests/fixtures/source_ai_actor_lifecycle_base.md" source-owned)
            set_tests_properties(retail_source_ai_actor_lifecycle_source_owned PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
            add_test(NAME retail_source_ai_actor_commands_lifecycle_source_owned COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/Testing/SourceAiActorCommandsLifecycleSourceOwned" --timeout 60
                -- $<TARGET_FILE:source_ai_actor_lifecycle_tests>
                "${CMAKE_SOURCE_DIR}/tests/fixtures/source_ai_actor_commands_lifecycle_base.md" source-owned)
            set_tests_properties(retail_source_ai_actor_commands_lifecycle_source_owned PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
            add_test(NAME retail_source_ai_actor_control COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/Testing/SourceAiActorControl" --timeout 60
                -- $<TARGET_FILE:source_ai_actor_control_tests>
                "${CMAKE_SOURCE_DIR}/tests/fixtures/source_ai_actor_control_base.md")
            set_tests_properties(retail_source_ai_actor_control PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
            add_test(NAME retail_source_ai_actor_control_source_owned COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/Testing/SourceAiActorControlSourceOwned" --timeout 60
                -- $<TARGET_FILE:source_ai_actor_control_tests>
                "${CMAKE_SOURCE_DIR}/tests/fixtures/source_ai_actor_control_base.md" source-owned)
            set_tests_properties(retail_source_ai_actor_control_source_owned PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
            add_test(NAME retail_source_ai_actor_membership COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/Testing/SourceAiActorMembership" --timeout 60
                -- $<TARGET_FILE:source_ai_actor_membership_tests>
                "${CMAKE_SOURCE_DIR}/tests/fixtures/source_ai_actor_membership_base.md")
            set_tests_properties(retail_source_ai_actor_membership PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
            add_test(NAME retail_source_ai_actor_membership_source_owned COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/Testing/SourceAiActorMembershipSourceOwned" --timeout 60
                -- $<TARGET_FILE:source_ai_actor_membership_tests>
                "${CMAKE_SOURCE_DIR}/tests/fixtures/source_ai_actor_membership_base.md" source-owned)
            set_tests_properties(retail_source_ai_actor_membership_source_owned PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
            add_test(NAME retail_source_ai_actor_binding_source_owned COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/Testing/SourceAiActorBindingSourceOwned" --timeout 60
                -- $<TARGET_FILE:source_ai_actor_binding_tests>
                "${CMAKE_SOURCE_DIR}/tests/fixtures/source_ai_actor_binding_base.md" source-owned)
            set_tests_properties(retail_source_ai_actor_binding_source_owned PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
            add_test(NAME retail_source_ai_actor_binding_lifecycle_source_owned COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/tools/tag_match_memory_budget.py"
                --output-root "@HALO2_TEST_SCRATCH@/Testing/SourceAiActorBindingLifecycleSourceOwned" --timeout 60
                -- $<TARGET_FILE:source_ai_actor_binding_tests>
                "${CMAKE_SOURCE_DIR}/tests/fixtures/source_ai_actor_binding_lifecycle_base.md" source-owned)
            set_tests_properties(retail_source_ai_actor_binding_lifecycle_source_owned PROPERTIES RUN_SERIAL TRUE TIMEOUT 90)
        endif()
        add_executable(source_script_tests tests/source_script_tests.cpp)
        target_link_libraries(source_script_tests PRIVATE halo2_systems)
        add_test(NAME retail_source_script COMMAND source_script_tests)
        set(HALO2_SOURCE_SCRIPT_WAKE_FIXTURES "${CMAKE_CURRENT_SOURCE_DIR}/tests/data/source_script_wake_tu15")
        add_executable(source_script_wake_tests tests/source_script_wake_tests.cpp)
        target_link_libraries(source_script_wake_tests PRIVATE halo2_systems)
        add_test(NAME original_tu15_script_wake_dispatch COMMAND source_script_wake_tests
            "${HALO2_SOURCE_SCRIPT_WAKE_FIXTURES}")
        add_executable(source_script_wake_codec_tests tests/source_script_wake_codec_tests.cpp)
        target_link_libraries(source_script_wake_codec_tests PRIVATE halo2_authority_codec OpenSSL::Crypto ZLIB::ZLIB)
        add_test(NAME native_script_external_sleep_codec COMMAND source_script_wake_codec_tests
            "${HALO2_SOURCE_SCRIPT_WAKE_FIXTURES}")
        add_executable(source_script_opening_cues_tu15_tests tests/source_script_opening_cues_tu15_tests.cpp)
        target_link_libraries(source_script_opening_cues_tu15_tests PRIVATE halo2_systems)
        add_test(NAME native_opening_script_cue_relative_ticks COMMAND source_script_opening_cues_tu15_tests
            "${HALO2_SOURCE_SCRIPT_WAKE_FIXTURES}"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map")
        set_tests_properties(native_opening_script_cue_relative_ticks PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(source_script_list_tests tests/source_script_list_tests.cpp)
        target_link_libraries(source_script_list_tests PRIVATE halo2_systems)
        add_test(NAME retail_source_script_lists COMMAND source_script_list_tests)
        add_executable(source_weather_tests tests/source_weather_tests.cpp)
        add_executable(source_game_time_tests tests/source_game_time_tests.cpp
            src/gameplay/source_game_time.cpp src/rendering/weather/source_weather.cpp)
        target_include_directories(source_game_time_tests PRIVATE include)
        add_test(NAME retail_source_game_time COMMAND source_game_time_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_game_time.tsv)
        add_executable(source_weather_assets_tests tests/source_weather_assets_tests.cpp)
        add_executable(source_weather_world_tests tests/source_weather_world_tests.cpp
            src/rendering/weather/source_weather_world.cpp src/runtime/source_runtime_memory.cpp src/rendering/weather/source_weather_wind.cpp)
        target_link_libraries(source_weather_world_tests PRIVATE halo2_snapshot_storage)
        target_include_directories(source_weather_world_tests PRIVATE include)
        target_link_libraries(source_weather_world_tests PRIVATE halo2_cache halo2_weather_simulation)
        add_test(NAME source_weather_world COMMAND source_weather_world_tests ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_weather_world_caller_tests tests/source_weather_world_caller_tests.cpp
            src/rendering/weather/source_weather_world.cpp src/runtime/source_runtime_memory.cpp src/rendering/weather/source_weather_wind.cpp src/rendering/weather/source_weather.cpp)
        target_link_libraries(source_weather_world_caller_tests PRIVATE halo2_snapshot_storage)
        target_include_directories(source_weather_world_caller_tests PRIVATE include)
        target_link_libraries(source_weather_world_caller_tests PRIVATE halo2_cache halo2_weather_simulation)
        if(NOT MSVC)
            target_compile_options(source_weather_world_caller_tests PRIVATE -fno-fast-math -ffp-contract=off)
        endif()
        add_test(NAME retail_source_weather_world_caller COMMAND source_weather_world_caller_tests ${CMAKE_CURRENT_SOURCE_DIR})
        set_tests_properties(retail_source_weather_world_caller PROPERTIES SKIP_RETURN_CODE 77)
        target_link_libraries(source_weather_assets_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_weather_assets COMMAND source_weather_assets_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_weather_geometry.tsv
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_weather_assets.tsv
            ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(source_weather_tests PRIVATE halo2_systems)
        add_test(NAME retail_source_weather COMMAND source_weather_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_weather.tsv)
        add_executable(source_weather_host_tests tests/source_weather_host_tests.cpp)
        target_link_libraries(source_weather_host_tests PRIVATE halo2_systems)
        add_test(NAME retail_backwash_weather_host COMMAND source_weather_host_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/backwash.map)
        add_executable(source_weather_initialization_tests tests/source_weather_initialization_tests.cpp)
        target_link_libraries(source_weather_initialization_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_weather_initialization COMMAND source_weather_initialization_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_weather_initialization.tsv)
        add_executable(source_weather_plate_initialization_tests tests/source_weather_plate_initialization_tests.cpp)
        target_link_libraries(source_weather_plate_initialization_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_weather_plate_initialization COMMAND source_weather_plate_initialization_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_weather_plate_initialization.tsv)
        add_executable(source_weather_plate_simulation_tests tests/source_weather_plate_simulation_tests.cpp)
        target_link_libraries(source_weather_plate_simulation_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_weather_plate_simulation COMMAND source_weather_plate_simulation_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_weather_plate_simulation.tsv)
        add_executable(source_weather_exclusion_tests tests/source_weather_exclusion_tests.cpp)
        target_link_libraries(source_weather_exclusion_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_weather_exclusion COMMAND source_weather_exclusion_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_weather_exclusion.tsv)
        add_executable(source_weather_wind_tests tests/source_weather_wind_tests.cpp)
        target_link_libraries(source_weather_wind_tests PRIVATE halo2_systems)
        add_test(NAME retail_source_weather_wind COMMAND source_weather_wind_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_weather_wind.tsv)
        add_executable(lightmap_tests tests/lightmap_tests.cpp)
        target_link_libraries(lightmap_tests PRIVATE halo2_cache)
        add_test(NAME retail_multiplayer_lightmaps COMMAND lightmap_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps)
        add_executable(scenery_tests tests/scenery_tests.cpp)
        target_link_libraries(scenery_tests PRIVATE halo2_cache)
        add_test(NAME retail_multiplayer_scenery_policies COMMAND scenery_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps)
        add_executable(bsp_surface_lighting_tests tests/bsp_surface_lighting_tests.cpp)
        target_link_libraries(bsp_surface_lighting_tests PRIVATE halo2_systems)
        add_test(NAME retail_multiplayer_surface_lighting COMMAND bsp_surface_lighting_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        add_executable(mcc_bsp_surface_lighting_tests tests/mcc_bsp_surface_lighting_tests.cpp)
        target_link_libraries(mcc_bsp_surface_lighting_tests PRIVATE halo2_cache)
        set(HALO2_MCC_SURFACE_LIGHTING_DONOR "" CACHE FILEPATH "Optional owned MCC map for CPU geometry conversion validation")
        set(HALO2_MCC_SURFACE_LIGHTING_REFERENCE "" CACHE FILEPATH "Optional matching original Xbox map for offline CPU geometry validation")
        add_test(NAME mcc_bsp_surface_lighting COMMAND mcc_bsp_surface_lighting_tests
            "${HALO2_MCC_SURFACE_LIGHTING_DONOR}" "${HALO2_MCC_SURFACE_LIGHTING_REFERENCE}")
        set_tests_properties(mcc_bsp_surface_lighting PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
        add_executable(bsp_representation_tu15_tests tests/bsp_representation_tu15_tests.cpp)
        target_link_libraries(bsp_representation_tu15_tests PRIVATE nlohmann_json::nlohmann_json)
        target_include_directories(bsp_representation_tu15_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
        set(HALO2_BSP_REPRESENTATION_REFERENCE_FIXTURE "" CACHE FILEPATH "Optional offline original CPU BSP packing and SHORT2 decoder fixture")
        add_test(NAME bsp_representation_tu15 COMMAND bsp_representation_tu15_tests
            "${HALO2_BSP_REPRESENTATION_REFERENCE_FIXTURE}")
        set_tests_properties(bsp_representation_tu15 PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
        add_executable(instance_contact_tie_tests tests/instance_contact_tie_tests.cpp)
        target_link_libraries(instance_contact_tie_tests PRIVATE halo2_cache)
        target_compile_definitions(instance_contact_tie_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        add_test(NAME retail_multiplayer_instance_contact_ties COMMAND instance_contact_tie_tests)
        set_tests_properties(retail_multiplayer_instance_contact_ties PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(instance_accelerator_contact_tests tests/instance_accelerator_contact_tests.cpp)
        target_link_libraries(instance_accelerator_contact_tests PRIVATE halo2_cache)
        target_compile_definitions(instance_accelerator_contact_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        add_test(NAME retail_multiplayer_instance_accelerator_contacts COMMAND instance_accelerator_contact_tests)
        set_tests_properties(retail_multiplayer_instance_accelerator_contacts PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(source_instance_accelerator_tests tests/source_instance_accelerator_tests.cpp)
        target_link_libraries(source_instance_accelerator_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_instance_accelerator COMMAND source_instance_accelerator_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_instance_accelerator.tsv)
        add_test(NAME retail_source_instance_accelerator_banks COMMAND source_instance_accelerator_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_instance_accelerator_banks.tsv)
        add_executable(source_bitmap_residency_tests tests/source_bitmap_residency_tests.cpp)
        target_link_libraries(source_bitmap_residency_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_bitmap_residency COMMAND source_bitmap_residency_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_bitmap_residency.tsv)
        add_executable(source_bitmap_cpu_selection_tests tests/source_bitmap_cpu_selection_tests.cpp)
        target_link_libraries(source_bitmap_cpu_selection_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_bitmap_cpu_selection COMMAND source_bitmap_cpu_selection_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_bitmap_cpu_selection.tsv)
        add_executable(source_bitmap_coverage_tests tests/source_bitmap_coverage_tests.cpp)
        target_link_libraries(source_bitmap_coverage_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_bitmap_coverage COMMAND source_bitmap_coverage_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_bitmap_coverage.tsv)
        add_executable(source_bitmap_initialization_tests tests/source_bitmap_initialization_tests.cpp)
        target_link_libraries(source_bitmap_initialization_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_bitmap_initialization COMMAND source_bitmap_initialization_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_bitmap_initialization.tsv)
        add_executable(source_async_jobs_tests tests/source_async_jobs_tests.cpp)
        target_link_libraries(source_async_jobs_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_async_queue COMMAND source_async_jobs_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_async_queue.tsv)
        add_executable(source_async_read_tests tests/source_async_read_tests.cpp)
        target_link_libraries(source_async_read_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_async_read COMMAND source_async_read_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_async_read.tsv)
        add_executable(source_bitmap_async_read_tests tests/source_bitmap_async_read_tests.cpp)
        target_link_libraries(source_bitmap_async_read_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_bitmap_async_read COMMAND source_bitmap_async_read_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_bitmap_async_read.tsv
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        add_executable(source_bitmap_manager_transition_tests tests/source_bitmap_manager_transition_tests.cpp)
        target_link_libraries(source_bitmap_manager_transition_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_bitmap_manager_transition COMMAND source_bitmap_manager_transition_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_bitmap_manager_transition.tsv)
        add_executable(source_bitmap_reservation_tests tests/source_bitmap_reservation_tests.cpp)
        target_link_libraries(source_bitmap_reservation_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_bitmap_reservation COMMAND source_bitmap_reservation_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_bitmap_reservation.tsv)
        add_executable(source_bitmap_image_binding_tests tests/source_bitmap_image_binding_tests.cpp)
        target_link_libraries(source_bitmap_image_binding_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_bitmap_image_binding COMMAND source_bitmap_image_binding_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_bitmap_image_binding.tsv)
        add_executable(source_bitmap_pending_header_tests tests/source_bitmap_pending_header_tests.cpp)
        target_link_libraries(source_bitmap_pending_header_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_bitmap_pending_header COMMAND source_bitmap_pending_header_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_bitmap_pending_header.tsv)
        add_executable(source_bitmap_pending_tests tests/source_bitmap_pending_tests.cpp)
        target_link_libraries(source_bitmap_pending_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_bitmap_pending COMMAND source_bitmap_pending_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_bitmap_pending.tsv)
        add_executable(source_bitmap_partition_tests tests/source_bitmap_partition_tests.cpp)
        target_link_libraries(source_bitmap_partition_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_bitmap_partitions COMMAND source_bitmap_partition_tests)
        set_tests_properties(retail_source_bitmap_partitions PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_bitmap_low_detail_tests tests/source_bitmap_low_detail_tests.cpp)
        target_link_libraries(source_bitmap_low_detail_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_bitmap_low_detail COMMAND source_bitmap_low_detail_tests)
        set_tests_properties(retail_source_bitmap_low_detail PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(object_sky_lighting_tests tests/object_sky_lighting_tests.cpp)
        add_executable(object_light_spatial_tests tests/object_light_spatial_tests.cpp)
        target_link_libraries(object_light_spatial_tests PRIVATE halo2_cache)
        add_test(NAME retail_object_light_spatial COMMAND object_light_spatial_tests)
        set_tests_properties(retail_object_light_spatial PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_object_admission_tests tests/source_object_admission_tests.cpp)
        target_link_libraries(source_object_admission_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_object_admission COMMAND source_object_admission_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_object_admission.tsv
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_object_spatial_cold.tsv)
        add_test(NAME retail_source_object_membership COMMAND source_object_admission_tests --spatial
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_object_membership.tsv)
        set_tests_properties(retail_source_object_membership PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_object_admission_snapshot_tests tests/source_object_admission_snapshot_tests.cpp)
        target_link_libraries(source_object_admission_snapshot_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_object_admission_snapshot COMMAND source_object_admission_snapshot_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_object_admission_snapshot.bin
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_object_admission_snapshot.tsv)
        set_tests_properties(retail_source_object_admission_snapshot PROPERTIES SKIP_RETURN_CODE 77)
        add_test(NAME retail_source_object_admission_shadow COMMAND source_object_admission_snapshot_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_object_admission_shadow.bin
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_object_admission_shadow.tsv)
        set_tests_properties(retail_source_object_admission_shadow PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(source_object_location_tests tests/source_object_location_tests.cpp)
        target_link_libraries(source_object_location_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_object_location COMMAND source_object_location_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_object_location.tsv)
        set_tests_properties(retail_source_object_location PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(model_selection_tests tests/model_selection_tests.cpp)
        target_link_libraries(model_selection_tests PRIVATE halo2_cache)
        add_test(NAME retail_initial_model_selection COMMAND model_selection_tests)
        set_tests_properties(retail_initial_model_selection PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(geometry_lod_model_tests tests/geometry_lod_model_tests.cpp)
        add_executable(source_geometry_lod_tests tests/source_geometry_lod_tests.cpp)
        add_executable(source_visibility_tests tests/source_visibility_tests.cpp)
        add_executable(source_render_policy_tests tests/source_render_policy_tests.cpp)
        target_link_libraries(source_render_policy_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_render_policy COMMAND source_render_policy_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_render_policy.tsv)
        target_link_libraries(source_visibility_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_visibility COMMAND source_visibility_tests)
        set_tests_properties(retail_source_visibility PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_portal_clip_tests tests/source_portal_clip_tests.cpp)
        target_link_libraries(source_portal_clip_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_portal_clip COMMAND source_portal_clip_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_portal_clip.tsv)

        add_executable(source_frustum_camera_tests tests/source_frustum_camera_tests.cpp)
        target_link_libraries(source_frustum_camera_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_frustum_camera COMMAND source_frustum_camera_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_frustum_camera.tsv)
        add_executable(source_portal_frustum_tests tests/source_portal_frustum_tests.cpp)
        target_link_libraries(source_portal_frustum_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_portal_frustum COMMAND source_portal_frustum_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_portal_frustum.tsv)
        add_executable(source_portal_visibility_tests tests/source_portal_visibility_tests.cpp)
        add_executable(source_bsp_admission_tests tests/source_bsp_admission_tests.cpp)
        target_link_libraries(source_bsp_admission_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_bsp_admission COMMAND source_bsp_admission_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_bsp_admission.tsv
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_bsp_admission_assets.tsv)
        add_test(NAME original_effect_light_sphere_admission COMMAND source_bsp_admission_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_sphere_admission.original.tsv
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_bsp_admission_assets.tsv)
        set_tests_properties(original_effect_light_sphere_admission PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_test(NAME original_effect_light_sphere_clusters COMMAND source_bsp_admission_tests --sphere-clusters
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/effect_light_sphere_clusters.original.tsv)
        set_tests_properties(retail_source_bsp_admission PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_test(NAME retail_source_bsp_fog_admission COMMAND source_bsp_admission_tests --fog
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_bsp_fog_admission.tsv
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_bsp_fog_admission_assets.tsv)
        set_tests_properties(retail_source_bsp_fog_admission PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_portal_mask_tests tests/source_portal_mask_tests.cpp)
        add_executable(mainmenu_portal_state_tests tests/mainmenu_portal_state_tests.cpp)
        target_link_libraries(mainmenu_portal_state_tests PRIVATE halo2_cache)
        add_test(NAME retail_mainmenu_portal_state COMMAND mainmenu_portal_state_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mainmenu_portal_state_original.tsv)
        set_tests_properties(retail_mainmenu_portal_state PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(mainmenu_bsp_frame_tests tests/mainmenu_bsp_frame_tests.cpp)
        target_link_libraries(mainmenu_bsp_frame_tests PRIVATE halo2_systems)
        add_test(NAME original_mainmenu_bsp_frame COMMAND mainmenu_bsp_frame_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mainmenu_bsp_frame_original.bin)
        set_tests_properties(original_mainmenu_bsp_frame PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(mainmenu_bsp_ranges_tests tests/mainmenu_bsp_ranges_tests.cpp)
        target_link_libraries(mainmenu_bsp_ranges_tests PRIVATE halo2_systems)
        add_test(NAME original_mainmenu_bsp_ranges COMMAND mainmenu_bsp_ranges_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mainmenu_bsp_ranges_original.bin)
        set_tests_properties(original_mainmenu_bsp_ranges PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(mainmenu_world_source_tests tests/mainmenu_world_source_tests.cpp)
        target_link_libraries(mainmenu_world_source_tests PRIVATE halo2_systems)
        add_test(NAME retail_mainmenu_world_source COMMAND mainmenu_world_source_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/mainmenu_bsp_frame_original.bin
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/title-camera-matrix-audit/world-vertex-stage3)
        set_tests_properties(retail_mainmenu_world_source PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(source_transparent_sort_tests tests/source_transparent_sort_tests.cpp)
        add_executable(king_queue_submission_tests tests/king_queue_submission_tests.cpp
            src/rendering/source_transparent_queue.cpp src/rendering/source_transparent_sort.cpp)
        target_include_directories(king_queue_submission_tests PRIVATE include)
        target_link_libraries(king_queue_submission_tests PRIVATE nlohmann_json::nlohmann_json)
        if(MSVC)
            target_compile_options(king_queue_submission_tests PRIVATE /fp:strict)
        else()
            target_compile_options(king_queue_submission_tests PRIVATE -fno-fast-math -ffp-contract=off)
        endif()
        add_test(NAME original_king_queue_submission COMMAND king_queue_submission_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/game-variants/king-queue-submission-source-20261012/complete/report.json)
        add_executable(source_transparent_queue_tests tests/source_transparent_queue_tests.cpp)
        target_link_libraries(source_transparent_queue_tests PRIVATE halo2_cache)
        add_test(NAME original_source_transparent_queue COMMAND source_transparent_queue_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_transparent_queue_original.tsv
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/shield_fp_suffix_original.tsv)
        add_executable(source_transparent_parts_tests tests/source_transparent_parts_tests.cpp)
        add_executable(source_shader_lod_tests tests/source_shader_lod_tests.cpp)
        add_executable(source_model_shader_tests tests/source_model_shader_tests.cpp)
        target_link_libraries(source_model_shader_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_model_shader COMMAND source_model_shader_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_model_shader.tsv)
        add_executable(source_dynamic_model_shaders_tests tests/source_dynamic_model_shaders_tests.cpp)
        target_link_libraries(source_dynamic_model_shaders_tests PRIVATE halo2_cache)
        target_compile_definitions(source_dynamic_model_shaders_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        add_test(NAME retail_dynamic_model_shader_integration COMMAND source_dynamic_model_shaders_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_dynamic_model_shader_assets.tsv)
        add_executable(scene_game_context_tests tests/scene_game_context_tests.cpp)
        target_link_libraries(scene_game_context_tests PRIVATE halo2_systems)
        add_test(NAME retail_scene_game_context COMMAND scene_game_context_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_scene_game_context.tsv
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps)
        set_tests_properties(retail_scene_game_context PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(scene_creation_order_tests tests/scene_creation_order_tests.cpp)
        target_link_libraries(scene_creation_order_tests PRIVATE halo2_systems)
        add_test(NAME retail_scene_creation_order COMMAND scene_creation_order_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_scene_creation_order.tsv)
        set_tests_properties(retail_scene_creation_order PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(scene_render_attribution_tests tests/scene_render_attribution_tests.cpp)
        target_link_libraries(scene_render_attribution_tests PRIVATE halo2_systems)
        add_test(NAME native_scene_render_attribution COMMAND scene_render_attribution_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_object_owner_flag31_original.tsv)
        set_tests_properties(native_scene_render_attribution PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(source_object_primitive_tests tests/source_object_primitive_tests.cpp)
        target_link_libraries(source_object_primitive_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_object_primitive COMMAND source_object_primitive_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_object_primitive.tsv)
        add_executable(source_object_spatial_pool_tests tests/source_object_spatial_pool_tests.cpp)
        add_executable(source_object_hierarchy_tests tests/source_object_hierarchy_tests.cpp)
        target_link_libraries(source_object_hierarchy_tests PRIVATE halo2_cache)
        add_test(NAME original_source_object_hierarchy COMMAND source_object_hierarchy_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_object_hierarchy_original.tsv)
        target_link_libraries(source_object_spatial_pool_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_object_spatial_pool COMMAND source_object_spatial_pool_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_object_spatial_pool.tsv)
        add_executable(source_object_spatial_scan_tests tests/source_object_spatial_scan_tests.cpp)
        add_executable(source_object_spatial_query_tests tests/source_object_spatial_query_tests.cpp)
        target_link_libraries(source_object_spatial_query_tests PRIVATE halo2_cache)
        add_test(NAME source_object_spatial_query_guards COMMAND source_object_spatial_query_tests)
        add_test(NAME retail_source_object_spatial_query COMMAND source_object_spatial_query_tests --fixture
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_object_spatial_query.tsv)
        target_link_libraries(source_object_spatial_scan_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_object_spatial_scan COMMAND source_object_spatial_scan_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_object_spatial_scan.tsv)
        add_executable(source_object_vector_contact_tests tests/source_object_vector_contact_tests.cpp)
        target_link_libraries(source_object_vector_contact_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_object_vector_contact COMMAND source_object_vector_contact_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_object_vector_contact.tsv)
        add_executable(source_object_light_attribution_tests tests/source_object_light_attribution_tests.cpp)
        target_link_libraries(source_object_light_attribution_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_object_light_attribution COMMAND source_object_light_attribution_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_object_light_attribution.tsv)
        add_executable(source_object_light_streams_tests tests/source_object_light_streams_tests.cpp)
        target_link_libraries(source_object_light_streams_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_object_light_streams COMMAND source_object_light_streams_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_object_light_streams.tsv)
        add_executable(source_vector_contact_tests tests/source_vector_contact_tests.cpp)
        target_link_libraries(source_vector_contact_tests PRIVATE halo2_cache)
        target_compile_definitions(source_vector_contact_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        add_test(NAME retail_source_vector_contact COMMAND source_vector_contact_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_object_admission_snapshot.bin
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_vector_contact.tsv)
        set_tests_properties(retail_source_vector_contact PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(source_render_contact_tests tests/source_render_contact_tests.cpp)
        target_link_libraries(source_render_contact_tests PRIVATE halo2_cache)
        target_compile_definitions(source_render_contact_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        add_test(NAME retail_source_render_contact COMMAND source_render_contact_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_object_admission_snapshot.bin
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_render_contact.tsv)
        set_tests_properties(retail_source_render_contact PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(source_object_light_rays_tests tests/source_object_light_rays_tests.cpp)
        target_link_libraries(source_object_light_rays_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_object_light_rays COMMAND source_object_light_rays_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_object_light_rays.tsv)
        add_executable(source_map_light_sample_tests tests/source_map_light_sample_tests.cpp)
        target_link_libraries(source_map_light_sample_tests PRIVATE halo2_systems)
        add_test(NAME retail_source_map_light_sample COMMAND source_map_light_sample_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_map_light_sample.tsv
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/zanzibar.map)
        set_tests_properties(retail_source_map_light_sample PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(source_model_palette_tests tests/source_model_palette_tests.cpp)
        target_link_libraries(source_model_palette_tests PRIVATE halo2_cache)
        target_compile_definitions(source_model_palette_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        add_test(NAME retail_source_model_palette COMMAND source_model_palette_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_model_palette.tsv)
        add_test(NAME retail_source_model_palette_assets COMMAND source_model_palette_tests --assets
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_model_palette_assets.tsv)
        add_executable(source_render_cache_tests tests/source_render_cache_tests.cpp)
        target_link_libraries(source_render_cache_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_render_cache COMMAND source_render_cache_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_render_cache.tsv)
        add_executable(source_model_pose_tests tests/source_model_pose_tests.cpp)
        target_link_libraries(source_model_pose_tests PRIVATE halo2_cache)
        target_compile_definitions(source_model_pose_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        add_test(NAME retail_source_model_pose COMMAND source_model_pose_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_model_pose.tsv)
        add_test(NAME retail_source_model_pose_assets COMMAND source_model_pose_tests --assets
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_model_pose_assets.tsv)
        add_executable(source_model_root_tests tests/source_model_root_tests.cpp)
        target_link_libraries(source_model_root_tests PRIVATE halo2_cache)
        target_compile_definitions(source_model_root_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        if(MSVC)
            target_compile_options(source_model_root_tests PRIVATE /fp:strict)
        else()
            target_compile_options(source_model_root_tests PRIVATE -ffp-contract=off)
        endif()
        add_test(NAME retail_source_model_root_assets COMMAND source_model_root_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_model_root_assets.tsv)
        add_test(NAME retail_source_model_root_fp COMMAND source_model_root_tests --graph
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_model_root_fp.tsv)
        add_executable(source_shadow_submission_tests tests/source_shadow_submission_tests.cpp)
        target_link_libraries(source_shadow_submission_tests PRIVATE halo2_cache)
        add_executable(source_shadow_lighting_tests tests/source_shadow_lighting_tests.cpp)
        target_link_libraries(source_shadow_lighting_tests PRIVATE halo2_cache)
        target_compile_definitions(source_shadow_lighting_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        add_test(NAME retail_source_shadow_lighting COMMAND source_shadow_lighting_tests)
        add_test(NAME retail_source_shadow_submission COMMAND source_shadow_submission_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_shadow_submission.tsv)
        add_executable(source_shadow_live_tests tests/source_shadow_live_tests.cpp)
        target_link_libraries(source_shadow_live_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_shadow_live COMMAND source_shadow_live_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_shadow_live.tsv)
        add_executable(model_extra_stream_tests tests/model_extra_stream_tests.cpp)
        target_link_libraries(model_extra_stream_tests PRIVATE halo2_cache)
        target_compile_definitions(model_extra_stream_tests PRIVATE HALO2_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        add_test(NAME retail_model_extra_streams COMMAND model_extra_stream_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_model_extra_streams.tsv)
        set_tests_properties(retail_model_extra_streams PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(source_shader_color_animation_tests tests/source_shader_color_animation_tests.cpp)
        target_link_libraries(source_shader_color_animation_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_shader_color_animation COMMAND source_shader_color_animation_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_shader_color_animation.tsv)
        add_executable(source_stipple_tests tests/source_stipple_tests.cpp)
        target_link_libraries(source_stipple_tests PRIVATE halo2_cache)
        add_test(NAME xbox_hardware_stipple COMMAND source_stipple_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/xbox_hardware_stipple.tsv)
        target_link_libraries(source_shader_lod_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_shader_lod COMMAND source_shader_lod_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_shader_lod.tsv)
        add_executable(source_shader_lod_asset_tests tests/source_shader_lod_asset_tests.cpp)
        target_link_libraries(source_shader_lod_asset_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_shader_lod_assets COMMAND source_shader_lod_asset_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_shader_lod_assets.tsv)
        set_tests_properties(retail_source_shader_lod_assets PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(source_transparent_parts_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_transparent_parts COMMAND source_transparent_parts_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_transparent_parts.tsv)
        add_test(NAME retail_source_transparent_parts_assets COMMAND source_transparent_parts_tests --assets
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_transparent_parts_assets.tsv)
        set_tests_properties(retail_source_transparent_parts_assets PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(source_transparent_sort_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_transparent_sort COMMAND source_transparent_sort_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_transparent_sort.tsv)
        target_link_libraries(source_portal_mask_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_portal_mask COMMAND source_portal_mask_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_portal_mask.tsv)
        add_test(NAME original_campaign_portal_mask_tu15 COMMAND source_portal_mask_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_portal_mask_tu15.txt)
        add_test(NAME retail_source_portal_mask_assets COMMAND source_portal_mask_tests --assets)
        set_tests_properties(retail_source_portal_mask_assets PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(source_portal_visibility_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_portal_visibility COMMAND source_portal_visibility_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_portal_visibility.tsv
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_portal_assets.tsv)
        set_tests_properties(retail_source_portal_visibility PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(source_geometry_lod_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_geometry_lod COMMAND source_geometry_lod_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_geometry_lod.tsv)
        add_test(NAME retail_source_geometry_lod_metric COMMAND source_geometry_lod_tests --metric
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_geometry_lod_metric.tsv)
        target_link_libraries(geometry_lod_model_tests PRIVATE halo2_cache)
        add_test(NAME retail_geometry_lod_models COMMAND geometry_lod_model_tests)
        set_tests_properties(retail_geometry_lod_models PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(object_initialization_tests tests/object_initialization_tests.cpp)
        target_link_libraries(object_initialization_tests PRIVATE halo2_cache)
        add_test(NAME retail_object_initialization COMMAND object_initialization_tests)
        set_tests_properties(retail_object_initialization PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_prop_initial_state_tests tests/source_prop_initial_state_tests.cpp)
        target_link_libraries(source_prop_initial_state_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_prop_initial_state COMMAND source_prop_initial_state_tests)
        set_tests_properties(retail_source_prop_initial_state PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_scenery_initial_state_tests tests/source_scenery_initial_state_tests.cpp)
        target_link_libraries(source_scenery_initial_state_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_scenery_initial_state COMMAND source_scenery_initial_state_tests)
        set_tests_properties(retail_source_scenery_initial_state PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_scenery_idle_phase_tests tests/source_scenery_idle_phase_tests.cpp)
        target_link_libraries(source_scenery_idle_phase_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_scenery_idle_phase COMMAND source_scenery_idle_phase_tests)
        set_tests_properties(retail_source_scenery_idle_phase PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_euler_tests tests/source_euler_tests.cpp)
        target_link_libraries(source_euler_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_euler COMMAND source_euler_tests)
        set_tests_properties(retail_source_euler PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_affine_pose_tests tests/source_affine_pose_tests.cpp)
        target_link_libraries(source_affine_pose_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_affine_pose COMMAND source_affine_pose_tests)
        set_tests_properties(retail_source_affine_pose PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_view_projection_tests tests/source_view_projection_tests.cpp)
        target_link_libraries(source_view_projection_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_view_projection COMMAND source_view_projection_tests)

        add_executable(campaign_cinematic_camera_lens_tu15_tests tests/campaign_cinematic_camera_lens_tu15_tests.cpp)
        target_link_libraries(campaign_cinematic_camera_lens_tu15_tests PRIVATE halo2_cache)
        add_test(NAME campaign_cinematic_camera_lens_tu15 COMMAND campaign_cinematic_camera_lens_tu15_tests)
        add_executable(source_view_shader_bank_tests tests/source_view_shader_bank_tests.cpp)
        target_link_libraries(source_view_shader_bank_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_view_shader_bank COMMAND source_view_shader_bank_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_view_shader_bank.tsv)
        set_tests_properties(retail_source_view_projection PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(source_object_colors_tests tests/source_object_colors_tests.cpp)
        target_link_libraries(source_object_colors_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_object_colors COMMAND source_object_colors_tests)
        set_tests_properties(retail_source_object_colors PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(object_sky_lighting_tests PRIVATE halo2_systems)
        add_test(NAME retail_multiplayer_object_sky_lighting COMMAND object_sky_lighting_tests)
        set_tests_properties(retail_multiplayer_object_sky_lighting PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        add_executable(bsp_sky_models_tests tests/bsp_sky_models_tests.cpp)
        target_link_libraries(bsp_sky_models_tests PRIVATE halo2_systems)
        add_test(NAME retail_campaign_per_view_skies COMMAND bsp_sky_models_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        add_executable(bsp_atmosphere_tests tests/bsp_atmosphere_tests.cpp)
        target_link_libraries(bsp_atmosphere_tests PRIVATE halo2_cache)
        add_executable(bsp_clear_color_tests tests/bsp_clear_color_tests.cpp)
        target_link_libraries(bsp_clear_color_tests PRIVATE halo2_cache)
        add_test(NAME retail_fog_clear_color_math COMMAND bsp_clear_color_tests --math ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_fog_clear.tsv)
        add_executable(source_view_clear_tests tests/source_view_clear_tests.cpp)
        target_link_libraries(source_view_clear_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_view_clear_schedule COMMAND source_view_clear_tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/source_view_clear.tsv)
        add_test(NAME retail_multiplayer_atmosphere COMMAND bsp_atmosphere_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps)
        add_executable(water_tests tests/water_tests.cpp)
        target_link_libraries(water_tests PRIVATE halo2_cache)
        add_test(NAME retail_multiplayer_water COMMAND water_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps)
        add_test(NAME original_tu15_water_materials COMMAND water_tests --materials
            "${HALO2_TU15_GAMEPLAY_XBE}"
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/zanzibar.map
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/05b_deltatowers.map
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/08a_deltacliffs.map)
        set_tests_properties(original_tu15_water_materials PROPERTIES SKIP_RETURN_CODE 77)
        add_executable(weapon_display_tests tests/weapon_display_tests.cpp)
        add_executable(source_weapon_shader_input_tests tests/source_weapon_shader_input_tests.cpp)
        add_executable(source_weapon_material_input_tests EXCLUDE_FROM_ALL tests/source_weapon_material_input_tests.cpp)
        add_executable(source_weapon_material_namespace_tests EXCLUDE_FROM_ALL tests/source_weapon_material_namespace_tests.cpp)
        target_link_libraries(source_weapon_material_namespace_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
        add_test(NAME original_tu15_weapon_ammunition_digits COMMAND source_weapon_material_namespace_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_ammunition_digits_tu15.json)
        add_executable(source_weapon_random_constant_tests EXCLUDE_FROM_ALL tests/source_weapon_random_constant_tests.cpp)
        target_link_libraries(source_weapon_random_constant_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
        add_test(NAME original_tu15_object_material_random_constant COMMAND source_weapon_random_constant_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/object_material_random_constant_tu15.json)
        add_executable(source_weapon_alive_tests EXCLUDE_FROM_ALL tests/source_weapon_alive_tests.cpp)
        target_link_libraries(source_weapon_alive_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
        add_test(NAME original_tu15_weapon_alive COMMAND source_weapon_alive_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_alive_material_tu15.json)
        add_executable(source_weapon_firing_material_tests EXCLUDE_FROM_ALL tests/source_weapon_firing_material_tests.cpp)
        target_link_libraries(source_weapon_firing_material_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
        add_test(NAME original_tu15_weapon_firing_material COMMAND source_weapon_firing_material_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_firing_material_tu15.json)
        add_executable(native_tu15_network_configuration_tests EXCLUDE_FROM_ALL tests/native_tu15_network_configuration_tests.cpp)
        target_link_libraries(native_tu15_network_configuration_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
        add_test(NAME native_tu15_network_configuration_defaults COMMAND native_tu15_network_configuration_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/network_configuration_defaults_tu15.json)
        add_executable(native_tu15_game_language_tests EXCLUDE_FROM_ALL tests/native_tu15_game_language_tests.cpp)
        target_link_libraries(native_tu15_game_language_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
        add_test(NAME native_tu15_game_language_storage COMMAND native_tu15_game_language_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/game_language_tu15_original.json)
        target_link_libraries(source_weapon_material_input_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
        add_test(NAME original_tu15_weapon_material_inputs COMMAND source_weapon_material_input_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_material_builtins_tu15.json
            ${CMAKE_CURRENT_BINARY_DIR}/weapon-material-input-acceptance.json)
        add_executable(source_netgame_weapon_scene_tests tests/source_netgame_weapon_scene_tests.cpp)
        target_link_libraries(source_netgame_weapon_scene_tests PRIVATE halo2_renderer)
        add_test(NAME retail_source_netgame_weapon_scene COMMAND source_netgame_weapon_scene_tests)
        set_tests_properties(retail_source_netgame_weapon_scene PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(source_weapon_shader_input_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_weapon_shader_input COMMAND source_weapon_shader_input_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_source_weapon_shader_input.tsv)
        add_executable(source_netgame_weapon_tests tests/source_netgame_weapon_tests.cpp)
        target_link_libraries(source_netgame_weapon_tests PRIVATE halo2_cache)
        add_test(NAME retail_source_netgame_weapon COMMAND source_netgame_weapon_tests)
        set_tests_properties(retail_source_netgame_weapon PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
        target_link_libraries(weapon_display_tests PRIVATE halo2_renderer Vulkan::Vulkan)
        target_compile_definitions(weapon_display_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
        add_dependencies(weapon_display_tests halo2_shaders)
        add_test(NAME retail_weapon_display COMMAND weapon_display_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        add_test(NAME retail_shader_animation COMMAND shader_animation_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        add_test(NAME retail_light_volume COMMAND light_volume_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map)
        add_test(NAME retail_appearance_inputs COMMAND appearance_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        add_test(NAME retail_rendering_content COMMAND rendering_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        add_test(NAME retail_first_person COMMAND first_person_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        add_test(NAME retail_map_geometry COMMAND halo2_native --validate-maps ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        add_test(NAME retail_system_content COMMAND halo2_native --validate-systems ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        add_test(NAME retail_system_runtime COMMAND systems_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
        add_test(NAME retail_campaign_skip COMMAND campaign_tests ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
    endif()
endif()

# Actual SDL Q pulse -> native UDP delivery; source throw/world owners are separate.
if(BUILD_TESTING)
  add_executable(prop_region_render_tests tests/prop_region_render_tests.cpp)
  target_link_libraries(prop_region_render_tests PRIVATE halo2_renderer)
  add_test(NAME live_prop_regions COMMAND prop_region_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/turf.map"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03")
  add_test(NAME live_prop_region_render COMMAND prop_region_render_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/turf.map"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03")
  set_tests_properties(live_prop_regions live_prop_region_render PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(prop_sandbox_tests tests/prop_sandbox_tests.cpp)
  target_link_libraries(prop_sandbox_tests PRIVATE halo2_systems)
  add_executable(prop_impact_capture tools/capture_prop_impact_presentation.cpp src/ui/frontend.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
  target_link_libraries(prop_impact_capture PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
  target_compile_definitions(prop_impact_capture PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
  add_test(NAME live_prop_sandbox COMMAND prop_sandbox_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03")
  set_tests_properties(live_prop_sandbox PROPERTIES SKIP_RETURN_CODE 77)
endif()

if(BUILD_TESTING)
  add_executable(multiplayer_grenade_delivery_tests tests/multiplayer_grenade_delivery_tests.cpp)
  target_link_libraries(multiplayer_grenade_delivery_tests PRIVATE halo2_systems)
  add_test(NAME multiplayer_grenade_delivery COMMAND multiplayer_grenade_delivery_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03")
  set_tests_properties(multiplayer_grenade_delivery PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(native_grenade_timer_tests tests/native_grenade_timer_tests.cpp)
  target_link_libraries(native_grenade_timer_tests PRIVATE halo2_systems)
  add_test(NAME original_native_grenade_timer COMMAND native_grenade_timer_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/analysis/multiplayer/integration/live-grenade-20261010/timer-measured/fixtures.bin")
  set_tests_properties(original_native_grenade_timer PROPERTIES SKIP_RETURN_CODE 77)
endif()

if(BUILD_TESTING)
  add_executable(mcc_sound_rate_tests tests/mcc_sound_rate_tests.cpp)
  target_link_libraries(mcc_sound_rate_tests PRIVATE halo2_systems)
  add_test(NAME mcc_sound_codec_rates COMMAND mcc_sound_rate_tests)
  set_tests_properties(mcc_sound_codec_rates PROPERTIES ENVIRONMENT "SDL_AUDIODRIVER=dummy")
  add_executable(multiplayer_audio_tests tests/multiplayer_audio_tests.cpp)
  target_link_libraries(multiplayer_audio_tests PRIVATE halo2_systems)
  include(cmake/MultiplayerAudioContinuation.cmake)
  add_test(NAME multiplayer_authored_audio COMMAND multiplayer_audio_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03")
  set_tests_properties(multiplayer_authored_audio PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT "SDL_AUDIODRIVER=dummy")
endif()

# Native mover presentation adapter; original channel clock and material sounds.
target_sources(halo2_systems PRIVATE src/gameplay/units/native_locomotion.cpp src/gameplay/animation/native_player_animation.cpp src/gameplay/units/biped_landing.cpp src/gameplay/animation/native_melee_animation.cpp)
if(MSVC)
  set_source_files_properties(src/gameplay/units/native_locomotion.cpp src/gameplay/animation/native_player_animation.cpp src/gameplay/units/biped_landing.cpp src/gameplay/animation/native_melee_animation.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
  set_source_files_properties(src/gameplay/units/native_locomotion.cpp src/gameplay/animation/native_player_animation.cpp src/gameplay/units/biped_landing.cpp src/gameplay/animation/native_melee_animation.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(BUILD_TESTING)
  add_executable(native_melee_gameplay_tests tests/native_melee_gameplay_tests.cpp)
  add_executable(melee_impact_effect_tests tests/melee_impact_effect_tests.cpp)
  target_link_libraries(melee_impact_effect_tests PRIVATE halo2_systems)
  add_test(NAME native_melee_and_sword_contact_effects COMMAND melee_impact_effect_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/melee_impact_routing_original.tsv)
  set_tests_properties(native_melee_and_sword_contact_effects PROPERTIES SKIP_RETURN_CODE 77)
  target_link_libraries(native_melee_gameplay_tests PRIVATE halo2_systems halo2_cache)
  add_executable(native_player_overlay_tests tests/native_player_overlay_tests.cpp)
  target_link_libraries(native_player_overlay_tests PRIVATE halo2_systems)
  add_test(NAME native_player_body_overlays COMMAND native_player_overlay_tests)
  add_executable(native_locomotion_tests tests/native_locomotion_tests.cpp)
  target_link_libraries(native_locomotion_tests PRIVATE halo2_systems)
  add_test(NAME native_authored_footsteps COMMAND native_locomotion_tests
    "${CMAKE_SOURCE_DIR}/assets/disc/maps/lockout.map"
    "${CMAKE_SOURCE_DIR}/assets/disc/maps"
    "${CMAKE_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03"
    "${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/live-authored-audio-20261010/footstep-original/fixtures.tsv")
  set_tests_properties(native_authored_footsteps PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT "SDL_AUDIODRIVER=dummy")
  add_test(NAME native_disc_player_animations COMMAND native_locomotion_tests
    "${CMAKE_SOURCE_DIR}/assets/disc/maps/lockout.map"
    "${CMAKE_SOURCE_DIR}/assets/disc/maps"
    "${CMAKE_SOURCE_DIR}/assets/disc/default.xbe"
    "${CMAKE_SOURCE_DIR}/analysis/multiplayer/integration/live-authored-audio-20261010/footstep-original/fixtures.tsv")
  set_tests_properties(native_disc_player_animations PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT "SDL_AUDIODRIVER=dummy")
  add_executable(native_weapon_pose_tests tests/native_weapon_pose_tests.cpp)
  target_link_libraries(native_weapon_pose_tests PRIVATE halo2_systems)
  add_test(NAME native_authored_weapon_poses COMMAND native_weapon_pose_tests
    "${CMAKE_SOURCE_DIR}/assets/disc/maps/lockout.map"
    "${CMAKE_SOURCE_DIR}/assets/disc/maps"
    "${CMAKE_SOURCE_DIR}/assets/disc/default.xbe")
  set_tests_properties(native_authored_weapon_poses PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Opt-in profiler for the actual interactive multiplayer/renderer path.
add_executable(profile_mp_render_cpu EXCLUDE_FROM_ALL tools/profile_mp_render_cpu.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(profile_mp_render_cpu PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
add_executable(vehicle_skin_reuse_runtime_tests EXCLUDE_FROM_ALL
    tests/vehicle_skin_reuse_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(vehicle_skin_reuse_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
add_executable(unit_skin_reuse_runtime_tests EXCLUDE_FROM_ALL
    tests/unit_skin_reuse_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(unit_skin_reuse_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
if(BUILD_TESTING)
    set(HALO2_VEHICLE_SKIN_MCC_MAPS "" CACHE PATH "Prepared MCC maps for split-screen vehicle skin equality")
    foreach(players 2 4)
        add_test(NAME native_vehicle_skin_reuse_${players}p COMMAND vehicle_skin_reuse_runtime_tests
            "${HALO2_VEHICLE_SKIN_MCC_MAPS}/coagulation.map"
            "${HALO2_VEHICLE_SKIN_MCC_MAPS}" ${players})
        set_tests_properties(native_vehicle_skin_reuse_${players}p PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
    endforeach()
    add_test(NAME native_unit_skin_reuse_4p COMMAND unit_skin_reuse_runtime_tests
        "${HALO2_VEHICLE_SKIN_MCC_MAPS}/coagulation.map" "${HALO2_VEHICLE_SKIN_MCC_MAPS}")
    set_tests_properties(native_unit_skin_reuse_4p PROPERTIES TIMEOUT 180 SKIP_RETURN_CODE 77)
endif()

# Supplied Vista lifecycle slot2: exact start-game handler/control services.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_session_loading_lifecycle.cpp)
if(BUILD_TESTING)
  add_executable(vista_session_loading_lifecycle_tests
    tests/vista_session_loading_lifecycle_tests.cpp
    src/ui/vista/vista_session_loading_lifecycle.cpp src/ui/vista/vista_lifecycle.cpp)
  target_include_directories(vista_session_loading_lifecycle_tests PRIVATE include)
  add_test(NAME original_vista_session_loading_lifecycle
    COMMAND vista_session_loading_lifecycle_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_session_loading_lifecycle_original.tsv)
endif()

# Supplied Vista retained PCR banks, including the original row tie ordering.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_pcr_results.cpp)
if(BUILD_TESTING)
  add_executable(vista_pcr_results_tests tests/vista_pcr_results_tests.cpp
    src/ui/vista/vista_pcr_results.cpp)
  target_include_directories(vista_pcr_results_tests PRIVATE include)
  add_test(NAME original_vista_pcr_results COMMAND vista_pcr_results_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_pcr_results_original.tsv)
endif()

# Supplied Vista loading player preparation and its concrete refresh owner writes.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_session_player_preparation.cpp)
if(BUILD_TESTING)
  add_executable(vista_session_player_preparation_tests
    tests/vista_session_player_preparation_tests.cpp
    src/ui/vista/vista_session_player_preparation.cpp)
  target_include_directories(vista_session_player_preparation_tests PRIVATE include)
  add_test(NAME original_vista_session_player_preparation
    COMMAND vista_session_player_preparation_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_session_player_preparation_original.tsv)
endif()

# Supplied Vista concrete retained transport/storage and complete peer payload.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_session_storage.cpp src/ui/vista/vista_session_local_peer.cpp)
if(MSVC)
  set_source_files_properties(src/ui/vista/vista_session_local_peer.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
  set_source_files_properties(src/ui/vista/vista_session_local_peer.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(BUILD_TESTING)
  add_executable(vista_session_owned_inputs_tests tests/vista_session_owned_inputs_tests.cpp
    src/ui/vista/vista_session_storage.cpp src/ui/vista/vista_session_local_peer.cpp)
  target_include_directories(vista_session_owned_inputs_tests PRIVATE include)
  target_link_libraries(vista_session_owned_inputs_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_session_owned_inputs COMMAND vista_session_owned_inputs_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_session_owned_inputs_original.jsonl)
endif()

if(BUILD_TESTING)
    add_executable(mainmenu_effects_tests tests/mainmenu_effects_tests.cpp)
    target_link_libraries(mainmenu_effects_tests PRIVATE halo2_systems)
    add_test(NAME native_mainmenu_flak_explosions COMMAND mainmenu_effects_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/mainmenu.map)
    add_executable(world_effects_tests tests/world_effects_tests.cpp)
    target_link_libraries(world_effects_tests PRIVATE halo2_systems)
    add_executable(weapon_impact_effect_tests tests/weapon_impact_effect_tests.cpp)
    target_link_libraries(weapon_impact_effect_tests PRIVATE halo2_systems)
    add_test(NAME native_authored_weapon_impact_effects COMMAND weapon_impact_effect_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe)
    add_executable(projectile_impact_angle_tests tests/projectile_impact_angle_tests.cpp)
    target_link_libraries(projectile_impact_angle_tests PRIVATE halo2_systems)
    add_test(NAME original_projectile_impact_angles COMMAND projectile_impact_angle_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/projectile_impact_angle_original.tsv)
    set_tests_properties(original_projectile_impact_angles PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(projectile_impact_inputs_tests tests/projectile_impact_inputs_tests.cpp)
    add_executable(native_projectile_effect_inputs_tests tests/native_projectile_effect_inputs_tests.cpp)
    target_link_libraries(native_projectile_effect_inputs_tests PRIVATE halo2_systems)
    add_test(NAME native_projectile_effect_input_owners COMMAND native_projectile_effect_inputs_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe)
    target_link_libraries(projectile_impact_inputs_tests PRIVATE halo2_systems)
    add_test(NAME original_tu15_projectile_impact_inputs COMMAND projectile_impact_inputs_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/projectile_impact_inputs_original.bin)
    set_tests_properties(original_tu15_projectile_impact_inputs PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(projectile_damage_presentation_tests tests/projectile_damage_presentation_tests.cpp)
    target_link_libraries(projectile_damage_presentation_tests PRIVATE halo2_systems)
    add_test(NAME original_tu15_projectile_damage_presentation COMMAND projectile_damage_presentation_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/projectile_damage_presentation_original.bin)
    set_tests_properties(original_tu15_projectile_damage_presentation PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_world_impact_and_grenade_effects COMMAND world_effects_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    add_test(NAME native_world_presentation_handoff COMMAND world_effects_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps "${HALO2_TU15_GAMEPLAY_XBE}")
    set_tests_properties(native_world_presentation_handoff PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 180)
    add_executable(particle_collision_response_tests tests/particle_collision_response_tests.cpp)
    target_link_libraries(particle_collision_response_tests PRIVATE halo2_cache)
    add_test(NAME original_particle_collision_response COMMAND particle_collision_response_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/particle_collision_response.original.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/particle_collision_sound_gain.original.bin)
    add_executable(material_sound_effects_tests tests/material_sound_effects_tests.cpp)
    target_link_libraries(material_sound_effects_tests PRIVATE halo2_cache)
    add_test(NAME original_sound_material_effects COMMAND material_sound_effects_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_material_effects.original.tsv)
    add_executable(material_visual_effects_tests tests/material_visual_effects_tests.cpp)
    target_link_libraries(material_visual_effects_tests PRIVATE halo2_cache)
    add_test(NAME original_visual_material_effects COMMAND material_visual_effects_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/visual_material_effects.original.tsv)
    add_test(NAME original_hard_terrain_particle_birth COMMAND particle_birth_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hard_terrain_particle_birth.inputs.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/hard_terrain_particle_birth.original.bin)
    add_test(NAME original_plasma_particle_birth COMMAND particle_birth_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/plasma_particle_birth.inputs.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/plasma_particle_birth.original.bin)
    add_executable(world_plasma_effects_tests tests/world_plasma_effects_tests.cpp)
    target_link_libraries(world_plasma_effects_tests PRIVATE halo2_systems)
    add_test(NAME native_world_plasma_grenade_effects COMMAND world_plasma_effects_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/midship.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
    add_executable(world_effect_maps_tests tests/world_effect_maps_tests.cpp)
    target_link_libraries(world_effect_maps_tests PRIVATE halo2_systems)
    add_test(NAME native_world_effect_multiplayer_maps COMMAND world_effect_maps_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
endif()

# Supplied Cartographer DLL offline user/machine banks and retained XOR RNG.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_offline_identity.cpp src/ui/vista/native_vista_session.cpp src/ui/vista/vista_session_disposal.cpp)
if(WIN32)
  target_link_libraries(halo2_systems PRIVATE bcrypt iphlpapi ws2_32)
endif()
if(BUILD_TESTING)
  add_executable(vista_offline_identity_tests tests/vista_offline_identity_tests.cpp
    src/ui/vista/vista_offline_identity.cpp)
  target_include_directories(vista_offline_identity_tests PRIVATE include)
  add_test(NAME original_vista_offline_identity COMMAND vista_offline_identity_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_offline_identity_original.tsv)
endif()

# Supplied Vista six PCR row consumers, input actions and intro draw.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_pcr_screen.cpp src/ui/vista/vista_pcr_owner_state.cpp)
if(MSVC)
  set_source_files_properties(src/ui/vista/vista_pcr_screen.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
  set_source_files_properties(src/ui/vista/vista_pcr_screen.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(BUILD_TESTING)
  add_executable(vista_pcr_screen_tests tests/vista_pcr_screen_tests.cpp src/ui/vista/vista_pcr_screen.cpp)
  target_include_directories(vista_pcr_screen_tests PRIVATE include)
  target_link_libraries(vista_pcr_screen_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_pcr_screen COMMAND vista_pcr_screen_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_pcr_screen_original.json)
endif()

# Supplied Vista complete retained local-user reconciliation and final-player tail.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_session_reconciliation.cpp)
if(BUILD_TESTING)
  add_executable(vista_session_reconciliation_tests tests/vista_session_reconciliation_tests.cpp
    src/ui/vista/vista_session_reconciliation.cpp src/ui/vista/vista_session_local_peer.cpp
    src/ui/vista/vista_controller_profile.cpp src/ui/vista/vista_local_user.cpp)
  target_include_directories(vista_session_reconciliation_tests PRIVATE include)
  target_link_libraries(vista_session_reconciliation_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_session_reconciliation COMMAND vista_session_reconciliation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_session_reconciliation_original.jsonl)
endif()

# Supplied Vista full raw-bank phase request and PCR Done session action.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_session_phase.cpp src/ui/vista/vista_session_transition.cpp)
if(BUILD_TESTING)
  add_executable(vista_session_phase_tests tests/vista_session_phase_tests.cpp
    src/ui/vista/vista_session_phase.cpp)
  target_include_directories(vista_session_phase_tests PRIVATE include)
  target_link_libraries(vista_session_phase_tests PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
  add_test(NAME original_vista_session_phase COMMAND vista_session_phase_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_session_phase_original.json)
endif()

# Supplied Vista PCR descriptor, rank, palette, skin, frame and emblem owners.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_pcr_appearance.cpp)
if(MSVC)
  set_source_files_properties(src/ui/vista/vista_pcr_appearance.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
  set_source_files_properties(src/ui/vista/vista_pcr_appearance.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING)
  add_executable(vista_pcr_appearance_tests tests/vista_pcr_appearance_tests.cpp
    src/ui/vista/vista_pcr_appearance.cpp)
  target_include_directories(vista_pcr_appearance_tests PRIVATE include)
  target_link_libraries(vista_pcr_appearance_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_pcr_appearance COMMAND vista_pcr_appearance_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_pcr_appearance_original.jsonl)
endif()

# Source-authoritative dead-biped joint construction and grouping owners.
target_sources(halo2_systems PRIVATE src/physics/vehicles/physics_ragdoll_constraints.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_death_runtime.cpp)
if(MSVC)
    set_source_files_properties(src/physics/vehicles/physics_ragdoll_constraints.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/physics/vehicles/physics_ragdoll_constraints.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(BUILD_TESTING)
    add_executable(ragdoll_owners_tests tests/ragdoll_owners_tests.cpp)
    target_link_libraries(ragdoll_owners_tests PRIVATE halo2_systems)
    add_test(NAME original_ragdoll_owners COMMAND ragdoll_owners_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/ragdoll_owners_original.tsv)
    set_tests_properties(original_ragdoll_owners PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Actual PC16/185 list/group bridge, original retained fixture rows and shader gaps.
if(BUILD_TESTING)
  add_executable(frontend_vista_pcr_painter_tests tests/frontend_vista_pcr_painter_tests.cpp)
  target_link_libraries(frontend_vista_pcr_painter_tests PRIVATE halo2_renderer nlohmann_json::nlohmann_json)
  add_dependencies(frontend_vista_pcr_painter_tests halo2_shaders)
  add_test(NAME retail_frontend_vista_pcr_painter COMMAND frontend_vista_pcr_painter_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
    "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map"
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_pcr_results_original.tsv)
  set_tests_properties(retail_frontend_vista_pcr_painter PROPERTIES SKIP_RETURN_CODE 77)
  add_test(NAME mcc_classic_menu_pcr_import COMMAND frontend_vista_pcr_painter_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/mcc-classic-menus
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_pcr_results_original.tsv)
  set_tests_properties(mcc_classic_menu_pcr_import PROPERTIES SKIP_RETURN_CODE 77
    ENVIRONMENT "H2_TEST_MENU_ASSETS=${CMAKE_CURRENT_SOURCE_DIR}/assets/mcc-classic-menus")
endif()

# Concrete native local offline banks and full supplied lifecycle3/4 transition.
if(WIN32)
  target_link_libraries(halo2_systems PUBLIC bcrypt iphlpapi ws2_32)
endif()
if(BUILD_TESTING)
  add_executable(native_vista_session_tests tests/native_vista_session_tests.cpp
    src/ui/vista/native_vista_session.cpp src/ui/vista/vista_session_birth.cpp src/ui/vista/vista_session_disposal.cpp
    src/ui/vista/vista_session_storage.cpp src/ui/vista/vista_offline_identity.cpp
    src/ui/vista/vista_local_user.cpp src/ui/vista/vista_session_local_peer.cpp
    src/ui/vista/vista_session_reconciliation.cpp src/ui/vista/vista_controller_profile.cpp
    src/ui/vista/vista_profile_catalog.cpp src/ui/vista/vista_lobby_input.cpp)
  target_include_directories(native_vista_session_tests PRIVATE include)
  target_link_libraries(native_vista_session_tests PRIVATE nlohmann_json::nlohmann_json)
  if(WIN32)
    target_link_libraries(native_vista_session_tests PRIVATE bcrypt iphlpapi ws2_32)
  endif()
  add_test(NAME original_native_vista_session COMMAND native_vista_session_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_vista_session_original.jsonl)
  add_test(NAME original_native_vista_menu_services COMMAND native_vista_session_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_vista_session_original.jsonl
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_vista_menu_services_original.json)

  add_executable(vista_session_transition_tests tests/vista_session_transition_tests.cpp
    src/ui/vista/vista_session_transition.cpp src/ui/vista/vista_lifecycle.cpp src/ui/vista/vista_session_phase.cpp)
  target_include_directories(vista_session_transition_tests PRIVATE include)
  target_link_libraries(vista_session_transition_tests PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
  add_test(NAME original_vista_session_transition COMMAND vista_session_transition_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_session_transition_original.json)
endif()

# Supplied Vista PCR pane headings, live chrome and retained-XUID voice decisions.
if(BUILD_TESTING)
  add_executable(vista_pcr_owner_state_tests tests/vista_pcr_owner_state_tests.cpp src/ui/vista/vista_pcr_owner_state.cpp)
  target_include_directories(vista_pcr_owner_state_tests PRIVATE include)
  target_link_libraries(vista_pcr_owner_state_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_pcr_owner_state COMMAND vista_pcr_owner_state_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_pcr_owner_state_original.json)
  add_executable(vista_pcr_chrome_tests tests/vista_pcr_chrome_tests.cpp src/ui/vista/vista_pcr_screen.cpp)
  target_include_directories(vista_pcr_chrome_tests PRIVATE include)
  target_link_libraries(vista_pcr_chrome_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_pcr_chrome COMMAND vista_pcr_chrome_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_pcr_chrome_original.json)
endif()

# Original PC retained-layout PCR consumed snapshot bridge; no Xbox bank cast.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_pcr_publication.cpp)
if(BUILD_TESTING)
  add_executable(vista_pcr_publication_tests tests/vista_pcr_publication_tests.cpp
    src/ui/vista/vista_pcr_publication.cpp src/gameplay/multiplayer/source_game_variant.cpp)
  target_include_directories(vista_pcr_publication_tests PRIVATE include)
  add_test(NAME original_vista_pcr_publication COMMAND vista_pcr_publication_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_pcr_publication_original.tsv)
endif()

# Supplied Vista complete raw-bank null/pregame/start-game joined transition.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_session_start_transition.cpp
  src/ui/vista/vista_session_selection.cpp)
if(BUILD_TESTING)
  add_executable(vista_session_start_transition_tests tests/vista_session_start_transition_tests.cpp
    src/ui/vista/vista_session_start_transition.cpp src/ui/vista/vista_session_transition.cpp
    src/ui/vista/vista_session_commit.cpp src/ui/vista/vista_session_phase.cpp
    src/ui/vista/vista_session_player_preparation.cpp src/ui/vista/vista_lifecycle.cpp)
  target_include_directories(vista_session_start_transition_tests PRIVATE include)
  target_link_libraries(vista_session_start_transition_tests PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
  add_test(NAME original_vista_session_start_transition COMMAND vista_session_start_transition_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_session_start_transition_original.json)
endif()

# Supplied Vista local map/variant setters with complete original bank comparison.
if(BUILD_TESTING)
  add_executable(vista_session_selection_tests tests/vista_session_selection_tests.cpp
    src/ui/vista/vista_session_selection.cpp)
  target_include_directories(vista_session_selection_tests PRIVATE include)
  target_link_libraries(vista_session_selection_tests PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
  add_test(NAME original_vista_session_selection COMMAND vista_session_selection_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_session_selection_original.json)
endif()

# Dynamic original lobby start action, state/stage adapters and countdown beeps.
if(BUILD_TESTING)
  add_executable(vista_lobby_presentation_tests tests/vista_lobby_presentation_tests.cpp
    src/ui/vista/vista_lobby_countdown.cpp)
  target_include_directories(vista_lobby_presentation_tests PRIVATE include)
  target_link_libraries(vista_lobby_presentation_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_lobby_presentation COMMAND vista_lobby_presentation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_presentation_original.json)
endif()

# Original PC14 layout owners, independent of the Xbox lobby presentation.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_lobby_layout.cpp)
if(BUILD_TESTING)
  add_executable(vista_lobby_layout_tests tests/vista_lobby_layout_tests.cpp)
  target_link_libraries(vista_lobby_layout_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_lobby_layout COMMAND vista_lobby_layout_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_layout_original.json)
  set_tests_properties(original_vista_lobby_layout PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Original PC14 privacy and authored description-list owners.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_lobby_info.cpp)
if(BUILD_TESTING)
  add_executable(vista_lobby_info_tests tests/vista_lobby_info_tests.cpp)
  target_link_libraries(vista_lobby_info_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_lobby_info COMMAND vista_lobby_info_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/vista-session-flow-audit/pc14-help-privacy-original/evidence.json
    ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/vista-session-flow-audit/pc14-help-privacy-original/execution.json)
endif()

# Original PC14 status tuple, source footer captions and session-bank adapter.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_lobby_status.cpp)
if(BUILD_TESTING)
  add_executable(vista_lobby_status_tests tests/vista_lobby_status_tests.cpp)
  target_link_libraries(vista_lobby_status_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_lobby_status COMMAND vista_lobby_status_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_status_original.json)
endif()

# Original PC14 roster configuration consumer and protected chat predicate.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_lobby_roster.cpp src/ui/vista/vista_lobby_chat.cpp)
if(BUILD_TESTING)
  add_executable(vista_lobby_roster_tests tests/vista_lobby_roster_tests.cpp)
  target_link_libraries(vista_lobby_roster_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_lobby_roster COMMAND vista_lobby_roster_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/vista-session-flow-audit/pc14-roster-original/evidence.json
    ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/vista-session-flow-audit/pc14-roster-original/execution.json)
  set_tests_properties(original_vista_lobby_roster PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vista_lobby_chat_tests tests/vista_lobby_chat_tests.cpp)
  target_link_libraries(vista_lobby_chat_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_lobby_chat COMMAND vista_lobby_chat_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_chat_original.json)
  set_tests_properties(original_vista_lobby_chat PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Source PC14 download indicators advance on screen updates, not wall time.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_lobby_spinner.cpp)
if(BUILD_TESTING)
  add_executable(vista_lobby_spinner_tests tests/vista_lobby_spinner_tests.cpp)
  target_link_libraries(vista_lobby_spinner_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_lobby_spinner COMMAND vista_lobby_spinner_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_spinner_original.json)
endif()

# Original PC14 queries, detail text/map resources, retained chat/browser state,
# and child layout. These source owners do not switch the visible lobby route.
target_sources(halo2_systems PRIVATE
  src/ui/vista/vista_lobby_queries.cpp src/ui/vista/vista_lobby_details.cpp
  src/ui/vista/vista_lobby_chat_queue.cpp src/ui/vista/vista_lobby_browser.cpp
  src/ui/vista/vista_lobby_child_layout.cpp src/ui/vista/vista_lobby_button.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set_source_files_properties(src/ui/vista/vista_lobby_queries.cpp PROPERTIES
    COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING)
  foreach(owner queries details chat_queue browser child_layout button)
    add_executable(vista_lobby_${owner}_tests tests/vista_lobby_${owner}_tests.cpp)
    target_link_libraries(vista_lobby_${owner}_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_vista_lobby_${owner} COMMAND vista_lobby_${owner}_tests
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_${owner}_original.json)
    set_tests_properties(original_vista_lobby_${owner} PROPERTIES SKIP_RETURN_CODE 77)
  endforeach()
endif()

# Original PC206 standard/custom map publication callbacks and retained edit
# bank writes. The visible map selector still requires its genuine factory.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_lobby_map_commit.cpp)
if(BUILD_TESTING)
  add_executable(vista_lobby_map_commit_tests tests/vista_lobby_map_commit_tests.cpp
    src/ui/vista/vista_lobby_map_commit.cpp)
  target_include_directories(vista_lobby_map_commit_tests PRIVATE include)
  target_link_libraries(vista_lobby_map_commit_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_lobby_map_commit COMMAND vista_lobby_map_commit_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_map_commit_original.json
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_map_commit_mutations_original.json)
  set_tests_properties(original_vista_lobby_map_commit PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Original PC206 factory, focus, direct text lookup and tab/input owners.
# Generic/list constructors, registration and the live selector remain external.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_map_selector.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set_source_files_properties(src/ui/vista/vista_map_selector.cpp PROPERTIES
    COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING)
  add_executable(vista_map_selector_tests tests/vista_map_selector_tests.cpp
    src/ui/vista/vista_map_selector.cpp)
  target_include_directories(vista_map_selector_tests PRIVATE include)
  target_link_libraries(vista_map_selector_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_map_selector COMMAND vista_map_selector_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_map_selector_original.json)
  set_tests_properties(original_vista_map_selector PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Complete original PC14 update order and direct retained-state writes.
# Session services and the visible screen lifecycle remain separate owners.
target_sources(halo2_cache PRIVATE src/ui/widgets/vista_widget_cache.cpp)
target_sources(halo2_systems PRIVATE src/ui/widgets/native_vista_widget_cache.cpp)
target_sources(halo2_systems PRIVATE src/ui/widgets/native_vista_widget_memory.cpp)
target_sources(halo2_systems PRIVATE src/ui/vista/vista_lobby_update.cpp src/ui/vista/vista_lobby_sort.cpp
  src/ui/vista/vista_lobby_constructor.cpp src/ui/vista/vista_lobby_initializer.cpp
  src/ui/widgets/vista_widget_registration.cpp src/ui/widgets/vista_widget_activation.cpp
  src/ui/widgets/vista_widget_grid_identifiers.cpp src/ui/widgets/vista_widget_text_clone.cpp
  src/ui/widgets/vista_widget_constructor.cpp src/ui/widgets/vista_widget_animation_descriptor.cpp
  src/ui/widgets/vista_widget_clone_string.cpp src/ui/vista/vista_lobby_factory.cpp
  src/ui/widgets/vista_widget_asset_birth.cpp src/ui/vista/vista_lobby_widget_queries.cpp
  src/ui/widgets/vista_widget_animation_consumer.cpp src/ui/widgets/vista_widget_grid_initialize.cpp
  src/ui/widgets/vista_widget_skin_children.cpp src/ui/widgets/vista_widget_resource_layout.cpp
  src/ui/widgets/vista_widget_child_bounds.cpp src/ui/widgets/vista_widget_header_footer.cpp
  src/ui/widgets/vista_widget_child_creation.cpp src/ui/widgets/vista_widget_text_assignment.cpp src/ui/widgets/vista_widget_bounds.cpp
  src/ui/widgets/vista_widget_bitmap_bounds.cpp src/ui/widgets/vista_widget_font.cpp
  src/ui/widgets/vista_widget_locale.cpp src/ui/widgets/vista_widget_disposal.cpp
  src/ui/widgets/vista_widget_text_measure.cpp src/ui/widgets/vista_widget_manager_lifetime.cpp
  src/ui/widgets/vista_widget_glyph_support.cpp src/ui/widgets/vista_widget_bitmap_update.cpp
  src/ui/widgets/vista_widget_glyph_layout.cpp
  src/ui/widgets/vista_widget_screen_disposal.cpp
  src/ui/widgets/vista_widget_pool.cpp
  src/ui/widgets/native_vista_widget_managers.cpp)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set_source_files_properties(src/ui/widgets/vista_widget_animation_consumer.cpp
    src/ui/widgets/vista_widget_bounds.cpp src/ui/widgets/vista_widget_bitmap_bounds.cpp src/ui/widgets/vista_widget_font.cpp
    src/ui/widgets/vista_widget_text_measure.cpp src/ui/widgets/vista_widget_bitmap_update.cpp
    src/ui/widgets/vista_widget_glyph_layout.cpp PROPERTIES
    COMPILE_OPTIONS "-ffp-contract=off;-fno-fast-math;-frounding-math")
endif()
if(BUILD_TESTING)
  add_executable(vista_widget_glyph_layout_tests tests/vista_widget_glyph_layout_tests.cpp
    src/ui/widgets/vista_widget_glyph_layout.cpp src/ui/widgets/vista_widget_font.cpp)
  target_include_directories(vista_widget_glyph_layout_tests PRIVATE include)
  target_link_libraries(vista_widget_glyph_layout_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_glyph_layout COMMAND vista_widget_glyph_layout_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_glyph_layout_original.json)
  set_tests_properties(original_vista_widget_glyph_layout PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(native_vista_widget_memory_tests tests/native_vista_widget_memory_tests.cpp
    src/ui/widgets/native_vista_widget_memory.cpp src/runtime/source_runtime_memory.cpp
    src/ui/widgets/native_vista_widget_managers.cpp src/ui/widgets/vista_widget_manager_lifetime.cpp
    src/ui/widgets/native_vista_widget_cache.cpp src/ui/widgets/vista_widget_cache.cpp
    src/ui/widgets/vista_widget_screen_disposal.cpp src/ui/widgets/vista_widget_locale.cpp)
  target_link_libraries(native_vista_widget_memory_tests PRIVATE halo2_snapshot_storage)
  target_include_directories(native_vista_widget_memory_tests PRIVATE include)
  target_link_libraries(native_vista_widget_memory_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME native_vista_widget_memory_lifetime COMMAND native_vista_widget_memory_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map"
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_screen_disposal_original.json)
  set_tests_properties(native_vista_widget_memory_lifetime PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_pool_tests tests/vista_widget_pool_tests.cpp
    src/ui/widgets/vista_widget_pool.cpp src/ui/widgets/vista_widget_manager_lifetime.cpp)
  target_include_directories(vista_widget_pool_tests PRIVATE include)
  target_link_libraries(vista_widget_pool_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_pool COMMAND vista_widget_pool_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_pool_original.json)
  set_tests_properties(original_vista_widget_pool PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_screen_disposal_tests tests/vista_widget_screen_disposal_tests.cpp
    src/ui/widgets/vista_widget_screen_disposal.cpp src/ui/widgets/vista_widget_locale.cpp)
  target_include_directories(vista_widget_screen_disposal_tests PRIVATE include)
  target_link_libraries(vista_widget_screen_disposal_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_screen_disposal COMMAND vista_widget_screen_disposal_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_screen_disposal_original.json)
  set_tests_properties(original_vista_widget_screen_disposal PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(native_vista_widget_cache_tests tests/native_vista_widget_cache_tests.cpp
    src/ui/widgets/native_vista_widget_cache.cpp src/ui/widgets/vista_widget_cache.cpp src/ui/widgets/vista_widget_bitmap_update.cpp)
  target_include_directories(native_vista_widget_cache_tests PRIVATE include)
  target_link_libraries(native_vista_widget_cache_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME native_vista_widget_cache_lifetime COMMAND native_vista_widget_cache_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map"
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_bitmap_update_original.json)
  set_tests_properties(native_vista_widget_cache_lifetime PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_bitmap_update_tests tests/vista_widget_bitmap_update_tests.cpp
    src/ui/widgets/vista_widget_bitmap_update.cpp)
  target_include_directories(vista_widget_bitmap_update_tests PRIVATE include)
  target_link_libraries(vista_widget_bitmap_update_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_bitmap_update COMMAND vista_widget_bitmap_update_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map"
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_bitmap_update_original.json)
  set_tests_properties(original_vista_widget_bitmap_update PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_glyph_support_tests tests/vista_widget_glyph_support_tests.cpp
    src/ui/widgets/vista_widget_glyph_support.cpp)
  target_include_directories(vista_widget_glyph_support_tests PRIVATE include)
  target_link_libraries(vista_widget_glyph_support_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_glyph_support COMMAND vista_widget_glyph_support_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_glyph_support_original.json)
  set_tests_properties(original_vista_widget_glyph_support PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_manager_lifetime_tests tests/vista_widget_manager_lifetime_tests.cpp
    src/ui/widgets/vista_widget_manager_lifetime.cpp src/ui/widgets/native_vista_widget_managers.cpp)
  target_include_directories(vista_widget_manager_lifetime_tests PRIVATE include)
  target_link_libraries(vista_widget_manager_lifetime_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_manager_lifetime COMMAND vista_widget_manager_lifetime_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_manager_lifetime_original.json)
  set_tests_properties(original_vista_widget_manager_lifetime PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_text_measure_tests tests/vista_widget_text_measure_tests.cpp
    src/ui/widgets/vista_widget_text_measure.cpp)
  target_include_directories(vista_widget_text_measure_tests PRIVATE include)
  target_link_libraries(vista_widget_text_measure_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_text_measure COMMAND vista_widget_text_measure_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_text_measure_original.json)
  set_tests_properties(original_vista_widget_text_measure PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_locale_disposal_tests tests/vista_widget_locale_disposal_tests.cpp
    src/ui/widgets/vista_widget_locale.cpp src/ui/widgets/vista_widget_disposal.cpp)
  target_include_directories(vista_widget_locale_disposal_tests PRIVATE include)
  target_link_libraries(vista_widget_locale_disposal_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_locale_disposal COMMAND vista_widget_locale_disposal_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_locale_disposal_original.json)
  set_tests_properties(original_vista_widget_locale_disposal PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_font_tests tests/vista_widget_font_tests.cpp
    src/ui/widgets/vista_widget_font.cpp)
  target_include_directories(vista_widget_font_tests PRIVATE include)
  target_link_libraries(vista_widget_font_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_font COMMAND vista_widget_font_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_font_original.json)
  set_tests_properties(original_vista_widget_font PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_bounds_tests tests/vista_widget_bounds_tests.cpp
    src/ui/widgets/vista_widget_bounds.cpp)
  target_include_directories(vista_widget_bounds_tests PRIVATE include)
  target_link_libraries(vista_widget_bounds_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_bounds COMMAND vista_widget_bounds_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map"
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_bounds_original.json)
  set_tests_properties(original_vista_widget_bounds PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_bitmap_bounds_tests tests/vista_widget_bitmap_bounds_tests.cpp
    src/ui/widgets/vista_widget_bitmap_bounds.cpp src/ui/widgets/vista_widget_bounds.cpp)
  target_include_directories(vista_widget_bitmap_bounds_tests PRIVATE include)
  target_link_libraries(vista_widget_bitmap_bounds_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_bitmap_bounds COMMAND vista_widget_bitmap_bounds_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_bitmap_bounds_original.json)
  set_tests_properties(original_vista_widget_bitmap_bounds PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_text_assignment_tests tests/vista_widget_text_assignment_tests.cpp
    src/ui/widgets/vista_widget_text_assignment.cpp src/ui/widgets/vista_widget_clone_string.cpp)
  target_include_directories(vista_widget_text_assignment_tests PRIVATE include)
  target_link_libraries(vista_widget_text_assignment_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_text_assignment COMMAND vista_widget_text_assignment_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_text_assignment_original.json)
  set_tests_properties(original_vista_widget_text_assignment PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_child_creation_tests tests/vista_widget_child_creation_tests.cpp
    src/ui/widgets/vista_widget_child_creation.cpp src/ui/widgets/vista_widget_asset_birth.cpp
    src/ui/widgets/vista_widget_constructor.cpp src/ui/widgets/vista_widget_activation.cpp
    src/ui/widgets/vista_widget_registration.cpp)
  target_include_directories(vista_widget_child_creation_tests PRIVATE include)
  target_link_libraries(vista_widget_child_creation_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_child_creation COMMAND vista_widget_child_creation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_child_creation_original.json)
  set_tests_properties(original_vista_widget_child_creation PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_header_footer_tests tests/vista_widget_header_footer_tests.cpp
    src/ui/widgets/vista_widget_header_footer.cpp)
  target_include_directories(vista_widget_header_footer_tests PRIVATE include)
  target_link_libraries(vista_widget_header_footer_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_header_footer COMMAND vista_widget_header_footer_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_header_footer_original.json)
  set_tests_properties(original_vista_widget_header_footer PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_child_bounds_tests tests/vista_widget_child_bounds_tests.cpp
    src/ui/widgets/vista_widget_child_bounds.cpp)
  target_include_directories(vista_widget_child_bounds_tests PRIVATE include)
  target_link_libraries(vista_widget_child_bounds_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_child_bounds COMMAND vista_widget_child_bounds_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_child_bounds_original.json)
  set_tests_properties(original_vista_widget_child_bounds PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_grid_initialize_tests tests/vista_widget_grid_initialize_tests.cpp
    src/ui/widgets/vista_widget_grid_initialize.cpp)
  target_include_directories(vista_widget_grid_initialize_tests PRIVATE include)
  target_link_libraries(vista_widget_grid_initialize_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_grid_initialize COMMAND vista_widget_grid_initialize_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_grid_initialize_original.json)
  set_tests_properties(original_vista_widget_grid_initialize PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_skin_children_tests tests/vista_widget_skin_children_tests.cpp
    src/ui/widgets/vista_widget_skin_children.cpp)
  target_include_directories(vista_widget_skin_children_tests PRIVATE include)
  target_link_libraries(vista_widget_skin_children_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_skin_children COMMAND vista_widget_skin_children_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_skin_children_original.json)
  set_tests_properties(original_vista_widget_skin_children PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_resource_layout_tests tests/vista_widget_resource_layout_tests.cpp
    src/ui/widgets/vista_widget_resource_layout.cpp src/ui/widgets/vista_widget_cache.cpp)
  target_include_directories(vista_widget_resource_layout_tests PRIVATE include)
  target_link_libraries(vista_widget_resource_layout_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_resource_layout COMMAND vista_widget_resource_layout_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map"
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_resource_layout_original.json)
  set_tests_properties(original_vista_widget_resource_layout PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_lobby_widget_queries_tests tests/vista_lobby_widget_queries_tests.cpp
    src/ui/vista/vista_lobby_widget_queries.cpp)
  target_include_directories(vista_lobby_widget_queries_tests PRIVATE include)
  target_link_libraries(vista_lobby_widget_queries_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_lobby_widget_queries COMMAND vista_lobby_widget_queries_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_widget_queries_original.json)
  set_tests_properties(original_vista_lobby_widget_queries PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_animation_consumer_tests tests/vista_widget_animation_consumer_tests.cpp
    src/ui/widgets/vista_widget_animation_consumer.cpp)
  target_include_directories(vista_widget_animation_consumer_tests PRIVATE include)
  target_link_libraries(vista_widget_animation_consumer_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_animation_consumer COMMAND vista_widget_animation_consumer_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_animation_consumer_original.json)
  set_tests_properties(original_vista_widget_animation_consumer PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_cache_tests tests/vista_widget_cache_tests.cpp src/ui/widgets/vista_widget_cache.cpp)
  target_include_directories(vista_widget_cache_tests PRIVATE include)
  target_link_libraries(vista_widget_cache_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_cache COMMAND vista_widget_cache_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map"
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_cache_original.json)
  set_tests_properties(original_vista_widget_cache PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_lobby_factory_tests tests/vista_lobby_factory_tests.cpp src/ui/vista/vista_lobby_factory.cpp
    src/ui/vista/vista_lobby_constructor.cpp)
  target_include_directories(vista_lobby_factory_tests PRIVATE include)
  target_link_libraries(vista_lobby_factory_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_lobby_factory COMMAND vista_lobby_factory_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_factory_original.json)
  set_tests_properties(original_vista_lobby_factory PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_asset_birth_tests tests/vista_widget_asset_birth_tests.cpp
    src/ui/widgets/vista_widget_asset_birth.cpp src/ui/widgets/vista_widget_constructor.cpp src/ui/widgets/vista_widget_activation.cpp)
  target_include_directories(vista_widget_asset_birth_tests PRIVATE include)
  target_link_libraries(vista_widget_asset_birth_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_asset_birth COMMAND vista_widget_asset_birth_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_asset_birth_original.json)
  set_tests_properties(original_vista_widget_asset_birth PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_animation_descriptor_tests tests/vista_widget_animation_descriptor_tests.cpp
    src/ui/widgets/vista_widget_animation_descriptor.cpp)
  target_include_directories(vista_widget_animation_descriptor_tests PRIVATE include)
  target_link_libraries(vista_widget_animation_descriptor_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_animation_descriptor COMMAND vista_widget_animation_descriptor_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_animation_descriptor_original.json)
  set_tests_properties(original_vista_widget_animation_descriptor PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_clone_string_tests tests/vista_widget_clone_string_tests.cpp
    src/ui/widgets/vista_widget_clone_string.cpp)
  target_include_directories(vista_widget_clone_string_tests PRIVATE include)
  target_link_libraries(vista_widget_clone_string_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_clone_string COMMAND vista_widget_clone_string_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_clone_string_original.json)
  set_tests_properties(original_vista_widget_clone_string PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_constructor_tests tests/vista_widget_constructor_tests.cpp
    src/ui/widgets/vista_widget_constructor.cpp src/ui/widgets/vista_widget_activation.cpp)
  target_include_directories(vista_widget_constructor_tests PRIVATE include)
  target_link_libraries(vista_widget_constructor_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_constructor COMMAND vista_widget_constructor_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_constructor_original.json)
  set_tests_properties(original_vista_widget_constructor PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_text_clone_tests tests/vista_widget_text_clone_tests.cpp
    src/ui/widgets/vista_widget_text_clone.cpp)
  target_include_directories(vista_widget_text_clone_tests PRIVATE include)
  target_link_libraries(vista_widget_text_clone_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_text_clone COMMAND vista_widget_text_clone_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_text_clone_original.json)
  set_tests_properties(original_vista_widget_text_clone PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_grid_identifiers_tests tests/vista_widget_grid_identifiers_tests.cpp
    src/ui/widgets/vista_widget_grid_identifiers.cpp src/ui/vista/vista_lobby_initializer.cpp src/ui/vista/vista_lobby_layout.cpp)
  target_include_directories(vista_widget_grid_identifiers_tests PRIVATE include)
  target_link_libraries(vista_widget_grid_identifiers_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_grid_identifiers COMMAND vista_widget_grid_identifiers_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_grid_identifiers_original.json)
  set_tests_properties(original_vista_widget_grid_identifiers PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_registration_tests tests/vista_widget_registration_tests.cpp
    src/ui/widgets/vista_widget_registration.cpp)
  target_include_directories(vista_widget_registration_tests PRIVATE include)
  target_link_libraries(vista_widget_registration_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_registration COMMAND vista_widget_registration_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_registration_original.json)
  set_tests_properties(original_vista_widget_registration PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_lobby_initializer_tests tests/vista_lobby_initializer_tests.cpp
    src/ui/vista/vista_lobby_initializer.cpp src/ui/vista/vista_lobby_layout.cpp)
  target_include_directories(vista_lobby_initializer_tests PRIVATE include)
  target_link_libraries(vista_lobby_initializer_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_lobby_initializer COMMAND vista_lobby_initializer_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_initializer_original.json)
  set_tests_properties(original_vista_lobby_initializer PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_widget_activation_tests tests/vista_widget_activation_tests.cpp
    src/ui/widgets/vista_widget_activation.cpp)
  target_include_directories(vista_widget_activation_tests PRIVATE include)
  target_link_libraries(vista_widget_activation_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_widget_activation COMMAND vista_widget_activation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_widget_activation_original.json)
  set_tests_properties(original_vista_widget_activation PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_lobby_constructor_tests tests/vista_lobby_constructor_tests.cpp
    src/ui/vista/vista_lobby_constructor.cpp)
  target_include_directories(vista_lobby_constructor_tests PRIVATE include)
  target_link_libraries(vista_lobby_constructor_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_lobby_constructor COMMAND vista_lobby_constructor_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_constructor_original.json)
  set_tests_properties(original_vista_lobby_constructor PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_lobby_sort_tests tests/vista_lobby_sort_tests.cpp
    src/ui/vista/vista_lobby_sort.cpp)
  target_include_directories(vista_lobby_sort_tests PRIVATE include)
  target_link_libraries(vista_lobby_sort_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_lobby_sort COMMAND vista_lobby_sort_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_sort_original.json)
  set_tests_properties(original_vista_lobby_sort PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(vista_lobby_update_tests tests/vista_lobby_update_tests.cpp
    src/ui/vista/vista_lobby_update.cpp)
  target_include_directories(vista_lobby_update_tests PRIVATE include)
  target_link_libraries(vista_lobby_update_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_lobby_update COMMAND vista_lobby_update_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_update_original.json)
  set_tests_properties(original_vista_lobby_update PROPERTIES SKIP_RETURN_CODE 77)

  add_executable(frontend_vista_lobby_metadata_tests tests/frontend_vista_lobby_metadata_tests.cpp)
  target_link_libraries(frontend_vista_lobby_metadata_tests PRIVATE
    halo2_cache nlohmann_json::nlohmann_json OpenSSL::Crypto)
  add_test(NAME retail_frontend_vista_lobby_metadata COMMAND frontend_vista_lobby_metadata_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map"
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_metadata_original.json)
  set_tests_properties(retail_frontend_vista_lobby_metadata PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Original PC23 Setup constructor, details and publication owners.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_lobby_setup.cpp)
if(BUILD_TESTING)
  add_executable(vista_lobby_setup_tests tests/vista_lobby_setup_tests.cpp)
  target_link_libraries(vista_lobby_setup_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_lobby_setup COMMAND vista_lobby_setup_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_setup_original.json)
  set_tests_properties(original_vista_lobby_setup PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Actual PC14/23 and PC17/206 authored assets; dynamic owners are separate.
if(BUILD_TESTING)
  add_executable(frontend_vista_lobby_assets_tests tests/frontend_vista_lobby_assets_tests.cpp)
  target_link_libraries(frontend_vista_lobby_assets_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json OpenSSL::Crypto)
  add_test(NAME retail_frontend_vista_lobby_assets COMMAND frontend_vista_lobby_assets_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map"
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_assets.json)
  set_tests_properties(retail_frontend_vista_lobby_assets PROPERTIES SKIP_RETURN_CODE 77)
  add_test(NAME retail_frontend_vista_selector_assets COMMAND frontend_vista_lobby_assets_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/mainmenu.map"
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_selector_assets.json)
  set_tests_properties(retail_frontend_vista_selector_assets PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Original PC14 countdown button routes and cancellation request chronology.
if(BUILD_TESTING)
  add_executable(vista_lobby_input_tests tests/vista_lobby_input_tests.cpp src/ui/vista/vista_lobby_input.cpp)
  target_include_directories(vista_lobby_input_tests PRIVATE include)
  target_link_libraries(vista_lobby_input_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_lobby_input COMMAND vista_lobby_input_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_input_original.json)
  set_tests_properties(original_vista_lobby_input PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vista_lobby_bank_tests tests/vista_lobby_bank_tests.cpp src/ui/vista/vista_lobby_input.cpp)
  target_include_directories(vista_lobby_bank_tests PRIVATE include)
  target_link_libraries(vista_lobby_bank_tests PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
  add_test(NAME original_vista_lobby_bank COMMAND vista_lobby_bank_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_bank_original.json)
  set_tests_properties(original_vista_lobby_bank PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Actual sole-local session disposal, browser reset and PC14 Leave task owner.
if(BUILD_TESTING)
  add_executable(vista_session_disposal_tests tests/vista_session_disposal_tests.cpp
    src/ui/vista/vista_session_disposal.cpp src/ui/vista/vista_session_start_transition.cpp
    src/ui/vista/vista_session_transition.cpp src/ui/vista/vista_session_commit.cpp src/ui/vista/vista_session_phase.cpp
    src/ui/vista/vista_session_player_preparation.cpp src/ui/vista/vista_lifecycle.cpp)
  target_include_directories(vista_session_disposal_tests PRIVATE include)
  target_link_libraries(vista_session_disposal_tests PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
  add_test(NAME original_vista_session_disposal COMMAND vista_session_disposal_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_session_disposal_original.json)
  set_tests_properties(original_vista_session_disposal PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vista_lobby_leave_return_tests tests/vista_lobby_leave_return_tests.cpp
    src/ui/vista/vista_lobby_leave_return.cpp src/ui/vista/vista_lobby_input.cpp)
  target_include_directories(vista_lobby_leave_return_tests PRIVATE include)
  target_link_libraries(vista_lobby_leave_return_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_lobby_leave_return COMMAND vista_lobby_leave_return_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_leave_return_original.json)
  set_tests_properties(original_vista_lobby_leave_return PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vista_lobby_leave_task_tests tests/vista_lobby_leave_task_tests.cpp
    src/ui/vista/vista_lobby_leave_task.cpp src/ui/vista/vista_session_disposal.cpp
    src/ui/vista/vista_session_start_transition.cpp src/ui/vista/vista_session_transition.cpp
    src/ui/vista/vista_session_commit.cpp src/ui/vista/vista_session_phase.cpp
    src/ui/vista/vista_session_player_preparation.cpp src/ui/vista/vista_lifecycle.cpp)
  target_include_directories(vista_lobby_leave_task_tests PRIVATE include)
  target_link_libraries(vista_lobby_leave_task_tests PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
  add_test(NAME original_vista_lobby_leave_task COMMAND vista_lobby_leave_task_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_lobby_leave_task_original.json)
  set_tests_properties(original_vista_lobby_leave_task PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vista_network_browser_tests tests/vista_network_browser_tests.cpp src/ui/vista/vista_network_browser.cpp)
  target_include_directories(vista_network_browser_tests PRIVATE include)
  target_link_libraries(vista_network_browser_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_network_browser COMMAND vista_network_browser_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_network_browser_original.json)
  set_tests_properties(original_vista_network_browser PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vista_browser_preflight_tests tests/vista_browser_preflight_tests.cpp src/ui/vista/vista_network_browser.cpp)
  target_include_directories(vista_browser_preflight_tests PRIVATE include)
  target_link_libraries(vista_browser_preflight_tests PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
  add_test(NAME original_vista_browser_preflight COMMAND vista_browser_preflight_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_browser_preflight_original.json)
  set_tests_properties(original_vista_browser_preflight PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Actual live-PCR query leaves and original CRT formatting, isolated from the
# native world/resource provider portions of the same translation unit.
if(BUILD_TESTING)
  add_executable(native_vista_pcr_tests tests/native_vista_pcr_tests.cpp src/ui/vista/native_vista_pcr.cpp)
  target_include_directories(native_vista_pcr_tests PRIVATE include)
  target_link_libraries(native_vista_pcr_tests PRIVATE nlohmann_json::nlohmann_json SDL2::SDL2)
  if(MSVC)
    target_link_libraries(native_vista_pcr_tests PRIVATE halo2_systems)
  else()
    target_compile_options(native_vista_pcr_tests PRIVATE -ffunction-sections -fdata-sections)
    if(APPLE)
      target_link_options(native_vista_pcr_tests PRIVATE -Wl,-dead_strip)
    else()
      target_link_options(native_vista_pcr_tests PRIVATE -Wl,--gc-sections)
    endif()
  endif()
  add_test(NAME original_native_vista_pcr_queries COMMAND native_vista_pcr_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/native_vista_pcr_original.json)
endif()

if(MSVC)
    set_source_files_properties(src/gameplay/units/biped_death_runtime.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/biped_death_runtime.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()

if(BUILD_TESTING)
    add_executable(biped_death_runtime_tests tests/biped_death_runtime_tests.cpp)
    target_link_libraries(biped_death_runtime_tests PRIVATE halo2_systems)
    add_test(NAME original_biped_death_runtime COMMAND biped_death_runtime_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_death_runtime_original.tsv)
    set_tests_properties(original_biped_death_runtime PROPERTIES SKIP_RETURN_CODE 77)
endif()

if(BUILD_TESTING)
    add_test(NAME original_natural_ragdoll_construction COMMAND ragdoll_owners_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/ragdoll_natural_original.tsv)
    set_tests_properties(original_natural_ragdoll_construction PROPERTIES SKIP_RETURN_CODE 77)
endif()

if(BUILD_TESTING)
    add_executable(physics_model_initialization_tests tests/physics_model_initialization_tests.cpp)
    target_link_libraries(physics_model_initialization_tests PRIVATE halo2_systems)
    add_test(NAME original_spartan_model_initialization COMMAND physics_model_initialization_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/spartan_model_initialization_original.tsv)
    set_tests_properties(original_spartan_model_initialization PROPERTIES SKIP_RETURN_CODE 77)
endif()

target_sources(halo2_systems PRIVATE src/gameplay/units/biped_ragdoll_admission.cpp src/gameplay/units/biped_contact_impulse.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_body_placement.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_death_transition.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/biped_physics_activation.cpp)
if(MSVC)
    set_source_files_properties(src/gameplay/units/biped_death_transition.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/biped_death_transition.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(BUILD_TESTING)
    add_executable(biped_body_placement_tests tests/biped_body_placement_tests.cpp)
    target_link_libraries(biped_body_placement_tests PRIVATE halo2_systems)
    add_test(NAME original_biped_body_placement COMMAND biped_body_placement_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_body_placement_original.tsv)
    add_test(NAME original_biped_body_placement_natural COMMAND biped_body_placement_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_body_placement_natural_original.tsv)
    add_test(NAME original_biped_body_velocity COMMAND biped_body_placement_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_body_velocity_original.tsv)
    set_tests_properties(original_biped_body_velocity PROPERTIES SKIP_RETURN_CODE 77)
    set_tests_properties(original_biped_body_placement original_biped_body_placement_natural PROPERTIES SKIP_RETURN_CODE 77)
endif()
if(MSVC)
    set_source_files_properties(src/gameplay/units/biped_body_placement.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/biped_body_placement.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(MSVC)
    set_source_files_properties(src/gameplay/units/biped_ragdoll_admission.cpp src/gameplay/units/biped_contact_impulse.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/gameplay/units/biped_ragdoll_admission.cpp src/gameplay/units/biped_contact_impulse.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING)
    add_executable(biped_admission_tests tests/biped_admission_tests.cpp)
    target_link_libraries(biped_admission_tests PRIVATE halo2_systems)
    add_executable(biped_contact_impulse_tests tests/biped_contact_impulse_tests.cpp)
    target_link_libraries(biped_contact_impulse_tests PRIVATE halo2_systems)
    add_test(NAME original_biped_contact_impulse COMMAND biped_contact_impulse_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_contact_impulse_original.tsv)
    set_tests_properties(original_biped_contact_impulse PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_biped_ragdoll_admission COMMAND biped_admission_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_admission_original.tsv)
    add_test(NAME original_biped_death_transition COMMAND biped_admission_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_death_transition_original.tsv)
    add_test(NAME original_biped_physical_construction COMMAND biped_admission_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/biped_physical_construction_original.tsv)
    set_tests_properties(original_biped_physical_construction PROPERTIES SKIP_RETURN_CODE 77)
    set_tests_properties(original_biped_ragdoll_admission PROPERTIES SKIP_RETURN_CODE 77)
    set_tests_properties(original_biped_death_transition PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Original Vista synchronous session map-change builder and identity publication.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_session_map_change.cpp)
if(BUILD_TESTING)
  add_executable(vista_session_map_change_tests tests/vista_session_map_change_tests.cpp
    src/ui/vista/vista_session_map_change.cpp src/gameplay/multiplayer/source_game_variant.cpp)
  target_include_directories(vista_session_map_change_tests PRIVATE include)
  target_link_libraries(vista_session_map_change_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_session_map_change COMMAND vista_session_map_change_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_session_map_change_original.jsonl)
endif()

target_sources(halo2_systems PRIVATE src/rendering/source_point_collision.cpp)
if(MSVC)
    set_source_files_properties(src/rendering/source_point_collision.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
    set_source_files_properties(src/rendering/source_point_collision.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(BUILD_TESTING)
    add_executable(source_point_collision_tests tests/source_point_collision_tests.cpp)
    target_link_libraries(source_point_collision_tests PRIVATE halo2_systems)
    add_test(NAME original_death_point_collision COMMAND source_point_collision_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/death_point_collision_original.tsv)
    add_test(NAME original_instance_point_collision COMMAND source_point_collision_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/instance_point_collision_original.tsv)
    set_tests_properties(original_death_point_collision original_instance_point_collision PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Original Vista achievement event admission for a real offline local session.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_pcr_achievement.cpp)
if(BUILD_TESTING)
  add_executable(vista_pcr_achievement_tests tests/vista_pcr_achievement_tests.cpp
    src/ui/vista/vista_pcr_achievement.cpp)
  target_include_directories(vista_pcr_achievement_tests PRIVATE include)
  target_link_libraries(vista_pcr_achievement_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_pcr_achievement COMMAND vista_pcr_achievement_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_pcr_achievement_original.jsonl)
endif()

# Original game-engine gameplay black-quad projection and invocation timing.
if(BUILD_TESTING)
  add_executable(vista_network_browser_back_tests tests/vista_network_browser_back_tests.cpp
    src/ui/vista/vista_network_browser_back.cpp src/ui/vista/vista_profile_catalog.cpp)
  target_include_directories(vista_network_browser_back_tests PRIVATE include)
  target_link_libraries(vista_network_browser_back_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_network_browser_back COMMAND vista_network_browser_back_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_network_browser_back_original.json)
  set_tests_properties(original_vista_network_browser_back PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Original game-engine gameplay black-quad projection and invocation timing.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_gameplay_fade.cpp)
if(MSVC)
  set_source_files_properties(src/ui/vista/vista_gameplay_fade.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
  set_source_files_properties(src/ui/vista/vista_gameplay_fade.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(BUILD_TESTING)
  add_executable(vista_gameplay_fade_tests tests/vista_gameplay_fade_tests.cpp
    src/ui/vista/vista_gameplay_fade.cpp src/gameplay/units/source_player_lives.cpp)
  target_include_directories(vista_gameplay_fade_tests PRIVATE include)
  target_link_libraries(vista_gameplay_fade_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_gameplay_fade COMMAND vista_gameplay_fade_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_gameplay_fade_original.jsonl
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_gameplay_fade_owners_original.json)
endif()

# Original Vista local event/response/recent-hash join and owned PC assets.
target_sources(halo2_systems PRIVATE src/audio/vista_announcer_events.cpp src/audio/vista_announcer_assets.cpp)
if(MSVC)
  set_source_files_properties(src/audio/vista_announcer_events.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
  set_source_files_properties(src/audio/vista_announcer_events.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(BUILD_TESTING)
  add_executable(vista_announcer_events_tests tests/vista_announcer_events_tests.cpp
    src/audio/vista_announcer_events.cpp src/audio/vista_announcer_assets.cpp
    src/audio/vista_announcer_queue.cpp src/assets/source_tag_metadata_store.cpp)
  target_include_directories(vista_announcer_events_tests PRIVATE include)
  target_link_libraries(vista_announcer_events_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_announcer_events COMMAND vista_announcer_events_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_announcer_events_original.json
    "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps/ascension.map"
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_announcer_publication_original.json)
endif()

if(BUILD_TESTING)
  add_executable(king_presentation_tests tests/king_presentation_tests.cpp
    src/audio/vista_announcer_events.cpp src/audio/vista_announcer_queue.cpp src/gameplay/multiplayer/source_king.cpp)
  target_include_directories(king_presentation_tests PRIVATE include)
  target_link_libraries(king_presentation_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
  add_test(NAME original_king_presentation COMMAND king_presentation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/king_presentation_original.json
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/king_event_producers_original.json)
endif()

# Explicit PC shared-sound owner and native voice/completion join.
target_sources(halo2_systems PRIVATE src/ui/vista/native_vista_announcer.cpp)
if(BUILD_TESTING)
  add_executable(native_vista_announcer_tests tests/native_vista_announcer_tests.cpp)
  target_link_libraries(native_vista_announcer_tests PRIVATE halo2_systems)
  add_executable(score_event_audio_tests tests/score_event_audio_tests.cpp)
  target_link_libraries(score_event_audio_tests PRIVATE halo2_systems)
  add_test(NAME native_score_event_audio COMMAND score_event_audio_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps"
    "scenarios\\multi\\ascension\\ascension"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps"
    "$ENV{HOME}/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe")
  set_tests_properties(native_score_event_audio PROPERTIES ENVIRONMENT "SDL_AUDIODRIVER=dummy")
  add_test(NAME native_vista_announcer COMMAND native_vista_announcer_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps"
    "scenarios\\multi\\ascension\\ascension"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/king_presentation_original.json")
  set_tests_properties(native_vista_announcer PROPERTIES SKIP_RETURN_CODE 77 ENVIRONMENT "SDL_AUDIODRIVER=dummy")
endif()

# Original player birth, local simulation identity and separate PC/Xbox spawn tails.
target_sources(halo2_systems PRIVATE src/ui/vista/vista_player_birth.cpp)
if(BUILD_TESTING)
  add_executable(vista_player_birth_tests tests/vista_player_birth_tests.cpp
    src/ui/vista/vista_player_birth.cpp src/ui/vista/vista_slayer_publication.cpp)
  target_include_directories(vista_player_birth_tests PRIVATE include)
  target_link_libraries(vista_player_birth_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_player_birth COMMAND vista_player_birth_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_player_birth_original.json
    ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/vista-session-flow-audit/player-birth-original)
  add_executable(native_vista_player_birth_tests tests/native_vista_player_birth_tests.cpp)
  target_link_libraries(native_vista_player_birth_tests PRIVATE halo2_systems)
  add_test(NAME native_vista_player_birth COMMAND native_vista_player_birth_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
    "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps"
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/default.xbe)
endif()

include(cmake/PhysicsSolver.cmake)
include(cmake/PhysicsContactRuntime.cmake)
include(cmake/VehicleControl.cmake)
include(cmake/PhysicsVehicleRuntime.cmake)
include(cmake/PhysicsVehicleUpdate.cmake)
include(cmake/PhysicsVehicleMotion.cmake)
include(cmake/PhysicsVehicleMotionMath.cmake)
include(cmake/PhysicsVehicleContactState.cmake)
include(cmake/PhysicsVehicleConstraintCleanup.cmake)
include(cmake/PhysicsVehicleEnvironmentState.cmake)
include(cmake/PhysicsPhantomForceTu15.cmake)

# Original Vista loading owners and measured DirectX texture allocations.
target_sources(halo2_cache PRIVATE src/ui/vista/vista_loading_assets.cpp)
target_sources(halo2_systems PRIVATE src/ui/vista/vista_loading_music.cpp src/ui/vista/vista_loading_geometry.cpp
  src/ui/vista/vista_loading_audio.cpp src/ui/vista/native_vista_loading_audio.cpp
  src/platform/media/native_loading_presentation.cpp src/ui/vista/vista_mainmenu_network.cpp src/ui/vista/vista_loading_thread.cpp
  src/ui/vista/vista_ui_initial_admission.cpp src/ui/vista/vista_mainmenu_platform_gate.cpp)
if(NOT MSVC)
  set_source_files_properties(src/ui/vista/vista_loading_music.cpp src/ui/vista/vista_loading_geometry.cpp src/ui/vista/vista_loading_audio.cpp PROPERTIES
    COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off;-frounding-math")
endif()
if(BUILD_TESTING)
  add_executable(vista_loading_assets_tests tests/vista_loading_assets_tests.cpp)
  target_link_libraries(vista_loading_assets_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_loading_assets COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_selected_vista_loading_test.py
    assets $<TARGET_FILE:vista_loading_assets_tests>
    ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/vista-session-flow-audit/native-loading-presentation-original/d3dx-readback-20261011)
  set_tests_properties(original_vista_loading_assets PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vista_loading_music_tests tests/vista_loading_music_tests.cpp src/ui/vista/vista_loading_music.cpp)
  target_include_directories(vista_loading_music_tests PRIVATE include)
  target_link_libraries(vista_loading_music_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_loading_music COMMAND vista_loading_music_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_loading_music_original.json)
  add_executable(vista_loading_audio_tests tests/vista_loading_audio_tests.cpp)
  target_link_libraries(vista_loading_audio_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_loading_audio COMMAND vista_loading_audio_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/vista-session-flow-audit/music-binding-20261011/loading-gain-consumer/execution.json
    ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/vista-session-flow-audit/music-binding-20261011/loading-gain-consumer/loading-tone-7f4bb8.xbox-adpcm
    ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/vista-session-flow-audit/music-binding-20261011/loading-gain-consumer/loading-tone-49a688.s16le-mono)
  add_executable(native_vista_loading_audio_tests tests/native_vista_loading_audio_tests.cpp)
  target_link_libraries(native_vista_loading_audio_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json OpenSSL::Crypto)
  add_test(NAME native_vista_loading_audio COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_selected_vista_loading_test.py
    audio $<TARGET_FILE:native_vista_loading_audio_tests>
    ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/vista-session-flow-audit/music-binding-20261011/loading-gain-consumer/execution.json
    ${CMAKE_CURRENT_SOURCE_DIR}/analysis/frontend/vista-session-flow-audit/music-binding-20261011/loading-gain-consumer/loading-tone-49a688.s16le-mono)
  set_tests_properties(native_vista_loading_audio PROPERTIES SKIP_RETURN_CODE 77)
  # Fresh-clone control coverage uses only the approved bundled tone and small
  # instruction-execution results, with no original installation or full engine build.
  add_executable(bundled_loading_ambience_tests tests/native_vista_loading_audio_tests.cpp
    src/ui/vista/vista_loading_audio.cpp src/ui/vista/native_vista_loading_audio.cpp)
  target_include_directories(bundled_loading_ambience_tests PRIVATE include)
  target_link_libraries(bundled_loading_ambience_tests PRIVATE SDL2::SDL2
    nlohmann_json::nlohmann_json OpenSSL::Crypto)
  add_test(NAME bundled_loading_ambience COMMAND bundled_loading_ambience_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/bundled_loading_ambience_original.json
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/audio/loading-ambience/loading-tone.xbox-adpcm
    sha256:b740acf4ce447da6583b4f6c7fd6696fabbb249664a53fb780b18c4dd58cd73d)
  set_tests_properties(bundled_loading_ambience PROPERTIES TIMEOUT 60)
  add_executable(vista_loading_geometry_tests tests/vista_loading_geometry_tests.cpp)
  target_link_libraries(vista_loading_geometry_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_loading_geometry COMMAND vista_loading_geometry_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_loading_geometry_original.json)
  add_executable(native_loading_presentation_tests tests/native_loading_presentation_tests.cpp)
  target_link_libraries(native_loading_presentation_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME native_vista_loading_adapter COMMAND native_loading_presentation_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_loading_geometry_original.json)
  add_executable(native_loading_presentation_capture tests/native_loading_presentation_capture.cpp)
  target_link_libraries(native_loading_presentation_capture PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan)
  add_dependencies(native_loading_presentation_capture halo2_shaders)
  add_executable(capture_vista_loading_video tools/capture_vista_loading_video.cpp)
  target_link_libraries(capture_vista_loading_video PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan)
  add_dependencies(capture_vista_loading_video halo2_shaders)
  add_executable(desktop_loading_bridge_gpu_tests tests/desktop_loading_bridge_gpu_tests.cpp)
  target_link_libraries(desktop_loading_bridge_gpu_tests PRIVATE halo2_renderer)
  add_dependencies(desktop_loading_bridge_gpu_tests halo2_shaders)
  add_executable(frontend_audio_session_tests tests/frontend_audio_session_tests.cpp)
  target_link_libraries(frontend_audio_session_tests PRIVATE halo2_systems)
  add_test(NAME native_frontend_audio_session COMMAND frontend_audio_session_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/Halo 2 Vista installer/maps")
  set_tests_properties(native_frontend_audio_session PROPERTIES SKIP_RETURN_CODE 77)
  add_executable(vista_mainmenu_network_tests tests/vista_mainmenu_network_tests.cpp)
  target_link_libraries(vista_mainmenu_network_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_mainmenu_network COMMAND vista_mainmenu_network_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_mainmenu_network_original.json)
  add_executable(vista_mainmenu_game_state_tests tests/vista_mainmenu_game_state_tests.cpp)
  target_link_libraries(vista_mainmenu_game_state_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_mainmenu_game_state COMMAND vista_mainmenu_game_state_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_mainmenu_game_state_original.json)
  add_executable(vista_loading_thread_tests tests/vista_loading_thread_tests.cpp)
  target_link_libraries(vista_loading_thread_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_loading_thread COMMAND vista_loading_thread_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_loading_thread_original.json)
  add_executable(vista_ui_initial_admission_tests tests/vista_ui_initial_admission_tests.cpp)
  target_link_libraries(vista_ui_initial_admission_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_ui_initial_admission COMMAND vista_ui_initial_admission_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_ui_initial_admission_original.json)
  add_executable(vista_mainmenu_platform_gate_tests tests/vista_mainmenu_platform_gate_tests.cpp
    src/ui/vista/vista_mainmenu_platform_gate.cpp)
  target_include_directories(vista_mainmenu_platform_gate_tests PRIVATE include)
  target_link_libraries(vista_mainmenu_platform_gate_tests PRIVATE nlohmann_json::nlohmann_json)
  add_test(NAME original_vista_mainmenu_platform_gate COMMAND vista_mainmenu_platform_gate_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/vista_mainmenu_platform_gate_original.json)
endif()

add_executable(shader_constant_compare_tests tests/shader_constant_compare_tests.cpp)
add_test(NAME shader_constant_float_equality COMMAND shader_constant_compare_tests)

add_executable(weapon_actions_tests tests/weapon_actions_tests.cpp)
target_link_libraries(weapon_actions_tests PRIVATE halo2_systems)
if(BUILD_TESTING)
  add_test(NAME native_weapon_actions COMMAND weapon_actions_tests ${CMAKE_SOURCE_DIR}/assets/disc/maps/lockout.map)
  set_tests_properties(native_weapon_actions PROPERTIES SKIP_RETURN_CODE 77)
endif()
add_executable(native_weapon_gameplay_tests tests/native_weapon_gameplay_tests.cpp)
target_sources(halo2_systems PRIVATE src/gameplay/units/player_action_edges.cpp)
target_sources(halo2_cache PRIVATE src/gameplay/animation/first_person_dual.cpp)
if(MSVC)
  set_source_files_properties(src/gameplay/animation/first_person_dual.cpp src/gameplay/units/player_action_edges.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
  set_source_files_properties(src/gameplay/animation/first_person_dual.cpp src/gameplay/units/player_action_edges.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
add_executable(first_person_dual_tests tests/first_person_dual_tests.cpp)
target_link_libraries(first_person_dual_tests PRIVATE halo2_systems)
if(BUILD_TESTING)
  add_test(NAME original_first_person_dual COMMAND first_person_dual_tests
    ${CMAKE_SOURCE_DIR}/tests/fixtures/first_person_dual_original.tsv)
endif()
add_executable(player_action_edges_tests tests/player_action_edges_tests.cpp)
target_link_libraries(player_action_edges_tests PRIVATE halo2_systems)
if(BUILD_TESTING)
  add_test(NAME original_player_action_edges COMMAND player_action_edges_tests
    ${CMAKE_SOURCE_DIR}/tests/fixtures/player_action_edges_original.tsv)
endif()
target_link_libraries(native_weapon_gameplay_tests PRIVATE halo2_systems)
if(BUILD_TESTING)
  add_test(NAME native_weapon_gameplay COMMAND native_weapon_gameplay_tests)
endif()

add_executable(native_weapon_runtime_tests tests/native_weapon_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(native_weapon_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(native_weapon_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(native_weapon_runtime_tests halo2_shaders)
add_executable(hud_action_prompt_runtime_tests tests/hud_action_prompt_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(hud_action_prompt_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(hud_action_prompt_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(hud_action_prompt_runtime_tests halo2_shaders)
add_executable(campaign_action_prompt_runtime_tests tests/campaign_action_prompt_runtime_tests.cpp)
target_link_libraries(campaign_action_prompt_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(campaign_action_prompt_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(campaign_action_prompt_runtime_tests halo2_shaders)
add_executable(source_control_runtime_tests tests/source_control_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
add_executable(teleporter_runtime_tu15_tests EXCLUDE_FROM_ALL tests/teleporter_runtime_tu15_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(teleporter_runtime_tu15_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(teleporter_runtime_tu15_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(teleporter_runtime_tu15_tests halo2_shaders)
if(BUILD_TESTING)
    add_test(NAME native_mcc_teleporter_rendered COMMAND teleporter_runtime_tu15_tests
        "${HALO2_MCC_TELEPORTER_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/teleporter-runtime.json")
    set_tests_properties(native_mcc_teleporter_rendered PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
    add_test(NAME native_mcc_teleporter_blocked_rendered COMMAND teleporter_runtime_tu15_tests
        "${HALO2_MCC_TELEPORTER_MAP}" "${CMAKE_CURRENT_BINARY_DIR}/teleporter-blocked-runtime.json")
    set_tests_properties(native_mcc_teleporter_blocked_rendered PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240
        ENVIRONMENT "HALO2_TELEPORTER_BLOCKED_RENDERED=1")
endif()
add_executable(source_device_audio_runtime_tests tests/source_device_audio_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(source_device_audio_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(source_device_audio_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(source_device_audio_runtime_tests halo2_shaders)
target_link_libraries(source_control_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(source_control_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(source_control_runtime_tests halo2_shaders)
add_executable(native_covenant_weapon_runtime_tests tests/native_covenant_weapon_runtime_tests.cpp src/gameplay/multiplayer/multiplayer_runtime.cpp)
target_link_libraries(native_covenant_weapon_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(native_covenant_weapon_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(native_covenant_weapon_runtime_tests halo2_shaders)

add_executable(weapon_compound_tests tests/weapon_compound_tests.cpp)
target_link_libraries(weapon_compound_tests PRIVATE halo2_cache)
if(BUILD_TESTING)
  add_test(NAME original_weapon_compound_palette COMMAND weapon_compound_tests ${CMAKE_SOURCE_DIR}/tests/fixtures/weapon_compound_palette_original.tsv)
endif()

add_executable(frame_vertex_reuse_tests tests/frame_vertex_reuse_tests.cpp)
target_include_directories(frame_vertex_reuse_tests PRIVATE include src)
add_test(NAME native_frame_vertex_reuse COMMAND frame_vertex_reuse_tests)

add_executable(object_lighting_bindings_tests tests/object_lighting_bindings_tests.cpp)
target_include_directories(object_lighting_bindings_tests PRIVATE include src)
target_link_libraries(object_lighting_bindings_tests PRIVATE halo2_cache)
add_test(NAME native_object_lighting_binding_equivalence COMMAND object_lighting_bindings_tests)

add_executable(material_frame_selection_tests tests/material_frame_selection_tests.cpp)
target_include_directories(material_frame_selection_tests PRIVATE include src)
add_test(NAME native_material_frame_selection COMMAND material_frame_selection_tests)

target_compile_definitions(profile_mp_render_cpu PRIVATE HALO2_PROFILE_MATERIAL_FRAME_ORACLE=1)

add_executable(bsp_frame_plan_reuse_tests tests/bsp_frame_plan_reuse_tests.cpp)
target_link_libraries(bsp_frame_plan_reuse_tests PRIVATE halo2_cache)
if(BUILD_TESTING)
  add_test(NAME native_bsp_frame_plan_reuse COMMAND bsp_frame_plan_reuse_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps)
  set_tests_properties(native_bsp_frame_plan_reuse PROPERTIES SKIP_RETURN_CODE 77)
endif()

include(cmake/PhysicsPlatformLocks.cmake)
include(cmake/PhysicsPlatformCall.cmake)
include(cmake/PhysicsBspConstruction.cmake)
include(cmake/SourceProcessHeapRuntime.cmake)
include(cmake/BipedFallingDamage.cmake)
include(cmake/SourceVitalityRuntime.cmake)
include(cmake/SourceDirectDamageRuntime.cmake)
include(cmake/SourceDamageDeliveryRuntime.cmake)
include(cmake/SourceTu15StartupRecords.cmake)

add_executable(model_skin_simd_tests tests/model_skin_simd_tests.cpp)
target_include_directories(model_skin_simd_tests PRIVATE include src)
target_link_libraries(model_skin_simd_tests PRIVATE halo2_cache)
if(MSVC)
  set_source_files_properties(tests/model_skin_simd_tests.cpp PROPERTIES COMPILE_OPTIONS /fp:strict)
else()
  set_source_files_properties(tests/model_skin_simd_tests.cpp PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-ffp-contract=off")
endif()
if(BUILD_TESTING)
  add_test(NAME native_model_skin_simd COMMAND model_skin_simd_tests ${CMAKE_CURRENT_SOURCE_DIR})
endif()

include(cmake/PhysicsWorldConstruction.cmake)

include(cmake/PhysicsWorldSetupListener.cmake)

include(cmake/PhysicsWorldManager.cmake)

include(cmake/PhysicsWorldGameCallback.cmake)

include(cmake/PhysicsMultiplayerAcceptance.cmake)
include(cmake/PlayerCollisionTu15.cmake)
include(cmake/PlayerExternalForcesTu15.cmake)

include(cmake/PhysicsPhantomWorldTu15.cmake)

include(cmake/PhysicsGameFrame.cmake)

include(cmake/PhysicsWorldQwordTu15.cmake)

# Original1690FD ammunition overlay regression.
add_executable(needler_ammunition_tests tests/needler_ammunition_tests.cpp)
target_link_libraries(needler_ammunition_tests PRIVATE halo2_cache)
add_test(NAME retail_needler_ammunition_overlay COMMAND needler_ammunition_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/needler_ammunition_original.tsv)

add_test(NAME retail_weapon_spark_material_textures COMMAND particle_material_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map --compare
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_spark_materials.inputs.bin
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/weapon_spark_materials.original.bin)

# Preserve original293200/293350 ammunition interpolation operation order.
include(cmake/PhysicsPoolStartup.cmake)
include(cmake/PhysicsMainHeap.cmake)
include(cmake/SourceTu15Settings.cmake)
include(cmake/NativeTu15StartupPlatform.cmake)
include(cmake/NativeTu15StartupTimer.cmake)
include(cmake/SourceTu15Status.cmake)
include(cmake/SourceTu15Timer.cmake)
include(cmake/SourceTu15Interrupt.cmake)
include(cmake/PhysicsPhysicalStorage.cmake)
include(cmake/PhysicsGameArena.cmake)
include(cmake/NativeTu15StaticOwners.cmake)
include(cmake/NativeTu15GameStartup.cmake)
include(cmake/NativeTu15NetworkStorage.cmake)
include(cmake/NativeTu15NetworkLink.cmake)
include(cmake/NativeTu15NetworkSession.cmake)
include(cmake/NativeTu15CacheStartup.cmake)
include(cmake/NativeTu15InputStartup.cmake)
include(cmake/NativeTu15BinkMemoryStartup.cmake)
include(cmake/NativeTu15LevelHandlesStartup.cmake)
include(cmake/NativeTu15RendererStartup.cmake)
include(cmake/NativeTu15ClockStartup.cmake)
include(cmake/NativeTu15StateContextStartup.cmake)
include(cmake/NativeTu15PlayerStartup.cmake)
include(cmake/NativeTu15ViewStorage.cmake)
include(cmake/NativeTu15StaticRegistration.cmake)
include(cmake/NativeTu15ScalarState.cmake)
include(cmake/NativeTu15ObjectStorage.cmake)
include(cmake/NativeTu15AuxObjectState.cmake)
include(cmake/NativeTu15RendererRequest.cmake)
include(cmake/NativeTu15GraphicsDevice.cmake)
include(cmake/NativeTu15RendererState.cmake)
include(cmake/NativeTu15RendererPool.cmake)
include(cmake/NativeTu15RendererPalette.cmake)
include(cmake/NativeTu15RendererConstants.cmake)
include(cmake/NativeTu15RendererDepth.cmake)
include(cmake/NativeTu15RendererTexturePairs.cmake)
include(cmake/NativeTu15RendererTargetStorage.cmake)
include(cmake/NativeTu15RendererAuxTextures.cmake)
include(cmake/NativeTu15RendererTransparentStorage.cmake)
include(cmake/NativeTu15RendererDebugStorage.cmake)
include(cmake/NativeTu15RenderObjectStartup.cmake)
include(cmake/NativeTu15SoundStorage.cmake)
include(cmake/NativeTu15AudioPreference.cmake)
include(cmake/NativeTu15SoundSdkPrefix.cmake)
include(cmake/NativeTu15SoundResources.cmake)
include(cmake/NativeTu15SoundEpOutput.cmake)
include(cmake/NativeTu15SoundAc97Output.cmake)
include(cmake/NativeTu15SoundAc97ChannelReset.cmake)
include(cmake/NativeTu15SoundPcmBuffers.cmake)
include(cmake/NativeTu15SoundPcmPreparedRange.cmake)
include(cmake/NativeTu15SoundAc97PageRelease.cmake)
include(cmake/NativeTu15SoundFrontendEvents.cmake)
include(cmake/NativeTu15SoundFrontendDpc.cmake)
include(cmake/NativeTu15SoundEventPort.cmake)
include(cmake/NativeTu15SoundTimedNotifications.cmake)
include(cmake/NativeTu15SoundTableLimits.cmake)
include(cmake/NativeTu15SoundPioResources.cmake)
include(cmake/NativeTu15SoundConfigurationCpu.cmake)
include(cmake/NativeTu15SoundInterruptPolicy.cmake)
include(cmake/NativeTu15SoundCodecInitialization.cmake)
include(cmake/NativeTu15SoundCodecEvents.cmake)
include(cmake/NativeTu15SoundColdAnalogStart.cmake)
include(cmake/NativeTu15SoundMonoDistortion.cmake)
include(cmake/NativeTu15SoundGainFanout.cmake)
include(cmake/NativeTu15SoundTwoProductAccumulator.cmake)
include(cmake/NativeTu15SoundControllerArithmetic.cmake)
include(cmake/NativeTu15SoundChorus.cmake)
include(cmake/NativeTu15SoundCrosstalk.cmake)
include(cmake/NativeTu15SoundRingEffect.cmake)
include(cmake/NativeTu15SoundSharedWord24.cmake)
include(cmake/NativeTu15SoundGraphStorage.cmake)
include(cmake/NativeTu15SoundController.cmake)
include(cmake/NativeTu15SoundGpOutput.cmake)
include(cmake/NativeTu15SoundEpStereoNumeric.cmake)
include(cmake/NativeTu15SoundEpMonoNumeric.cmake)
include(cmake/NativeTu15SoundEpModule0Numeric.cmake)
include(cmake/NativeTu15SoundConfigurationFrontier.cmake)
include(cmake/MccSoundEffectDefinitions.cmake)
include(cmake/NativeTu15SoundMonoParameters.cmake)
include(cmake/MccEffectParameterFunction.cmake)
include(cmake/MccMonoFunctionBridge.cmake)
include(cmake/NativeTu15SoundRecord0Context.cmake)
include(cmake/NativeTu15UiScriptStartup.cmake)
include(cmake/NativeTu15FinalStartupStorage.cmake)
include(cmake/NativeTu15LateStartupValidation.cmake)
include(cmake/NativeTu15RendererConstructorTests.cmake)
include(cmake/PhysicsEarlyInitializer.cmake)
include(cmake/PhysicsRendererStorage.cmake)
include(cmake/PhysicsLiveWorldTu15.cmake)
target_sources(halo2_physics_world_runtime PRIVATE src/gameplay/objects/source_damage_receiver_tu15_generated.cpp)
include(cmake/VehicleBvAgentTu15.cmake)
include(cmake/CampaignAiPlacementSettings.cmake)
include(cmake/CampaignAiConditions.cmake)
include(cmake/CampaignAiWorkingMask.cmake)
include(cmake/CampaignAiChildDirectory.cmake)
include(cmake/CampaignAiScalarDefaults.cmake)
include(cmake/CampaignAiDescriptorCallbacks.cmake)
include(cmake/CampaignAiObjectDecisions.cmake)
include(cmake/CampaignSessionSeedOrder.cmake)
include(cmake/MultiplayerEnginesTu15.cmake)
include(cmake/MultiplayerContinuation.cmake)
include(cmake/AuthorityCodec.cmake)
include(cmake/MccMeleeContinuation.cmake)
include(cmake/PhysicsNativeFrameTu15.cmake)
include(cmake/SourceScenePhysics.cmake)
if(MSVC)
  set_property(SOURCE src/rendering/models/model_pose.cpp APPEND PROPERTY COMPILE_OPTIONS /fp:strict)
else()
  set_property(SOURCE src/rendering/models/model_pose.cpp APPEND PROPERTY COMPILE_OPTIONS -fno-fast-math -ffp-contract=off)
endif()

# Independent original Needler prepared-effect input/tick comparison.
if(BUILD_TESTING)
  add_executable(needler_firing_tests tests/needler_firing_tests.cpp)
  target_link_libraries(needler_firing_tests PRIVATE halo2_systems)
  if(MSVC)
    target_compile_options(needler_firing_tests PRIVATE /fp:strict)
  else()
    target_compile_options(needler_firing_tests PRIVATE -fno-fast-math -ffp-contract=off)
  endif()
  add_test(NAME original_needler_firing_effects COMMAND needler_firing_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/needler_firing_effects_original.tsv)
  set_tests_properties(original_needler_firing_effects PROPERTIES SKIP_RETURN_CODE 77)
endif()

add_executable(needler_projectile_birth_tests tests/needler_projectile_birth_tests.cpp)
target_link_libraries(needler_projectile_birth_tests PRIVATE halo2_systems)
add_test(NAME original_needler_projectile_birth COMMAND needler_projectile_birth_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/needler_projectile_birth_original.tsv)

# Complete original voice pause/envelope/common-stop CPU owners. Kernel, FIFO,
# pending retirement and parent-buffer boundaries remain mandatory services.
target_sources(halo2_cache PRIVATE src/audio/sound_sdk_voice_stop.cpp)
if(BUILD_TESTING)
  add_executable(sound_sdk_voice_stop_tests tests/sound_sdk_voice_stop_tests.cpp)
  target_link_libraries(sound_sdk_voice_stop_tests PRIVATE halo2_cache halo2_native_synchronization)
  add_test(NAME original_sound_sdk_voice_stop COMMAND sound_sdk_voice_stop_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/sound_sdk_voice_stop.original.json)
endif()

add_executable(campaign_weapon_prompt_runtime_tests tests/campaign_weapon_prompt_runtime_tests.cpp)
target_link_libraries(campaign_weapon_prompt_runtime_tests PRIVATE halo2_renderer SDL2::SDL2 Vulkan::Vulkan halo2_glm)
target_compile_definitions(campaign_weapon_prompt_runtime_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")
add_dependencies(campaign_weapon_prompt_runtime_tests halo2_shaders)

target_sources(halo2_systems PRIVATE src/gameplay/weapons/source_weapon_carry.cpp src/gameplay/units/native_unit_interaction.cpp)
add_executable(campaign_ally_trade_tests tests/campaign_ally_trade_tests.cpp)
target_link_libraries(campaign_ally_trade_tests PRIVATE halo2_systems halo2_cache)

target_sources(halo2_systems PRIVATE src/gameplay/vehicles/native_vehicle_interaction.cpp)
add_executable(vehicle_action_prompt_tests tests/vehicle_action_prompt_tests.cpp)
target_link_libraries(vehicle_action_prompt_tests PRIVATE halo2_systems halo2_cache)
add_executable(inspect_unit_prompt_strings tools/inspect_unit_prompt_strings.cpp)
target_link_libraries(inspect_unit_prompt_strings PRIVATE halo2_systems halo2_cache)

target_sources(halo2_cache PRIVATE src/gameplay/units/native_unit_interaction_assets.cpp)
add_executable(unit_action_prompt_runtime_tests tests/unit_action_prompt_runtime_tests.cpp)
target_link_libraries(unit_action_prompt_runtime_tests PRIVATE halo2_renderer halo2_systems halo2_cache SDL2::SDL2)

include(cmake/DeathCycle.cmake)
include(cmake/WeaponBarrelTiming.cmake)

add_executable(custom_slayer_tests tests/custom_slayer_tests.cpp)
target_link_libraries(custom_slayer_tests PRIVATE halo2_systems)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/releases/halo2-custom-slayer-v1.zip
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/bundle_custom_slayer.py
        --root ${CMAKE_CURRENT_SOURCE_DIR} --output ${CMAKE_CURRENT_BINARY_DIR}/releases/halo2-custom-slayer-v1.zip
    DEPENDS tools/bundle_custom_slayer.py tools/native_variant_assets.py data/tags/variants/fiesta-slayer-v1.json data/tags/variants/infection-slayer-v1.json data/tags/variants/bomb-defusal-v1.json data/tags/bomb-defusal-sites-v1.json)
add_custom_target(custom_slayer_bundle ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/releases/halo2-custom-slayer-v1.zip)
add_dependencies(halo2_native custom_slayer_bundle)
if(BUILD_TESTING)
    add_executable(bomb_defusal_tests tests/bomb_defusal_tests.cpp src/gameplay/multiplayer/custom_slayer.cpp)
    target_link_libraries(bomb_defusal_tests PRIVATE halo2_bomb_defusal nlohmann_json::nlohmann_json)
    target_compile_definitions(bomb_defusal_tests PRIVATE HALO2_VARIANT_TAG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/tags/variants")
    add_test(NAME bomb_defusal_rules COMMAND bomb_defusal_tests ${CMAKE_CURRENT_SOURCE_DIR})
    add_executable(bomb_defusal_match_tests tests/bomb_defusal_match_tests.cpp)
    target_link_libraries(bomb_defusal_match_tests PRIVATE halo2_systems)
    add_test(NAME bomb_defusal_match COMMAND bomb_defusal_match_tests)
    add_executable(bomb_defusal_map_tests tests/bomb_defusal_map_tests.cpp)
    target_link_libraries(bomb_defusal_map_tests PRIVATE halo2_systems)
    add_test(NAME bomb_defusal_maps COMMAND bomb_defusal_map_tests ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(bomb_defusal_maps PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
    add_executable(bomb_defusal_hud_tests tests/bomb_defusal_hud_tests.cpp)
    target_link_libraries(bomb_defusal_hud_tests PRIVATE halo2_renderer halo2_systems)
    add_test(NAME bomb_defusal_hud COMMAND bomb_defusal_hud_tests ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(bomb_defusal_hud PROPERTIES SKIP_RETURN_CODE 77)
    if(HALO2_LAUNCHER_XVFB)
        add_test(NAME bomb_defusal_hud_render COMMAND ${HALO2_LAUNCHER_XVFB} -a
            $<TARGET_FILE:bomb_defusal_hud_tests> ${CMAKE_CURRENT_SOURCE_DIR} --render)
        set_tests_properties(bomb_defusal_hud_render PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 90 RUN_SERIAL TRUE)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d/VkLayer_khronos_validation.json")
            set_tests_properties(bomb_defusal_hud_render PROPERTIES ENVIRONMENT
                "VK_LAYER_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/share/vulkan/explicit_layer.d;LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR}/.tools/validation/usr/lib:$ENV{LD_LIBRARY_PATH}")
        endif()
    endif()
    add_executable(inspect_bomb_defusal_maps tools/inspect_bomb_defusal_maps.cpp src/physics/collision/collision.cpp)
    target_link_libraries(inspect_bomb_defusal_maps PRIVATE halo2_cache)
endif()

# Native relay handoff controls. The complete-world digests in this transport
# fixture are declared inputs; this is not a full gameplay migration test.
if(BUILD_TESTING)
    add_executable(online_continuity_transport_tests tests/online_continuity_transport_tests.cpp)
    target_link_libraries(online_continuity_transport_tests PRIVATE halo2_online_matchmaking nlohmann_json::nlohmann_json)
    add_test(NAME online_continuity_native_transport COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_continuity_transport_http_test.py
        $<TARGET_FILE:online_continuity_transport_tests>)
    add_test(NAME online_continuity_legacy_transport COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/online_continuity_transport_http_test.py
        $<TARGET_FILE:online_continuity_transport_tests> legacy)
endif()

if(BUILD_TESTING)
    add_executable(source_effect_event_tests tests/source_effect_event_tests.cpp)
    target_include_directories(source_effect_event_tests PRIVATE include)
    target_link_libraries(source_effect_event_tests PRIVATE nlohmann_json::nlohmann_json)
    if(MSVC)
        target_compile_options(source_effect_event_tests PRIVATE /fp:strict)
    else()
        target_compile_options(source_effect_event_tests PRIVATE -fno-fast-math -ffp-contract=off)
    endif()
    add_test(NAME original_effect_event_timeline COMMAND source_effect_event_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/effect-events-20261012/cases.jsonl")
endif()

if(BUILD_TESTING)
    add_executable(physics_scenery_lightmap_lookup_tests tests/physics_scenery_lightmap_lookup_tests.cpp)
    target_include_directories(physics_scenery_lightmap_lookup_tests PRIVATE include)
    target_link_libraries(physics_scenery_lightmap_lookup_tests PRIVATE halo2_physics_world_runtime nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_scenery_lightmap_lookup COMMAND physics_scenery_lightmap_lookup_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/zanzibar-factory-20261013/cases.jsonl")
endif()

target_sources(halo2_systems PRIVATE src/runtime/source_map_effects.cpp)
target_sources(halo2_systems PRIVATE src/rendering/effects/effect_presentation_continuity.cpp src/rendering/effects/effect_audio_auxiliary.cpp)
target_sources(halo2_systems PRIVATE src/runtime/source_native_publication.cpp)
target_sources(halo2_systems PRIVATE src/audio/mcc_sound_variation_native_binding.cpp)
if(BUILD_TESTING)
    add_executable(effect_presentation_continuity_tests tests/effect_presentation_continuity_tests.cpp)
    target_link_libraries(effect_presentation_continuity_tests PRIVATE halo2_systems halo2_authority_codec)
    add_test(NAME native_effect_presentation_cut COMMAND effect_presentation_continuity_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps/turf.map")
    set_tests_properties(native_effect_presentation_cut PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 240)
endif()
if(BUILD_TESTING)
    add_executable(source_map_effects_tests tests/source_map_effects_tests.cpp)
    target_link_libraries(source_map_effects_tests PRIVATE halo2_systems)
    add_test(NAME native_map_effect_attachments COMMAND source_map_effects_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/map-pack/maps
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03)
endif()
target_sources(halo2_physics_world_runtime PRIVATE src/runtime/source_map_world_support_tu15_generated.cpp)
target_sources(halo2_systems PRIVATE src/rendering/effects/source_decal_vertex_cache.cpp)
if(BUILD_TESTING)
    add_executable(source_decal_vertex_cache_tests tests/source_decal_vertex_cache_tests.cpp)
    target_link_libraries(source_decal_vertex_cache_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_source_decal_vertex_cache COMMAND source_decal_vertex_cache_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/graphics-owner-20261013/cache-fixture.json)
    set_tests_properties(native_source_decal_vertex_cache PROPERTIES SKIP_RETURN_CODE 77)
    set(HALO2_TU15_GAMEPLAY_XBE "" CACHE FILEPATH "Owned original English TU1.5 gameplay executable for acceptance tests")
    add_test(NAME native_campaign_lens_restore_adoption_tu15 COMMAND campaign_lens_restore_runtime_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(native_campaign_lens_restore_adoption_tu15 PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE)
    add_test(NAME native_campaign_camera_outer_frame_tu15 COMMAND "${Python3_EXECUTABLE}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/tag_match_memory_budget.py
        --output-root @HALO2_TEST_SCRATCH@/camera-original --timeout 120 --
        $<TARGET_FILE:campaign_camera_frame_tu15_tests>
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/03b_newmombasa.map)
    set_tests_properties(native_campaign_camera_outer_frame_tu15 PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE)
    add_test(NAME native_campaign_camera_cold_outer_frame_tu15 COMMAND "${Python3_EXECUTABLE}"
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/tag_match_memory_budget.py
        --output-root @HALO2_TEST_SCRATCH@/camera-cold-original --timeout 120 --
        $<TARGET_FILE:campaign_camera_frame_tu15_tests>
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/03b_newmombasa.map --native-cold)
    set_tests_properties(native_campaign_camera_cold_outer_frame_tu15 PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE)
    add_test(NAME original_script_marker_effect_kernel_tu15 COMMAND source_script_effect_kernel_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/script_marker_effect_tu15.json "${HALO2_TU15_GAMEPLAY_XBE}")
    set_tests_properties(original_script_marker_effect_kernel_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_script_effect_allocation COMMAND campaign_script_effect_runtime_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(native_opening_script_effect_allocation PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_source_cinematic_controllers COMMAND campaign_cinematic_source_runtime_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(native_opening_source_cinematic_controllers PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_source_cinematic_camera COMMAND campaign_cinematic_camera_runtime_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(native_opening_source_cinematic_camera PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_campaign_point_camera_adoption COMMAND campaign_point_camera_adoption_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/03b_newmombasa.map)
    set_tests_properties(native_campaign_point_camera_adoption PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_campaign_screen_bank_tu15 COMMAND source_campaign_screen_bank_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/campaign_screen_bank_tu15.json "${HALO2_TU15_GAMEPLAY_XBE}")
    set_tests_properties(original_campaign_screen_bank_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_source_screen_bank COMMAND campaign_cinematic_screen_runtime_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(native_opening_source_screen_bank PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_source_animation_composition COMMAND campaign_cinematic_composition_runtime_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(native_opening_source_animation_composition PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_object_function_slots_tu15 COMMAND source_object_function_slots_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/object_function_slots_tu15.json "${HALO2_TU15_GAMEPLAY_XBE}")
    set_tests_properties(original_object_function_slots_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_source_shader_functions COMMAND campaign_cinematic_function_runtime_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(native_opening_source_shader_functions PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_unit_cinematic_emotion_tu15 COMMAND source_unit_emotion_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/unit_cinematic_emotion_tu15.json "${HALO2_TU15_GAMEPLAY_XBE}")
    set_tests_properties(original_unit_cinematic_emotion_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_source_unit_controls COMMAND campaign_unit_cinematic_runtime_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(native_opening_source_unit_controls PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_source_cloth_construction COMMAND campaign_cloth_construction_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(native_opening_source_cloth_construction PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_source_scene_retirement COMMAND campaign_scene_retirement_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(native_opening_source_scene_retirement PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_scripted_sound_points_tu15 COMMAND campaign_scripted_sound_point_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(native_opening_scripted_sound_points_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_cinematic_lighting_bridge_tu15 COMMAND campaign_cinematic_lighting_bridge_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" "${CMAKE_SOURCE_DIR}/assets/disc/maps/00a_introduction.map")
    set_tests_properties(native_opening_cinematic_lighting_bridge_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_scripted_audio_volumes COMMAND campaign_scripted_audio_volume_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(native_opening_scripted_audio_volumes PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_scripted_audio_emitters COMMAND campaign_scripted_audio_emitter_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(native_opening_scripted_audio_emitters PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_scripted_speech_tu15 COMMAND campaign_scripted_speech_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(native_opening_scripted_speech_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_scripted_speech_armorer_tu15 COMMAND campaign_scripted_speech_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map --armorer)
    set_tests_properties(native_opening_scripted_speech_armorer_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_scripted_speech_chief_tu15 COMMAND campaign_scripted_speech_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map --chief)
    set_tests_properties(native_opening_scripted_speech_chief_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_campaign_session_identity_tu15 COMMAND campaign_session_identity_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(native_opening_campaign_session_identity_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_scripted_audio_timeline COMMAND campaign_scripted_audio_timeline_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/opening_dialogue_timing_tu15.tsv)
    set_tests_properties(native_opening_scripted_audio_timeline PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_script_sound_admission_phase COMMAND script_sound_admission_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map native)
    add_test(NAME native_source_script_sound_admission_guards COMMAND script_sound_admission_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map source)
    set_tests_properties(native_script_sound_admission_phase native_source_script_sound_admission_guards
        PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE)
    add_test(NAME original_opening_scripted_audio_timing COMMAND campaign_scripted_audio_timing_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/opening_dialogue_timing_tu15.tsv)
    set_tests_properties(original_opening_scripted_audio_timing PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_opening_scripted_audio_gains COMMAND campaign_scripted_audio_gain_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/opening_audio_gains_tu15.tsv)
    set_tests_properties(original_opening_scripted_audio_gains PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_opening_camera_interpolation COMMAND campaign_camera_interpolation_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(native_opening_camera_interpolation PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_campaign_screen_interpolation COMMAND campaign_screen_interpolation_tests
        "${HALO2_TU15_GAMEPLAY_XBE}")
    set_tests_properties(native_campaign_screen_interpolation PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_opening_independent_materials COMMAND campaign_independent_material_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(original_opening_independent_materials PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_opening_cinematic_environment_materials COMMAND campaign_cinematic_environment_material_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(original_opening_cinematic_environment_materials PROPERTIES SKIP_RETURN_CODE 77)
    if(EXISTS "${HALO2_TU15_GAMEPLAY_XBE}" AND
            EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map")
        add_test(NAME original_reach_float3_model_positions COMMAND reach_model_float_positions_tests
            ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map "${HALO2_TU15_GAMEPLAY_XBE}"
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_reach_model_streams_tu15.json)
    endif()
    add_test(NAME original_opening_cinematic_matte_materials COMMAND campaign_cinematic_matte_material_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(original_opening_cinematic_matte_materials PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_campaign_lightmap_shadow_hosts_tu15 COMMAND campaign_lightmap_shadow_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map)
    set_tests_properties(native_campaign_lightmap_shadow_hosts_tu15 PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE)
    add_test(NAME original_campaign_dof_filter_recipe_tu15 COMMAND campaign_dof_recipe_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_cinematic_dof_filter_tu15.json)
    set_tests_properties(original_campaign_dof_filter_recipe_tu15 PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME native_campaign_authored_depth_of_field_tu15 COMMAND campaign_depth_of_field_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/retail_cinematic_dof_tu15.json)
    set_tests_properties(native_campaign_authored_depth_of_field_tu15 PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE)
    add_test(NAME native_campaign_authored_far_clip_tu15 COMMAND campaign_far_clip_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/06b_floodzone.map)
    set_tests_properties(native_campaign_authored_far_clip_tu15 PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE)
    add_test(NAME original_opening_cinematic_shadow_control COMMAND source_cinematic_shadow_control_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/cinematic_shadow_control_tu15.json "${HALO2_TU15_GAMEPLAY_XBE}")
    set_tests_properties(original_opening_cinematic_shadow_control PROPERTIES SKIP_RETURN_CODE 77)
    foreach(scene08_object RANGE 0 2)
        add_test(NAME native_opening_source_model_geometry_${scene08_object} COMMAND campaign_model_geometry_runtime_tu15_tests
            "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map ${scene08_object})
        set_tests_properties(native_opening_source_model_geometry_${scene08_object} PROPERTIES SKIP_RETURN_CODE 77)
    endforeach()
    add_test(NAME native_opening_complete_effect_graphs COMMAND campaign_effect_graph_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/00a_introduction.map
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps "${HALO2_TU15_GAMEPLAY_XBE}")
    set_tests_properties(native_opening_complete_effect_graphs PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_map_decal_lifecycle_tu15_tests tests/source_map_decal_lifecycle_tu15_tests.cpp)
    target_link_libraries(source_map_decal_lifecycle_tu15_tests PRIVATE halo2_systems)
    add_test(NAME original_tu15_map_decal_initialization COMMAND source_map_decal_lifecycle_tu15_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/zanzibar.map)
    set_tests_properties(original_tu15_map_decal_initialization PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_physical_byte_protection_tests tests/source_physical_byte_protection_tests.cpp)
    target_link_libraries(source_physical_byte_protection_tests PRIVATE halo2_systems)
    add_test(NAME native_source_physical_byte_ranges COMMAND source_physical_byte_protection_tests)
    set_tests_properties(native_source_physical_byte_ranges PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(source_map_world_support_tu15_tests tests/source_map_world_support_tu15_tests.cpp)
    target_link_libraries(source_map_world_support_tu15_tests PRIVATE halo2_physics_world_runtime nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_map_object_callback COMMAND source_map_world_support_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/map-world-support-20261013/cases.jsonl)
    add_executable(source_map_world_startup_tu15_tests tests/source_map_world_startup_tu15_tests.cpp)
    target_link_libraries(source_map_world_startup_tu15_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_map_world_startup COMMAND source_map_world_startup_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/map-world-support-20261013/startup-cases.jsonl)
    add_test(NAME original_tu15_map_light_shape COMMAND source_map_world_startup_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/map-world-support-20261013/cone-cases.jsonl 64)
    add_test(NAME original_tu15_map_light_vector COMMAND source_map_world_startup_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/map-world-support-20261013/vector-cases.jsonl 64)
    add_test(NAME original_tu15_map_physics_callback COMMAND source_map_world_startup_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/map-world-support-20261013/physics-callback-cases.jsonl 512)
    set_tests_properties(original_tu15_map_physics_callback PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_map_object_effect_reset COMMAND source_map_world_startup_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/map-world-support-20261013/effect-reset-cases.jsonl 256)
    set_tests_properties(original_tu15_map_object_effect_reset PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_terminal_live_callback COMMAND source_map_world_startup_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/terminal-continuation-20261014/callback-original.jsonl 1)
    set_tests_properties(original_tu15_terminal_live_callback PROPERTIES SKIP_RETURN_CODE 77)
    add_test(NAME original_tu15_map_event_strings COMMAND source_map_world_startup_tu15_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/decal-consumer-20261014/original-event-strings.jsonl 1038)
    set_tests_properties(original_tu15_map_event_strings PROPERTIES SKIP_RETURN_CODE 77)
    set_tests_properties(original_tu15_map_world_startup original_tu15_map_light_shape original_tu15_map_light_vector PROPERTIES SKIP_RETURN_CODE 77)
endif()

add_executable(object_light_direction_tu15_tests tests/object_light_direction_tu15_tests.cpp)
target_link_libraries(object_light_direction_tu15_tests PRIVATE halo2_cache)
add_test(NAME original_tu15_object_light_direction COMMAND object_light_direction_tu15_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_object_light_direction.original.json)

if(BUILD_TESTING)
    add_executable(collision_layers_tests tests/collision_layers_tests.cpp src/physics/collision/collision.cpp)
    target_include_directories(collision_layers_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    add_test(NAME native_collision_layers COMMAND collision_layers_tests)
    add_executable(tag_campaign_navigation_tests tests/tag_campaign_navigation_tests.cpp)
    target_link_libraries(tag_campaign_navigation_tests PRIVATE halo2_systems)
    add_test(NAME native_tag_campaign_navigation COMMAND tag_campaign_navigation_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/01a_tutorial.map)
    set_tests_properties(native_tag_campaign_navigation PROPERTIES SKIP_RETURN_CODE 77)
    add_executable(tag_campaign_sound_prediction_tests tests/tag_campaign_sound_prediction_tests.cpp)
    target_link_libraries(tag_campaign_sound_prediction_tests PRIVATE halo2_systems)
    add_test(NAME native_tag_campaign_sound_prediction COMMAND tag_campaign_sound_prediction_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/01b_spacestation.map)
    set_tests_properties(native_tag_campaign_sound_prediction PROPERTIES SKIP_RETURN_CODE 77)
    if(UNIX AND NOT APPLE)
        add_executable(tag_campaign_simulation_runner tools/tag_campaign_simulation_runner.cpp)
        target_link_libraries(tag_campaign_simulation_runner PRIVATE nlohmann_json::nlohmann_json OpenSSL::Crypto)
    endif()
endif()

add_executable(texture_filtering_tu15_tests tests/texture_filtering_tu15_tests.cpp)
target_link_libraries(texture_filtering_tu15_tests PRIVATE halo2_cache)
add_test(NAME original_tu15_texture_filtering COMMAND texture_filtering_tu15_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_texture_filtering.original.json)

if(BUILD_TESTING)
    add_executable(source_model_live_resources_tu15_tests tests/source_model_live_resources_tu15_tests.cpp)
    target_include_directories(source_model_live_resources_tu15_tests PRIVATE src)
    target_link_libraries(source_model_live_resources_tu15_tests PRIVATE halo2_systems halo2_menu_world_storage nlohmann_json::nlohmann_json)
    add_test(NAME native_tu15_model_resource_publication COMMAND source_model_live_resources_tu15_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map" "masterchief")
    set_tests_properties(native_tu15_model_resource_publication PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
    add_test(NAME native_tu15_live_model_builder COMMAND source_model_live_resources_tu15_tests
        "/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/lockout.map" "masterchief" "--build")
    set_tests_properties(native_tu15_live_model_builder PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
endif()

target_sources(halo2_physics_world_runtime PRIVATE src/rendering/models/source_model_render_tu15_generated.cpp)
target_sources(halo2_physics_world_runtime PRIVATE src/rendering/effects/source_first_person_light_pose_tu15_generated.cpp)
target_sources(halo2_systems PRIVATE src/rendering/models/source_live_model_draws_tu15.cpp)

if(BUILD_TESTING)
    add_executable(first_person_light_up_tu15_tests tests/first_person_light_up_tu15_tests.cpp
        src/runtime/source_runtime_memory.cpp src/runtime/source_runtime_host_storage.cpp
        src/runtime/source_runtime_host_virtual_memory.cpp)
    target_include_directories(first_person_light_up_tu15_tests PRIVATE src)
    target_link_libraries(first_person_light_up_tu15_tests PRIVATE
        halo2_physics_world_runtime halo2_cache halo2_snapshot_storage nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_first_person_light_up COMMAND first_person_light_up_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/first_person_light_up_tu15.json")
endif()

if(BUILD_TESTING)
    add_executable(source_object_light_admission_tu15_tests tests/source_object_light_admission_tu15_tests.cpp)
    target_link_libraries(source_object_light_admission_tu15_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
    add_test(NAME original_tu15_object_light_admission COMMAND source_object_light_admission_tu15_tests
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_object_light_admission.original.json")
    set_tests_properties(original_tu15_object_light_admission PROPERTIES SKIP_RETURN_CODE 77)
endif()

target_sources(halo2_systems PRIVATE src/rendering/models/source_live_model_geometry_tu15.cpp)

if(TARGET halo2_renderer)
    add_executable(capture_live_model_light_tu15 tools/capture_live_model_light_tu15.cpp)
    target_link_libraries(capture_live_model_light_tu15 PRIVATE halo2_renderer)
    add_executable(capture_live_model_base_tu15 tools/capture_live_model_base_tu15.cpp)
    target_link_libraries(capture_live_model_base_tu15 PRIVATE halo2_renderer)
endif()

# Pure original decal state remains available without local Xbox shader assets.
target_sources(halo2_cache PRIVATE src/rendering/effects/source_decal_state.cpp)

# Original cached decal stream, independently owned from procedural impacts.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/zanzibar.map")
    set(SOURCE_DECAL_SHADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    add_custom_command(OUTPUT ${SOURCE_DECAL_SHADER_DIR}/source_decal_program.glsl
        ${SOURCE_DECAL_SHADER_DIR}/source_decal_program.hpp ${SOURCE_DECAL_SHADER_DIR}/source_decal_program.json
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_source_decal_program_tu15.py
            --cache ${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/zanzibar.map
            --output ${SOURCE_DECAL_SHADER_DIR}/source_decal_program.glsl
        DEPENDS tools/generate_source_decal_program_tu15.py tools/generate_particle_graphics_program.py
            tools/generate_particle_vertex_probe.py tools/audit_rendering.py assets/disc/maps/zanzibar.map VERBATIM)
    add_custom_target(source_decal_program DEPENDS ${SOURCE_DECAL_SHADER_DIR}/source_decal_program.hpp)
    foreach(stage vert geom frag)
        add_custom_command(OUTPUT ${SOURCE_DECAL_SHADER_DIR}/source_decal.${stage}.spv
            COMMAND ${GLSLANG_VALIDATOR} -V -I${SOURCE_DECAL_SHADER_DIR} -I${CMAKE_CURRENT_SOURCE_DIR}/shaders
                ${CMAKE_CURRENT_SOURCE_DIR}/shaders/source_decal.${stage}
                -o ${SOURCE_DECAL_SHADER_DIR}/source_decal.${stage}.spv
            DEPENDS shaders/source_decal.${stage} shaders/combiner.glsl shaders/world_depth_fragment.glsl shaders/source_depth_storage.glsl
                ${SOURCE_DECAL_SHADER_DIR}/source_decal_program.glsl VERBATIM)
    endforeach()
    add_custom_target(source_decal_shaders DEPENDS ${SOURCE_DECAL_SHADER_DIR}/source_decal.vert.spv
        ${SOURCE_DECAL_SHADER_DIR}/source_decal.geom.spv ${SOURCE_DECAL_SHADER_DIR}/source_decal.frag.spv)
    target_sources(halo2_systems PRIVATE src/rendering/effects/source_decal_draw.cpp)
    set_source_files_properties(src/rendering/effects/source_decal_draw.cpp PROPERTIES
        INCLUDE_DIRECTORIES "${SOURCE_DECAL_SHADER_DIR}")
    add_dependencies(halo2_systems source_decal_program)
    target_sources(halo2_renderer PRIVATE src/rendering/effects/source_decal_renderer.cpp)
    target_compile_definitions(halo2_renderer PRIVATE HALO2_SOURCE_DECALS_AVAILABLE=1)
    target_compile_definitions(halo2_native PRIVATE HALO2_SOURCE_DECALS_AVAILABLE=1)
    if(TARGET physics_multiplayer_acceptance)
        target_compile_definitions(physics_multiplayer_acceptance PRIVATE HALO2_SOURCE_DECALS_AVAILABLE=1)
    endif()
    add_dependencies(halo2_renderer source_decal_shaders)
endif()
if(TARGET source_decal_program AND BUILD_TESTING)
    add_executable(source_decal_draw_tests tests/source_decal_draw_tests.cpp)
    target_link_libraries(source_decal_draw_tests PRIVATE halo2_systems nlohmann_json::nlohmann_json)
    add_test(NAME native_source_cached_decal_draw COMMAND source_decal_draw_tests
        "${HALO2_TU15_GAMEPLAY_XBE}" "${CMAKE_CURRENT_SOURCE_DIR}/assets/disc/maps/zanzibar.map"
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/graphics-owner-20261013/cache-fixture.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/decal_pass_tu15_original.json"
        "${CMAKE_CURRENT_SOURCE_DIR}/analysis/map-world-features/decal-consumer-20261014/original-cache-traversal.json")
    set_tests_properties(native_source_cached_decal_draw PROPERTIES SKIP_RETURN_CODE 77)
endif()

# Original-placement receiver cull diagnostic; matched CPU/GPU gates stay separate.
add_executable(capture_live_model_cull_tu15 tools/capture_live_model_cull_tu15.cpp)
target_link_libraries(capture_live_model_cull_tu15 PRIVATE halo2_systems halo2_menu_world_storage nlohmann_json::nlohmann_json)

include(cmake/MapParticleBirthTu15.cmake)

include(cmake/MapAttachedEffectsTu15.cmake)
include(cmake/MapSourceDamageTu15.cmake)
include(cmake/MapDistortionTu15.cmake)
include(cmake/SourceParticleSwarmTu15.cmake)
include(cmake/SourceRenderVisibilityRowTu15.cmake)
include(cmake/SourceParticleSpriteContext.cmake)
include(cmake/SourceParticleRenderStateTu15.cmake)
include(cmake/SourceWorldParticleDrawsTu15.cmake)

include(cmake/SourceContrailGraphics.cmake)
include(cmake/SourceEffectTagLayoutTu15.cmake)
include(cmake/NativeTu15ContrailDrawData.cmake)

if(BUILD_TESTING)
  add_executable(fog_draw_selection_tu15_tests tests/fog_draw_selection_tu15_tests.cpp)
  target_link_libraries(fog_draw_selection_tu15_tests PRIVATE halo2_cache nlohmann_json::nlohmann_json)
  add_test(NAME original_tu15_fog_draw_selection COMMAND fog_draw_selection_tu15_tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tu15_fog_draw_selection.original.json)
endif()

include(cmake/SourceLensQueries.cmake)
include(cmake/SourceLensDraws.cmake)
include(cmake/SourceSceneHandoff.cmake)
include(cmake/MccCpuBitmapProjection.cmake)

# One dependency traversal keeps the HUD delivery cohort consistent.
if(BUILD_TESTING)
    add_custom_target(weapon_hud_delivery_checks DEPENDS
        halo2_native halo2_launcher halo2_title_launcher campaign_weapon_hud_world_tests weapon_zoom_hud_tests
        weapon_zoom_world_tests vehicle_weapon_hud_tests vehicle_weapon_hud_world_tests
        vehicle_weapon_hud_runtime_tests multiplayer_scope_runtime_tests multiplayer_scope_network_tests
        hud_target_draw_tests hud_target_anchor_tests weapon_hud_transport_tests
        native_vehicle_camera_tu15_tests physics_live_world_continuation_tests
        hud_aim_tests hud_bitmap_geometry_tests unit_zoom_hud_tests unit_zoom_tests weapon_hud_tests
        hud_vista_crosshair_tests mcc_hud_import_tests)
endif()

include(cmake/AnimationInterpolation.cmake)
include(cmake/PStencil.cmake)

include(cmake/CampaignEnvironmentTu15.cmake)
include(cmake/NativeTu15ObserverData.cmake)
include(cmake/RendererCorrectness.cmake)
include(cmake/MccLoadingTrack.cmake)
include(cmake/MccBodyActions.cmake)
include(cmake/NativeTu15ColdData.cmake)
include(cmake/EngineWiring.cmake)
include(cmake/SourceShadowArraySwap.cmake)
include(cmake/SourceRuntimeVectorRead.cmake)
include(cmake/MotionSensorComposite.cmake)
include(cmake/SelectedSkinReuse.cmake)
include(cmake/PresentationOffload.cmake)
include(cmake/ModelSkinGpu.cmake)
include(cmake/ModelSkinGpuAdmission.cmake)
include(cmake/ModelSkinCpuMaterialization.cmake)
include(cmake/FrameModelSkinPlan.cmake)
include(cmake/ModelSkinRuntime.cmake)
include(cmake/CampaignModelSkinHandoff.cmake)
include(cmake/CampaignReferenceCapture.cmake)
foreach(native_target halo2_systems halo2_physics_world_runtime halo2_shadow_array_swap_tu15
        halo2_physics_vehicle_runtime halo2_physics_vehicle_update halo2_physics_vehicle_motion
        halo2_physics_vehicle_motion_math halo2_physics_vehicle_environment_state
        halo2_physics_vehicle_contact_state halo2_physics_vehicle_constraint_cleanup)
    halo2_specialize_native_target(${native_target})
endforeach()
if(BUILD_TESTING)
    add_test(NAME native_operand_specialization COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_operand_specialization.py)
    add_test(NAME native_instruction_cursor COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_native_instruction_cursor.py)
endif()
if(APPLE)
    include(cmake/macos/DependencyTests.cmake)
endif()
include(cmake/ShadowNativePrefixTu15.cmake)
if(BUILD_TESTING)
    # Integration fixtures assert that every LanHost::broadcast() replicates;
    # the retail per-client budget may skip ticks by design. Budget behavior
    # is covered by native_lan_snapshot_stream and the Rust netcode tests,
    # which select their own profile.
    get_property(halo2_registered_tests DIRECTORY PROPERTY TESTS)
    foreach(halo2_test IN LISTS halo2_registered_tests)
        set_property(TEST "${halo2_test}" APPEND PROPERTY ENVIRONMENT "H2_NET_PROFILE=unlimited")
    endforeach()
endif()
