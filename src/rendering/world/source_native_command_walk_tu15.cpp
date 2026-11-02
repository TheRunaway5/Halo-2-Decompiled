#include "halo2/native_tu15_command_walk.hpp"
#include "halo2/native_tu15_render_prepare.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <atomic>
#include <cstdlib>
#include <iostream>

namespace halo2 {
namespace {
struct Stats {
    std::atomic<std::uint64_t> frames{},geometry{},parts{},emits{};
    ~Stats(){if(const auto* v=std::getenv("HALO2_NATIVE_COMMAND_WALK_STATS");v&&*v)
        std::cerr<<"Native40E80 stats: frames="<<frames<<" geometry_calls="<<geometry<<
            " part_calls="<<parts<<" emit_calls="<<emits<<'\n';}
};
Stats& stats(){static Stats s;return s;}
}
NativeTu15PhysicsFrameFp source_native_command_frame_tu15(PhysicsLiveWorldTu15& world,std::uint32_t selector) {
    const auto initial=world.prepare_source_call();const auto controls=initial.floating_point.value();
    NativeTu15PhysicsFrameFp fp{controls.x87_control_word,initial.x87_status,controls.mxcsr};
    // Reserve the genuine caller packet above the maximum service arguments.
    // Preserve every existing byte while validating this whole caller extent;
    // nested calls may not claim another caller's arbitrary stack memory.
    const auto packet=initial.gp[4]+0x80u;
    std::array<std::uint32_t,42> retained{};
    world.memory().read(initial.gp[4]+4u,std::as_writable_bytes(std::span(retained)));
    world.prepare_source_call(retained);
    const auto invoke=[&](auto entry,auto& abi,auto& arithmetic) {
        abi.x87_status=arithmetic.status;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{arithmetic.control_word,arithmetic.mxcsr};
        world.run_actual(entry,abi);
        arithmetic={abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    };
    NativeTu15CommandFrameServices s;s.parts.packet=packet;
    static const bool reference=[]{const auto* p=std::getenv("HALO2_REFERENCE_RENDER_PREPARE");return p&&*p;}();
    s.parts.geometry_449d0=[&](auto unit,auto owner,auto& arithmetic) {
        ++stats().geometry;
        if(!reference)return source_native_render_geometry_tu15(world,unit,0,std::uint8_t(owner),arithmetic);
        auto abi=world.prepare_source_call(std::array{owner});
        abi.gp[0]=unit;abi.gp[2]=0;invoke(0x449d0u,abi,arithmetic);return abi.gp[0];
    };
    s.parts.prepare_45cd0=[&](auto unit,auto part,auto matrix,auto& arithmetic) {
        ++stats().parts;auto abi=world.prepare_source_call(std::array{part,matrix});
        abi.gp[0]=packet;abi.gp[1]=unit;invoke(0x45cd0u,abi,arithmetic);return (abi.gp[0]&255u)!=0;
    };
    s.parts.emit_41030=[&](auto mode,auto coverage,auto shader,bool grouped,auto unit,auto part,auto& arithmetic) {
        ++stats().emits;auto abi=world.prepare_source_call(std::array{mode,coverage,packet,shader,std::uint32_t(grouped),0xffffffffu,0u});
        abi.gp[1]=part;abi.gp[2]=unit;invoke(0x41030u,abi,arithmetic);
    };
    s.prepare_41c70=[&](auto context,auto mode,auto& arithmetic) {
        if(!reference){source_native_render_prepare_tu15(world,context,mode,arithmetic);return;}
        auto abi=world.prepare_source_call();abi.gp[7]=context;abi.gp[3]=mode;invoke(0x41c70u,abi,arithmetic);
    };
    const auto no_args=[&](auto entry,auto& arithmetic) {auto abi=world.prepare_source_call();invoke(entry,abi,arithmetic);};
    s.producer_179a40=[&](auto& arithmetic){no_args(0x179a40u,arithmetic);};
    s.producer_179b40=[&](auto& arithmetic){no_args(0x179b40u,arithmetic);};
    s.producer_1770c0=[&](auto& arithmetic){auto abi=world.prepare_source_call(std::array{0u});invoke(0x1770c0u,abi,arithmetic);};
    s.producer_42090=[&](auto& arithmetic){no_args(0x42090u,arithmetic);};
    s.producer_42750=[&](auto mask,auto& arithmetic){auto abi=world.prepare_source_call(std::array{mask});invoke(0x42750u,abi,arithmetic);};
    s.producer_41630=[&](auto& arithmetic){no_args(0x41630u,arithmetic);};
    native_tu15_command_frame_40e80(world.memory(),selector,fp,s);++stats().frames;return fp;
}
}
