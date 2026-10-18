#include "halo2/source_campaign_screen_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include <bit>
namespace halo2 {
namespace {
std::uint32_t bank(const SourceRuntimeMemory &m) {return Bytes(m.view(0x515ec0u,4)).u32(0);}
template<class T>void put(SourceRuntimeMemory &m,std::uint32_t at,const T &v) {
    m.write(at,std::as_bytes(std::span(&v,1)));
}
}
void source_campaign_far_clip_tu15(const SourceRuntimeMemory &m,
    const std::function<void(std::uint32_t,std::span<const std::byte>)> &write,float distance) {
    const auto p=bank(m);if(!p)return;
    if(!write)throw FormatError("Cinematic far clip requires its checked source writer");
    write(p+0x194u,std::as_bytes(std::span(&distance,1)));
}
void source_campaign_far_clip_tu15(PhysicsLiveWorldTu15 &world,float distance) {
    source_campaign_far_clip_tu15(world.memory(),[&](auto at,auto bytes) {
        world.write_source_call_input(at,bytes);
    },distance);
}
Vec2 source_campaign_clip_planes_tu15(const SourceRuntimeMemory &m) {
    const Bytes defaults(m.view(0x473574u,8));Vec2 result{defaults.f32(0),defaults.f32(4)};
    if(const auto p=bank(m)) {
        const Bytes values(m.view(p+0x190u,8));
        // COMISS/JBE rejects zero, negative and unordered overrides. Do not
        // sanitize the stored REAL: original2B5185 copies all32 bits.
        const float near=values.f32(0),far=values.f32(4);
        if(near>0.f)result.x=near;
        if(far>0.f)result.y=far;
    }
    return result;
}
Vec2 source_campaign_clip_planes_tu15(PhysicsLiveWorldTu15 &world) {
    const auto result=source_campaign_clip_planes_tu15(world.memory());
    world.write_source_call_input(0x4922f4u,std::as_bytes(std::span(&result.x,1)));
    world.write_source_call_input(0x4922f8u,std::as_bytes(std::span(&result.y,1)));
    return result;
}
void source_campaign_depth_of_field_tu15(PhysicsLiveWorldTu15 &world,
    std::span<const std::uint32_t,7> arguments) {
    auto abi=world.prepare_source_call(arguments.subspan<1>());
    abi.xmm[0][0]=arguments[0];world.run_actual(0x1fc10u,abi);
}
SourceCampaignDepthOfFieldFrameTu15 source_campaign_depth_of_field_frame_tu15(
    const SourceRuntimeMemory &m,bool view_modifier,bool target_ready) {
    const auto p=bank(m);if(!p || m.view(p+0xacu,1)[0]==std::byte{})return {};
    const Bytes mode(m.view(0x4920b8u,4)),special(m.view(0x491e22u,2));
    const auto kind=std::bit_cast<std::int32_t>(mode.u32(0));
    if(special.u16(0)==2 || view_modifier || (kind>=5&&kind<=7))return {};
    const auto raw=m.view(p+0x3cu,16);const Bytes output(raw);
    if(raw[0]==std::byte{} || !(output.f32(4)>0.f))return {};
    const Bytes rectangle(m.view(0x491e68u,8)),views(m.view(0x4c686cu,4));
    if(rectangle.u16(0)!=0 || rectangle.u16(2)!=0 || rectangle.u16(4)!=480 ||
       rectangle.u16(6)!=640 || std::bit_cast<std::int32_t>(views.u32(0))>1 || !target_ready)return {};
    return {true,output.f32(4),output.f32(8),output.f32(12)};
}
std::array<SourceCampaignDepthOfFieldSceneTu15,2> source_campaign_depth_of_field_scenes_tu15(
    float near,float far,float focus,std::uint32_t current_target) {
    volatile float reciprocal=1.f/far;
    const auto plane=[&](float distance) {
        volatile float product=float(reciprocal)*distance;
        volatile float scaled=float(product)*16777215.f;float value=scaled;
        // The original COMISS branches retain unordered operands and -0.
        if(0.f>value)value=0.f;else if(value>16777215.f)value=16777215.f;
        return value;
    };
    const float minimum=plane(near),maximum=plane(far),boundary=plane(focus);
    return {{{1u,current_target,boundary,maximum},{2u,18u,minimum,boundary}}};
}
void source_campaign_screen_start_tu15(SourceRuntimeMemory &m,bool reset) {
    const auto p=bank(m);if(!p)return;
    if(reset || m.view(p+0xadu,1)[0]==std::byte{}) {
        const std::array<std::byte,0xac> clear{};m.write(p,clear);
        put(m,p+0xadu,std::uint8_t{1});
    }
    put(m,p+0xacu,std::uint8_t{1});
}
void source_campaign_crossfade_set_tu15(SourceRuntimeMemory &m,float seconds,float strength) {
    const auto p=bank(m);if(!p)return;
    const Bytes clock_bytes(m.view(0x4920c0u,8));
    const auto clock=std::bit_cast<double>(std::uint64_t(clock_bytes.u32(0))|
        (std::uint64_t(clock_bytes.u32(4))<<32));
    // Original FLD/FST/FADD/FSTP rounds each published endpoint to float32,
    // retaining the original double clock for the end-time addition.
    put(m,p+0x170u,std::uint8_t{1});put(m,p+0x174u,strength);
    put(m,p+0x178u,float(clock));put(m,p+0x17cu,float(clock+double(seconds)));
}
void source_campaign_screen_clock_tu15(SourceRuntimeMemory &m,double clock) {
    //13AA2D..13AA38 owns the accepted elapsed accumulator;2B5B0/12A30
    // publishes that double at the beginning of the native render phase.
    put(m,0x4c6860u,clock);put(m,0x4920c0u,clock);
}
void source_campaign_screen_update_tu15(PhysicsLiveWorldTu15 &w) {w.call_actual(0x1fd00u);}
void source_campaign_screen_stop_tu15(PhysicsLiveWorldTu15 &w) {w.call_actual(0x1fcb0u);}
void source_campaign_screen_capture_complete_tu15(PhysicsLiveWorldTu15 &w) {w.call_actual(0x1fce0u);}
CinematicCrossfadeState source_campaign_crossfade_state_tu15(const SourceRuntimeMemory &m) {
    const auto p=bank(m);if(!p)return {};
    const auto raw=m.view(p,0x3fcu);const Bytes b(raw);
    return {raw[0xac]!=std::byte{},raw[0xad]!=std::byte{},raw[0x170]!=std::byte{},raw[0xa0]!=std::byte{},
        b.f32(0x174),b.f32(0x178),b.f32(0x17c),b.f32(0xa4),b.f32(0xa8)};
}
CinematicCrossfadeFrame source_campaign_crossfade_frame_tu15(const SourceRuntimeMemory &m) {
    return source_campaign_crossfade_state_tu15(m).frame();
}
}
