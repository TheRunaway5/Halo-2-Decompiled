#include "halo2/native_player_effect_tick_tu15.hpp"
#include "halo2/physics_world_runtime.hpp"
#include <array>
#include <stdexcept>
namespace halo2 {
void native_player_effect_tick_tu15_run(SourcePhysicsContactMemory& memory,
    SourcePhysicsWorldRuntimeAbi& abi,const SourcePhysicsContactRecordObserver& observe) {
    if(!abi.floating_point || abi.active_mmx || abi.x87_depth>7)
        throw std::invalid_argument("Player-effect tick requires retained FP and one private x87 operand");
    NativeTu15PhysicsFrameFp fp{abi.floating_point->x87_control_word,abi.x87_status,abi.floating_point->mxcsr};
    struct Publication {
        SourcePhysicsWorldRuntimeAbi& abi;NativeTu15PhysicsFrameFp& fp;
        ~Publication(){abi.floating_point=SourcePhysicsFactoryFloatingPoint{fp.control_word,fp.mxcsr};abi.x87_status=fp.status;}
    } publication{abi,fp};
    native_player_effect_tick_tu15(memory,fp,observe);
    std::array<std::byte,4> returned;memory.read(abi.gp[4],returned);abi.gp[4]+=4u;
}
}
