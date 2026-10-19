#pragma once
#include "halo2/renderer.hpp"
#include "halo2/debug_panel.hpp"
#include <cstdlib>
#include <charconv>
#include <functional>
#include <set>

namespace halo2::pstencil {
// Opt-in native diagnostics. Captures use the completed frame without another
// source query or simulation tick; normal playback has no control presents.
inline std::filesystem::path campaign_capture_path(std::uint64_t tick) {
    const auto* directory=std::getenv("HALO2_PSTENCIL_CAMPAIGN_CAPTURES");
    if(!directory || !*directory)return {};
    // A bounded, opt-in final-GPU-frame sequence distinguishes presentation
    // capture artifacts from renderer output. The caller owns capture storage.
    if(const auto* range=std::getenv("HALO2_PSTENCIL_CAMPAIGN_CAPTURE_TICK_RANGE");range && *range) {
        const std::string_view text(range);const auto colon=text.find(':');
        std::uint64_t first{},last{};
        if(colon==std::string_view::npos)throw FormatError("Capture tick range requires first:last");
        const auto a=std::from_chars(text.data(),text.data()+colon,first);
        const auto b=std::from_chars(text.data()+colon+1,text.data()+text.size(),last);
        if(a.ec!=std::errc{} || b.ec!=std::errc{} || a.ptr!=text.data()+colon ||
           b.ptr!=text.data()+text.size() || last<first)
            throw FormatError("Invalid capture tick range");
        if(tick<first || tick>last)return {};
        static std::uint64_t serial{};
        std::filesystem::create_directories(directory);
        return std::filesystem::path(directory)/("tick-"+std::to_string(tick)+"-frame-"+
            std::to_string(serial++)+".ppm");
    }
    static std::set<unsigned> captured;
    constexpr unsigned targets[]{450,972,1150,1400,1800,2839,3400,3700,4000,4700,
        5000,5200,5248,5250,5270,5300,5400,5500,5650,5700};
    for(auto target:targets)if(tick>=target && !captured.contains(target)) {
        captured.insert(target);
        std::filesystem::create_directories(directory);
        return std::filesystem::path(directory)/("tick-"+std::to_string(tick)+"-target-"+
            std::to_string(target)+".ppm");
    }
    return {};
}
using CampaignCaptureDraw=std::function<void(const ModelView&,const std::filesystem::path&)>;
inline void campaign_material_capture_control(Renderer& renderer,std::span<const Material> materials,
    const ModelView& view,const std::filesystem::path& path,const CampaignCaptureDraw& draw) {
    if(!std::getenv("HALO2_PSTENCIL_CAMPAIGN_MATERIAL_CONTROLS"))return;
    const auto output=[&](const char* suffix){return path.parent_path()/(path.stem().string()+suffix);};
    // Use the same query-free diagnostic submission on both sides. Repeating
    // the full bank separately detects frame-history/query side effects instead
    // of attributing a second-render difference to material selection.
    draw(view,output("-active-materials-repeat.ppm"));
    // Same completed view, source state and shader inputs. Restore the earlier
    // full-bank upload as an image control after the actual active-path frame.
    renderer.update_material_constants(materials);
    draw(view,output("-full-materials.ppm"));
    draw(view,output("-full-materials-repeat.ppm"));
}
inline void campaign_capture_controls(Renderer& renderer,const ModelView& view,
    const std::filesystem::path& path,const CampaignCaptureDraw& draw,bool after=false) {
    const auto output=[&](const char* suffix){return path.parent_path()/(path.stem().string()+suffix);};
    if(std::getenv("HALO2_PSTENCIL_CAMPAIGN_MODE_CONTROLS")) {
        auto settings=renderer.debug_panel().display_settings();
        struct Restore {
            Renderer& renderer;decltype(settings) saved;
            ~Restore(){renderer.debug_panel().display_settings(saved);}
        } restore{renderer,settings};
        settings.graphics_mode=GraphicsMode::Original;
        renderer.debug_panel().display_settings(settings);
        auto original=view;original.raw_effect_lights={};original.effect_lights={};
        original.pstencil_point_casters={};original.pstencil_lightmap_casters={};
        draw(original,output(after?"-Original-restored.ppm":"-Original-before.ppm"));
    }
    if(after)return;
    if(std::getenv("HALO2_PSTENCIL_CAMPAIGN_PROJECTED_CONTROLS")) {
        auto control=view;control.raw_effect_lights={};control.pstencil_point_casters={};
        draw(control,output("-without-projected-lights.ppm"));
        control=view;control.pstencil_point_casters={};
        draw(control,output("-without-point-shadows.ppm"));
    }
    if(std::getenv("HALO2_PSTENCIL_CAMPAIGN_SHADOW_CONTROLS")) {
        auto control=view;control.pstencil_point_casters={};control.pstencil_lightmap_casters={};
        draw(control,output("-without-shadows.ppm"));
    }
}
}
