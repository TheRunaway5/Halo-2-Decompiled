#include "halo2/source_texture_camera_producer_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>

namespace halo2 {
namespace {
std::uint32_t word(const SourcePhysicsContactMemory &memory,std::uint32_t address,unsigned bytes=4) {
    std::array<std::byte,4> raw;memory.read(address,std::span(raw).first(bytes));
    std::uint32_t value{};for(unsigned i=0;i<bytes;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(i*8);
    return value;
}
}
void source_texture_camera_set_object_marker_tu15(PhysicsLiveWorldTu15 &world,
    std::uint32_t object,std::uint32_t marker,std::uint32_t fov_bits) {
    auto call=world.prepare_source_call(std::array{marker,fov_bits});
    call.gp[6]=object;world.run_actual(0x255e0u,call);
}
void source_texture_camera_off_tu15(PhysicsLiveWorldTu15 &world) {world.call_actual(0x255c0u);}
std::array<std::byte,0x118> source_texture_camera_pc_presented_view_tu15(
    std::span<const std::byte,0x118> original,const SourceTextureCameraPcPresentationTu15 &input) {
    const auto &camera=input.camera;const auto &lens=input.lens;
    if(Bytes(original).u32(0)!=0 || Bytes(original).u32(4)>=4 ||
       lens.outer[2]<=lens.outer[0] || lens.outer[3]<=lens.outer[1] ||
       lens.safe[2]<=lens.safe[0] || lens.safe[3]<=lens.safe[1] ||
       camera.near_distance<0 || camera.far_distance<=camera.near_distance ||
       !(lens.projection_angle>0 && lens.projection_angle<3.141592654f) ||
       (camera.near_distance==0 && !input.oblique_near_plane))
        throw FormatError("PC texture-camera presentation requires its complete admitted source/lens inputs");
    for(float value:{camera.position.x,camera.position.y,camera.position.z,camera.forward.x,
        camera.forward.y,camera.forward.z,camera.up.x,camera.up.y,camera.up.z,
        camera.near_distance,camera.far_distance,input.lighting_distance_scale,
        lens.rectangle_center.x,lens.rectangle_center.y,lens.rectangle_scale,
        lens.screen_offset.x,lens.screen_offset.y})
        if(!std::isfinite(value))throw FormatError("PC texture-camera presentation input is not finite");
    std::array<std::byte,0x118> view;std::copy(original.begin(),original.end(),view.begin());
    const auto put=[&](unsigned offset,const auto &value) {
        std::memcpy(view.data()+offset,&value,sizeof(value));
    };
    put(0x0cu,camera.position);put(0x18u,camera.forward);put(0x24u,camera.up);
    put(0x34u,lens.projection_angle);put(0x38u,input.lighting_distance_scale);
    put(0x3cu,lens.outer);put(0x44u,lens.safe);
    put(0x4cu,camera.near_distance);put(0x50u,camera.far_distance);
    if(camera.near_distance==0) {
        for(float value:*input.oblique_near_plane)if(!std::isfinite(value))
            throw FormatError("PC texture-camera oblique plane input is not finite");
        put(0x54u,*input.oblique_near_plane);
    }
    put(0x64u,std::uint8_t(lens.rectangle_override_enabled));
    put(0x68u,lens.rectangle_center);put(0x70u,lens.rectangle_scale);
    put(0x74u,std::uint8_t(lens.screen_offset_enabled));put(0x78u,lens.screen_offset);
    // Original229A90 copies the complete generated camera at+80 into+0C;
    // this admitted PC overlay keeps both actual cameras identical BEFORE
    // original2B770 can independently clamp only the collection camera1.
    std::copy_n(view.begin()+0x0cu,0x74u,view.begin()+0x80u);
    if(!std::equal(original.begin(),original.begin()+12,view.begin()) ||
       !std::equal(original.begin()+0xf4u,original.end(),view.begin()+0xf4u))
        throw FormatError("PC texture-camera presentation changed an original header/tail owner");
    return view;
}
std::array<std::byte,0x118> source_texture_camera_original_view_tu15(
    PhysicsLiveWorldTu15 &world,unsigned ordinal,unsigned count,unsigned split,std::int32_t user) {
    if(count<1 || count>4 || ordinal>=count || split>2 || user< -1 || user>=4)
        throw FormatError("Texture camera original view lacks the source local-user/count/split domain");
    auto &memory=world.memory();
    if(user>=0) {
        const auto users=word(memory,0x4f54f0u);
        if(!users || word(memory,users+12u+unsigned(user)*4u)==0xffffffffu)
            throw FormatError("Texture camera original view requires its actual live local-user binding");
    }
    // Literal229EF0 user scan and conditional state operand at229FD8..9FF6.
    // A source NONE controller mode retains the original null observer route.
    const auto observer=user>=0 && word(memory,0x474c24u,2)!=0xffffu?
        0x4f655cu+unsigned(user)*0x358u:0u;
    const auto address=0x48ba28u+ordinal*0x118u;
    std::array<std::byte,0x118> before,after;memory.read(address,before);
    auto abi=world.prepare_source_call(std::array<std::uint32_t,4>{address,count,split,observer});
    abi.gp[0]=0;abi.gp[1]=std::uint32_t(user);abi.gp[2]=ordinal;
    world.run_actual(0x229a90u,abi);memory.read(address,after);
    if(word(memory,address)!=0 || word(memory,address+4u)!=ordinal ||
       word(memory,address+8u)!=std::uint32_t(user) ||
       !std::equal(before.begin()+0xf4,before.end(),after.begin()+0xf4))
        throw FormatError("Original229A90 lost its actual view header or inherited tail owner");
    return after;
}
}
