#include "halo2/source_texture_camera_frame_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_texture_camera_scene_prefix_tu15.hpp"
#include <algorithm>
#include <bit>
#include <cmath>

namespace halo2 {
// Implemented with the capture owner in the same physics component, so the
// World does not link upward to the application or Vulkan renderer.
void begin_source_texture_camera_capture_tu15(SourcePhysicsContactMemory &,SourceTextureCameraFrameTu15 &);
void end_source_texture_camera_capture_tu15(SourcePhysicsContactMemory &)noexcept;
SourceTextureCameraViewFieldsTu15 source_texture_camera_view_fields_tu15(
    const SourceTextureCameraFrameTu15 &frame,SourceFrustumProjectionInput context) {
    frame.targets().require_current();
    if(!frame.scene_view())throw FormatError("Texture camera has no completed source scene view");
    const Bytes raw(frame.completed_scene_view()?*frame.completed_scene_view():*frame.scene_view());
    if(raw.u32(0)!=0 || raw.u32(8)!=0xffffffffu)
        throw FormatError("Texture camera lost its source world/NONE-user view header");
    const auto vector=[&](unsigned p){return Vec3{raw.f32(p),raw.f32(p+4u),raw.f32(p+8u)};};
    context.projection_angle=raw.f32(0x34u);
    for(unsigned n=0;n<4;++n) {
        context.outer[n]=std::bit_cast<std::int16_t>(raw.u16(0x3cu+n*2u));
        context.safe[n]=std::bit_cast<std::int16_t>(raw.u16(0x44u+n*2u));
    }
    context.rectangle_override_enabled=raw.slice(0x64u,1)[0]!=std::byte{};
    context.rectangle_center={raw.f32(0x68u),raw.f32(0x6cu)};
    context.rectangle_scale=raw.f32(0x70u);
    context.screen_offset_enabled=raw.slice(0x74u,1)[0]!=std::byte{};
    context.screen_offset={raw.f32(0x78u),raw.f32(0x7cu)};
    return {vector(0xcu),vector(0x18u),vector(0x24u),context,raw.f32(0x38u),
        raw.f32(0x4cu),raw.f32(0x50u),{raw.f32(0x54u),raw.f32(0x58u),raw.f32(0x5cu),raw.f32(0x60u)}};
}
std::array<std::int32_t,4> source_texture_camera_viewport_tu15(const SourceTextureCameraFrameTu15 &frame) {
    if(!frame.completed_scene_prefix())throw FormatError("Texture camera viewport lacks its completed CPU-prefix");
    const auto &completed=*frame.completed_scene_prefix();
    if(completed.backend_target!=0u)
        throw FormatError("Texture camera direct backend target has no admitted native routing owner");
    const Bytes packet(completed.packet);
    if(packet.u16(2u)==2u)return {0,0,128,128}; // Literal14D27..14D44.
    float scale=1.f;
    if(completed.texture_camera_view && !completed.copy_complete) {
        scale=std::bit_cast<float>(completed.render_scale_bits);
        if(!std::isfinite(scale))throw FormatError("Texture camera viewport scale exceeds finite source arithmetic");
        scale=std::clamp(scale,.0625f,1.f); // Actual46A4B0 and46A0D0.
    }
    const auto edge=[&](unsigned p){return std::int32_t(std::bit_cast<std::int16_t>(packet.u16(p)));};
    const auto left=edge(0x4au),top=edge(0x48u),width=edge(0x4eu)-left,height=edge(0x4cu)-top;
    //14E19..14E79 FILD/FMUL then whole32CC0C truncates toward zero.
    // A signed-short span times binary32 scale needs fewer than53 precision
    // bits, so the original x87 product is exact before integer truncation.
    const auto product=[&](std::int32_t v){return std::int32_t(static_cast<long double>(v)*scale);};
    const auto l=product(left),t=product(top),w=product(width),h=product(height);
    if(w<=0 || h<=0)throw FormatError("Texture camera source viewport is empty or reversed");
    return {t,l,t+h,l+w};
}
void source_texture_camera_complete_scene_prefix_tu15(PhysicsLiveWorldTu15 &world,
    SourceTextureCameraFrameTu15 &frame,const SourceTextureCameraScenePrefixTu15 &prefix) {
    prefix.require_current(world.memory());
    if(frame.memory_owner_!=&world.memory() || !frame.view_ || frame.completed_ || frame.submission_->load()!=0u)
        throw FormatError("Texture camera CPU-prefix completion lacks its same pending source request");
    if(!prefix.texture_camera || !prefix.backend_requested || prefix.finished || prefix.actual_local_user!=-1)
        throw FormatError("Texture camera completion lost its actual secondary/NONE-user backend boundary");
    // Whole2B770 changes camera1+44 only via the literal2B888 clamp. Its
    // alternate camera2 remains the complete pre-clamp25800 copy.
    for(unsigned n=0;n<0x118u;++n)if((n<0x50u || n>=0x54u) && prefix.completed_view[n]!=(*frame.view_)[n])
        throw FormatError("Texture camera CPU-prefix changed an unauthored source view field");
    const auto equal=[](auto actual,auto expected){return std::ranges::equal(actual,expected);};
    const auto view=std::span(prefix.completed_view);
    const auto request=std::span(*frame.view_);
    if(!equal(std::span(prefix.raw_camera),view.subspan(0xcu,0x74u)) ||
       !equal(std::span(prefix.packet).subspan(0x18u,0x74u),request.subspan(0x80u,0x74u)) ||
       !equal(world.memory().view(0x4c6710u,0x120u),prefix.environment) ||
       !equal(world.memory().view(0x4c65c0u,0x74u),prefix.raw_camera) ||
       !equal(world.memory().view(0x4c6634u,0xc0u),prefix.projection))
        throw FormatError("Texture camera CPU-prefix lost its distinct actual camera/packet/environment banks");
    const Bytes packet(prefix.packet);
    if(packet.u16(0)!=Bytes(*frame.view_).u32(4) ||
       packet.slice(7,1)[0]!=std::byte{1})
        throw FormatError("Texture camera packet lost its original ordinal/recursive-view flag");
    SourceTextureCameraCompletedPrefixTu15 completed{prefix.packet,prefix.raw_camera,prefix.projection,{},{},{},{},{},{}};
    const Bytes rectangle(world.memory().view(0x4c65f0u,8u)),depth(world.memory().view(0x4922f4u,8u));
    for(unsigned n=0;n<4;++n)completed.copy_rectangle[n]=std::bit_cast<std::int16_t>(rectangle.u16(n*2u));
    for(unsigned n=0;n<2;++n)completed.copy_depth_bits[n]=depth.u32(n*4u);
    completed.backend_target=Bytes(world.memory().view(0x4920d8u,4u)).u32(0);
    completed.render_scale_bits=Bytes(world.memory().view(0x473628u,4u)).u32(0);
    completed.texture_camera_view=std::to_integer<std::uint8_t>(world.memory().view(0x491e27u,1u)[0]);
    completed.copy_complete=std::to_integer<std::uint8_t>(world.memory().view(0x515edcu,1u)[0]);
    if(completed.texture_camera_view!=1u)
        throw FormatError("Texture camera CPU-prefix did not publish its actual recursive-view byte");
    prefix.require_current(world.memory());
    frame.completed_=prefix.completed_view;frame.prefix_=std::move(completed);
}
SourceTextureCameraFrameTu15 source_texture_camera_frame_tu15(PhysicsLiveWorldTu15 &world,
    std::span<const std::byte,0x118> original,const SourceTextureCameraNativeTargetsTu15 &targets) {
    targets.require_current();
    SourceTextureCameraFrameTu15 frame(targets);auto &memory=world.memory();
    frame.memory_owner_=&memory;
    frame.before_=source_texture_camera_inputs_tu15(memory);
    targets.require_inputs(frame.before_);
    auto call=world.prepare_source_call();const auto input=call.gp[4]+0x1000u;
    world.write_source_call_input(input,original);call.gp[0]=input;
    begin_source_texture_camera_capture_tu15(memory,frame);
    struct Scope {SourcePhysicsContactMemory &m;~Scope(){end_source_texture_camera_capture_tu15(m);}} scope{memory};
    world.run_actual(0x25800u,call);
    frame.after_=source_texture_camera_inputs_tu15(memory);
    frame.rendered_=(call.gp[0]&255u)!=0u;
    if(frame.rendered_!=frame.view_.has_value())
        throw FormatError("Texture camera source result lost its deferred scene request");
    frame.suppress_ordinary_=frame.rendered_ && frame.after_.copy_complete;
    if(!(frame.after_.cinematic_auxiliary && frame.after_.cinematic)) {
        if(frame.after_.camera_words[0]!=0xffffffffu || frame.after_.copy_complete ||
           std::any_of(frame.after_.camera_words.begin()+1,frame.after_.camera_words.end(),[](auto v){return v!=0;}))
            throw FormatError("Texture camera source25800 did not complete its exact unconditional cleanup");
    }
    targets.require_current();return frame;
}
}
