#include "halo2/native_tu15_render_prepare.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <atomic>
#include <cstdlib>
#include <iostream>

namespace halo2 {
namespace {
struct Stats {
    std::atomic<std::uint64_t> queues{},objects{},geometry{},requests{};
    ~Stats(){if(const auto* v=std::getenv("HALO2_NATIVE_RENDER_PREPARE_STATS");v&&*v)
        std::cerr<<"Native render prepare stats: queues="<<queues<<" objects="<<objects<<
            " geometry="<<geometry<<" resource_calls="<<requests<<'\n';}
};
Stats& stats(){static Stats s;return s;}
void invoke(PhysicsLiveWorldTu15& world,std::uint32_t entry,SourcePhysicsWorldRuntimeAbi& abi,NativeTu15PhysicsFrameFp& fp) {
    abi.x87_status=fp.status;abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};
    world.run_actual(entry,abi);fp={abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
}
NativeTu15RenderPrepareServices services(PhysicsLiveWorldTu15& world) {
    NativeTu15RenderPrepareServices s;
    s.update_d7230=[&](auto object,auto mode,auto flags,auto& fp) {
        ++stats().objects;auto abi=world.prepare_source_call(std::array{flags});
        abi.gp[0]=object;abi.gp[3]=mode;invoke(world,0xd7230u,abi,fp);
    };
    s.attached_41970=[&](auto mode,auto object,auto& fp) {
        auto abi=world.prepare_source_call(std::array{mode,object});invoke(world,0x41970u,abi,fp);
    };
    return s;
}
}
void source_native_render_prepare_tu15(PhysicsLiveWorldTu15& world,std::uint32_t context,std::uint32_t mode,NativeTu15PhysicsFrameFp& fp) {
    ++stats().queues;native_tu15_render_prepare_41c70(world.memory(),context,mode,fp,services(world));
}
void source_native_render_object_prepare_tu15(PhysicsLiveWorldTu15& world,std::uint32_t mode,std::uint32_t object,
    std::uint32_t flags,NativeTu15PhysicsFrameFp& fp) {
    native_tu15_render_object_prepare_41c10(world.memory(),mode,object,flags,fp,services(world));
}
std::uint32_t source_native_render_geometry_tu15(PhysicsLiveWorldTu15& world,std::uint32_t unit,
    std::uint8_t request,std::uint8_t owner,NativeTu15PhysicsFrameFp& fp) {
    ++stats().geometry;
    return native_tu15_render_geometry_449d0(world.memory(),unit,request,owner,fp,[&](auto header,auto& arithmetic) {
        ++stats().requests;return world.request_render_geometry_tu15(3u,header,arithmetic);
    });
}
}
