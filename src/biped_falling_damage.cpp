#include "halo2/biped_falling_damage.hpp"
#include "halo2/biped_postphysics_phase.hpp"
#include "halo2/physics_vehicle_update.hpp"
#include <stdexcept>

namespace halo2 {
void source_run_biped_falling_damage_1_1(SourcePhysicsContactMemory &memory,
    std::uint32_t entry, SourceUnitAnimationRuntimeAbi &abi, SourcePhysicsFactoryFloatingPoint fp,
    const SourceUnitAnimationRuntimeServices &services, const SourcePhysicsContactRecordObserver &observe) {
    auto joined=services;
    joined.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &observer) {
        switch(call.target) {
        case 0xdef10:
            source_run_biped_postphysics_phase_1_1(bank,call.target,state,fp,joined,observer);
            return;
        case 0x30bd0:
        case 0xbacc0:
        case 0xe4f40:
            source_run_unit_animation_selection_1_1(bank,call.target,state,fp,joined,observer);
            return;
        case 0xbb0c0: {
            SourcePhysicsContactRuntimeServices physical;
            physical.instruction=joined.instruction;
            if(joined.estimate)physical.reciprocal_sqrt=[&](auto value) {
                return joined.estimate(SourceUnitAnimationEstimate::ReciprocalSquareRoot,value);
            };
            physical.required_owner=[&](auto &m,const auto &request,auto &raw,const auto &o) {
                auto full=state;static_cast<SourcePhysicsContactRuntimeAbi &>(full)=raw;
                try { joined.required_owner(m,request,full,o); }
                catch(...) { raw=static_cast<const SourcePhysicsContactRuntimeAbi &>(full);state.mmx=full.mmx;throw; }
                raw=static_cast<const SourcePhysicsContactRuntimeAbi &>(full);state.mmx=full.mmx;
            };
            source_run_physics_vehicle_update_runtime(bank,call.target,state,
                SourcePhysicsContactImage::MapPack11,fp,physical,observer);
            return;
        }
        default:
            if(!services.required_owner)throw std::runtime_error("Required original falling lower owner unavailable");
            services.required_owner(bank,call,state,observer);
        }
    };
    SourcePhysicsContactRuntimeServices physical;
    physical.instruction=services.instruction;
    physical.required_owner=[&](auto &bank,const auto &call,auto &raw,const auto &observer) {
        auto full=abi;static_cast<SourcePhysicsContactRuntimeAbi &>(full)=raw;
        try { joined.required_owner(bank,call,full,observer); }
        catch(...) { raw=static_cast<const SourcePhysicsContactRuntimeAbi &>(full);abi.mmx=full.mmx;throw; }
        raw=static_cast<const SourcePhysicsContactRuntimeAbi &>(full);abi.mmx=full.mmx;
    };
    source_run_biped_falling_damage_runtime(memory,entry,abi,SourcePhysicsContactImage::MapPack11,
        fp,physical,observe);
}
std::uint32_t source_call_biped_falling_damage_1_1(SourcePhysicsContactMemory &memory,
    const SourceRuntimeCall &call, SourcePhysicsFactoryFloatingPoint fp,
    const SourceUnitAnimationRuntimeServices &services, const SourcePhysicsContactRecordObserver &observe) {
    const bool velocity=call.entry==0xe4d30 && call.eax && call.arguments.empty();
    const bool airborne=call.entry==0xe5700 && call.arguments.size()==2;
    if((!velocity && !airborne) || !call.argument_masks.empty())
        throw std::runtime_error("Missing defined original falling producer ABI");
    SourceUnitAnimationRuntimeAbi abi{};
    abi.gp[4]=call.entry_esp;abi.eflags=2;
    if(velocity)abi.gp[0]=*call.eax;
    for(std::size_t i=0;i<call.arguments.size();++i) {
        std::array<std::byte,4> bytes;
        for(unsigned j=0;j<4;++j)bytes[j]=std::byte(call.arguments[i]>>(8*j));
        memory.write(call.entry_esp+4+4*i,bytes);
    }
    source_run_biped_falling_damage_1_1(memory,call.entry,abi,fp,services,observe);
    if(abi.gp[4]!=call.entry_esp+(velocity?4:12))
        throw std::runtime_error("Original falling producer stack ownership mismatch");
    return abi.gp[0];
}
} // namespace halo2
