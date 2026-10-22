"""Explicit inventory family emitted by the native Vulkan/pStencil/upscaling build.

Legacy capability readers retain their 97-file family. This family replaced
unused compute programs with native pStencil/upscaling stages; count alone
cannot establish that either renderer's complete bank is present.
"""
import hashlib
from pathlib import Path

PROFILE = "native-vulkan-pstencil-upscaling-v1"
REQUIRED_FILES = frozenset("""
bloom.frag.spv
bloom.vert.spv
bsp_patchy_composite.frag.spv
bsp_patchy_noise.frag.spv
cinematic_crossfade.frag.spv
crt_oled.frag.spv
display.frag.spv
fog.frag.spv
fog.vert.spv
hud.frag.spv
light_volume.frag.spv
light_volume.vert.spv
menu_world.frag.spv
menu_world.geom.spv
overlay.frag.spv
overlay.vert.spv
particle.frag.spv
particle.geom.spv
particle.vert.spv
particle_directional.vert.spv
particle_directional_program.glsl
particle_directional_program.hpp
particle_directional_program.json
particle_distortion.vert.spv
particle_distortion_program.glsl
particle_distortion_program.hpp
particle_distortion_program.json
particle_model.geom.spv
particle_model.vert.spv
particle_model_program.glsl
particle_model_program.hpp
particle_model_program.json
particle_perpendicular.vert.spv
particle_perpendicular_program.glsl
particle_perpendicular_program.hpp
particle_perpendicular_program.json
particle_program.glsl
particle_program.hpp
particle_program.json
patchy_composite.frag.spv
patchy_noise.frag.spv
patchy_noise.vert.spv
pstencil/dependent_light.frag.spv
pstencil/light_utility.frag.spv
pstencil/lightmap.frag.spv
pstencil/lightmap.vert.spv
pstencil/normal_accumulation.frag.spv
pstencil/normal_accumulation.vert.spv
pstencil/normal_light_utility.frag.spv
pstencil/normal_light_utility.vert.spv
pstencil/texture_accumulation.frag.spv
pstencil/volume.frag.spv
pstencil/volume.vert.spv
scene_copy.frag.spv
scope_copy.frag.spv
source_contrail.frag.spv
source_contrail.geom.spv
source_contrail.vert.spv
source_contrail_program.glsl
source_contrail_program.hpp
source_contrail_program.json
source_decal.frag.spv
source_decal.geom.spv
source_decal.vert.spv
source_decal_program.glsl
source_decal_program.hpp
source_decal_program.json
source_depth_color_writeback.comp.spv
source_lens_draw.frag.spv
source_lens_draw.vert.spv
source_lens_query.frag.spv
source_lens_query.vert.spv
source_shadow_filter.frag.spv
source_shadow_filter.vert.spv
source_weather_depth_alias.comp.spv
source_weather_line.frag.spv
source_weather_line.geom.spv
source_weather_line.vert.spv
source_weather_plate.frag.spv
source_weather_plate.vert.spv
source_weather_point.frag.spv
source_weather_point.vert.spv
ui.frag.spv
ui.vert.spv
ui_frame.frag.spv
upscale_motion.frag.spv
upscale_motion.vert.spv
upscale_prepare.comp.spv
vista_loading.frag.spv
vista_loading.vert.spv
water_copy.frag.spv
world.frag.spv
world.vert.spv
world_emblem.frag.spv
world_emblem.geom.spv
world_prt.vert.spv
""".split())


def inventory(directory):
    directory = Path(directory)
    if directory.is_symlink() or directory.resolve(strict=True) != directory:
        raise ValueError("Shader bank requires its canonical directory owner")
    files = {}
    for path in sorted(directory.rglob("*")):
        if path.is_symlink():
            raise ValueError("Shader inventory cannot contain aliases")
        if path.is_file():
            files[path.relative_to(directory).as_posix()] = hashlib.sha256(path.read_bytes()).hexdigest()
    if not REQUIRED_FILES <= files.keys():
        raise ValueError("Native renderer shader family is incomplete")
    return files
