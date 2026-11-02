#include "halo2/native_tu15_cull_walk_44710.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <atomic>
#include <cstdlib>
#include <iostream>

namespace halo2 {
namespace {
struct Stats {
    std::atomic<std::uint64_t> calls{},frustum{},sphere{},native{},residual{};
    ~Stats() {
        const auto* enabled=std::getenv("HALO2_NATIVE_CULL_WALK_STATS");if(!enabled || !*enabled)return;
        std::cerr<<"Native44710 stats: calls="<<calls<<" frustum_calls="<<frustum<<" sphere_calls="<<sphere<<
            " native_entries="<<native<<" residual_entries="<<residual<<'\n';
    }
};
Stats& stats(){static Stats s;return s;}
}
NativeTu15PhysicsFrameFp source_native_cull_walk_tu15(PhysicsLiveWorldTu15& world,const NativeTu15CullFrustumEntry& native_entry) {
    const auto initial=world.prepare_source_call();const auto controls=initial.floating_point.value();
    NativeTu15PhysicsFrameFp fp{controls.x87_control_word,initial.x87_status,controls.mxcsr};
    const auto invoke=[&](auto entry,auto& abi,auto& arithmetic) {
        abi.x87_status=arithmetic.status;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{arithmetic.control_word,arithmetic.mxcsr};
        world.run_actual(entry,abi);
        arithmetic={abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    };
    NativeTu15CullWalkServices services;
    services.frustum=[&](auto unit,auto entry,auto& arithmetic) {
        ++stats().frustum;
        if(native_entry && native_entry(unit,entry,arithmetic)){++stats().native;return;}
        ++stats().residual;
        auto abi=world.prepare_source_call(std::array{unit,entry});invoke(0x44e80u,abi,arithmetic);
    };
    services.sphere=[&](auto unit,const auto& center,auto radius,auto& arithmetic) {
        ++stats().sphere;++stats().residual;
        // Caller-owned arguments above ESP survive the actual 45550 frame.
        // prepare_source_call checks the full extent for nested callers too.
        auto abi=world.prepare_source_call(std::array{0u,radius,center[0],center[1],center[2]});
        const auto point=abi.gp[4]+12u;
        world.write_source_call_input(abi.gp[4]+4u,std::as_bytes(std::span(&point,1)));
        abi.gp[1]=unit;invoke(0x45550u,abi,arithmetic);
    };
    native_tu15_cull_walk_44710(world.memory(),fp,services);
    ++stats().calls;
    return fp;
}
}
