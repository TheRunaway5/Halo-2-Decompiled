#!/usr/bin/env python3
"""Apply the small pStencil renderer seam to a checkout or frozen source copy.

Every anchor must be unique. This preserves unrelated renderer changes and
rejects a changed interface instead of replacing the whole shared file.
"""
import argparse
import hashlib
import json
from pathlib import Path


def wire(path):
    before = path.read_text()
    if '#include "halo2/pstencil/frame.hpp"' in before:
        for marker in ("pstencil_frame.prepare", "pstencil_frame.add", "pstencil_render->light", "->pstencil_available"):
            if marker not in before:
                raise ValueError("partial pStencil renderer integration: " + marker)
        return before, before
    edits = [
        ('#include "halo2/source_shadow_render.hpp"', '#include "halo2/source_shadow_render.hpp"\n#include "halo2/pstencil/frame.hpp"'),
        ('    std::unique_ptr<SourceShadowRender> shadow_render;', '    std::unique_ptr<SourceShadowRender> shadow_render;\n    std::unique_ptr<pstencil::Render> pstencil_render;'),
        ('        shadow_render.reset();', '        shadow_render.reset();\n        pstencil_render.reset();'),
        ('    std::string validate_display(const DisplaySettings &s) const {', '''    std::string validate_display(const DisplaySettings &s) const {
        if(s.graphics_mode!=pstencil::GraphicsMode::Original && s.graphics_mode!=pstencil::GraphicsMode::pStencil)
            return "Invalid graphics mode.";
        if(s.graphics_mode==pstencil::GraphicsMode::pStencil &&
           (!source_world_device.depth_clamp_enabled || depth_format!=VK_FORMAT_D24_UNORM_S8_UINT))
            return "pStencil is unavailable on this graphics device.";'''),
        ('        std::vector<std::vector<std::size_t>> raw_light_views(views.size());', '''        pstencil::Frame pstencil_frame(views.size());
        const bool pstencil_enabled=display.graphics_mode==pstencil::GraphicsMode::pStencil;
        std::vector<std::vector<std::size_t>> raw_light_views(views.size());'''),
        ('                raw_light_inputs.push_back(menu_world_native_packet(input));', '''                if(pstencil_enabled)pstencil_frame.add(i,draw,raw_light_inputs.size());
                if(pstencil_enabled)input.raster=pstencil::receiver_raster(input.raster);
                raw_light_inputs.push_back(menu_world_native_packet(input));'''),
        ('        if(effect_light_native)effect_light_native->prepare(raw_light_inputs);', '''        if(effect_light_native)effect_light_native->prepare(raw_light_inputs);
        if(pstencil_enabled && !pstencil_frame.empty()) {
            if(!pstencil_render)pstencil_render=std::make_unique<pstencil::Render>(pstencil::RenderContext{
                device,physical,extent,continuation_pass,depth,this->views,shader_directory});
            pstencil_frame.prepare(*pstencil_render);
        }
        if(pstencil_enabled && !screenshot.empty())std::cout<<pstencil_frame.describe()<<'\\n';'''),
        ('                for(const auto draw:raw_light_views[i]){capture_unknown();effect_light_native->record(command,draw);}', '''                if(pstencil_enabled && !pstencil_frame.lights(i).empty()) {
                    capture_unknown();pstencil_render->begin(command,index);
                    for(const auto& light:pstencil_frame.lights(i)) {
                        if(!v.source_view_shader_constants)
                            throw FormatError("pStencil requires the current source camera bank");
                        pstencil::VolumeCamera volume_camera;
                        std::memcpy(volume_camera.matrix.data(),&model_uniform.matrix,sizeof(volume_camera.matrix));
                        volume_camera.depth_row=v.source_view_shader_constants->world[3];
                        volume_camera.light={light.position.x,light.position.y,light.position.z,pstencil::shadow_extrusion_distance};
                        volume_camera.control={model_uniform.eye.w,1,0,0};
                        pstencil_render->light(command,light.upload,volume_camera,model_view,clip,[&] {
                            vkCmdSetViewport(command,0,1,&raw_light_viewport);vkCmdSetScissor(command,0,1,&clip);
                            for(const auto draw:light.receivers)effect_light_native->record(command,draw);
                        });
                    }
                    pstencil_render->end(command,framebuffers[index]);
                } else {
                    for(const auto draw:raw_light_views[i]){capture_unknown();effect_light_native->record(command,draw);}
                }'''),
        ('    if(!impl_->debug_panel)impl_->debug_panel=std::make_unique<DebugPanel>(impl_->window);', '''    if(!impl_->debug_panel)impl_->debug_panel=std::make_unique<DebugPanel>(impl_->window);
    impl_->debug_panel->pstencil_available(impl_->source_world_device.depth_clamp_enabled &&
        impl_->depth_format==VK_FORMAT_D24_UNORM_S8_UINT);''')]
    result = before
    for old, new in edits:
        if result.count(old) != 1:
            raise ValueError(f"renderer anchor is not unique: {old}")
        result = result.replace(old, new, 1)
    path.write_text(result)
    return before, result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("renderer", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    before, after = wire(args.renderer)
    report = dict(renderer=str(args.renderer.resolve()), changed=before != after,
                  before_sha256=hashlib.sha256(before.encode()).hexdigest(),
                  after_sha256=hashlib.sha256(after.encode()).hexdigest())
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report))


if __name__ == "__main__":
    main()
