#include "halo2/native_tu15_object_scale.hpp"
#include "halo2/physics_live_world_tu15.hpp"

namespace halo2 {
void source_campaign_object_scale_tu15(PhysicsLiveWorldTu15& world,std::uint32_t object,
    std::uint32_t scale,std::uint16_t ticks) {
    if(object==0xffffffffu)return;
    const auto initial=world.prepare_source_call();const auto& controls=initial.floating_point.value();
    NativeTu15PhysicsFrameFp fp{controls.x87_control_word,initial.x87_status,controls.mxcsr};
    NativeTu15ObjectScaleServices services;
    const auto call=[&](std::uint32_t entry,std::span<const std::uint32_t> args,
        std::uint32_t esi,NativeTu15PhysicsFrameFp& arithmetic) {
        auto abi=world.prepare_source_call(args);abi.gp[6]=esi;
        abi.x87_status=arithmetic.status;
        abi.floating_point=SourcePhysicsFactoryFloatingPoint{arithmetic.control_word,arithmetic.mxcsr};
        world.run_actual(entry,abi);const auto& result=abi.floating_point.value();
        arithmetic={result.x87_control_word,abi.x87_status,result.mxcsr};
    };
    services.interpolate=[&](auto object,auto seconds,auto& arithmetic) {
        call(0xbc380u,std::array{seconds},object,arithmetic);
    };
    services.refresh=[&](auto object,auto& arithmetic) {
        call(0xbf050u,std::array{object},0,arithmetic);
    };
    native_tu15_object_scale_hierarchy(world.memory(),object,scale,ticks,fp,services,
        [&](auto at,auto bytes){world.write_source_call_input(at,bytes);});
}
}
