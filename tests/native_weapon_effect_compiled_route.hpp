#pragma once
#include "halo2/native_weapon_effect_dispatch_tu15.hpp"
#include <stdexcept>
#include <string>
#ifdef HALO2_NATIVE_EFFECT_COMPILED_ROUTE
#include "halo2/physics_live_support_tu15.hpp"
#include "halo2/source_ai_frame_tu15.hpp"
#include "halo2/source_script_effect_tu15.hpp"
#include "halo2/source_combat_action_tu15.hpp"
#include "halo2/death_cycle_runtime_tu15.hpp"
#include "halo2/source_map_world_support_tu15.hpp"
#include "halo2/source_sandbox_special_tu15.hpp"
#include "halo2/source_damage_delivery_runtime.hpp"
#include "halo2/biped_falling_damage.hpp"
#endif

namespace halo2 {
// The same unchanged-original fixtures qualify a complete exported caller.
// They do not select lower-owner outcomes or relax any per-case comparison.
class NativeWeaponEffectTestRoute {
    std::string module_;
    std::uint32_t entry_{};
public:
    NativeWeaponEffectTestRoute(int argc,char **argv) {
#ifdef HALO2_NATIVE_EFFECT_COMPILED_ROUTE
        if(argc!=4)throw std::runtime_error("Supply original reference, module and entry");
        module_=argv[2];entry_=std::stoul(argv[3],nullptr,16);
        const bool accepted=(module_=="support" && (entry_==0xbbe00 || entry_==0x106f90 || entry_==0x108a70 || entry_==0x1791c0 || entry_==0x179610 || entry_==0x17a1a0 || entry_==0x17dff0 || entry_==0x18be50)) ||
            (module_=="ai" && (entry_==0xbbe00 || entry_==0x106f90)) ||
            (module_=="script" && entry_==0x1791c0) || (module_=="death" && entry_==0x17a1a0) ||
            (module_=="combat" && (entry_==0x106f90 || entry_==0x108a70 || entry_==0x17b0d0 || entry_==0x17dff0)) ||
            (module_=="map" && entry_==0x18c4b0) || (module_=="sandbox" && entry_==0xcabc0) ||
            (module_=="biped" && entry_==0xbbe00) ||
            ((module_=="damage" || module_=="damage-selected") && (entry_==0xbbe00 || entry_==0xcabc0 || entry_==0x1791c0 || entry_==0x179610));
        if(!accepted)throw std::runtime_error("Unknown original compiled caller route");
#else
        (void)argc;(void)argv;
#endif
    }
    bool compiled() const {return !module_.empty();}
    bool accepts(std::uint32_t entry) const {return !compiled() || entry==entry_;}
    unsigned expected_cases() const {
        switch(entry_) {
        case 0xbbe00:case 0x1791c0:return 111;
        case 0x106f90:case 0x108a70:return 268;
        case 0x179610:return 53;
        case 0x17a1a0:return 49;
        case 0x17dff0:return 82;
        case 0x17b0d0:return 92;
        case 0x18be50:return 42;
        case 0x18c4b0:return 23;
        case 0xcabc0:return 171;
        default:throw std::runtime_error("Missing explicit original caller case count");
        }
    }
    bool scope(bool native_scope,unsigned cases) const {
        return compiled()?cases==expected_cases():native_scope;
    }
    // The module's public CPU API has no reset-publication observer. Its
    // raw ABI is checked here; actual1088E5 journal/context routing has its
    // separate live-world test. The native-owner test still checks all events.
    bool observes_reset_decisions() const {return !compiled();}
    void run(SourcePhysicsContactMemory &m,std::uint32_t entry,SourcePhysicsWorldRuntimeAbi &abi,
        const SourcePhysicsWorldRuntimeServices &services,const SourcePhysicsContactRecordObserver &observe={},
        const NativeWeaponZoomResetObserverTu15 &reset={}) const {
#ifdef HALO2_NATIVE_EFFECT_COMPILED_ROUTE
        const auto fp=abi.floating_point.value();
        if(module_=="biped" || module_=="damage" || module_=="damage-selected") {
            std::uint32_t canonical{},last{};
            switch(entry) {
            case 0xbbe00:canonical=0xbacc0;last=0xbbf1c;break;
            case 0xcabc0:canonical=0xc9a40;last=0xcad35;break;
            case 0x1791c0:canonical=0x177da0;last=0x17922b;break;
            case 0x179610:canonical=0x1781f0;last=0x1796f0;break;
            default:throw std::runtime_error("Unknown paired original route");
            }
            const auto delta=entry-canonical,canonical_last=last-delta;
            // Public paired APIs retain canonical lower-owner sites even
            // when their instruction/store observations select TU1.5 PCs.
            auto forwarded=services;auto observer=observe;
            if(module_!="damage-selected") {
                if(services.instruction)forwarded.instruction=[&](auto pc){services.instruction(pc>=canonical && pc<=canonical_last?pc+delta:pc);};
                if(observe)observer=[&](const auto &event){auto actual=event;if(actual.instruction>=canonical && actual.instruction<=canonical_last)actual.instruction+=delta;observe(actual);};
            }
            if(services.required_owner)forwarded.required_owner=[&](auto &memory,const auto &call,auto &state,const auto &stores){
                auto actual=call;if(actual.instruction>=canonical && actual.instruction<=canonical_last)actual.instruction+=delta;
                services.required_owner(memory,actual,state,stores);
            };
            if(module_=="biped")source_run_biped_falling_damage_runtime(m,canonical,abi,SourcePhysicsContactImage::TitleUpdate15,fp,forwarded,observer);
            else if(module_=="damage")source_run_damage_delivery_runtime(m,canonical,abi,SourcePhysicsContactImage::TitleUpdate15,fp,forwarded,observer);
            else source_run_damage_delivery_runtime_selected_observations(m,canonical,abi,SourcePhysicsContactImage::TitleUpdate15,fp,forwarded,observer);
        }
        else if(module_=="support")source_run_physics_live_support_tu15(m,entry,abi,fp,services,observe);
        else if(module_=="ai")source_run_ai_frame_tu15(m,entry,abi,fp,services,observe);
        else if(module_=="script")source_run_script_effect_tu15(m,entry,abi,fp,services,observe);
        else if(module_=="combat")source_run_combat_action_tu15(m,entry,abi,fp,services,observe);
        else if(module_=="death")source_run_death_cycle_runtime_tu15(m,entry,abi,fp,services,observe);
        else if(module_=="map")source_run_map_world_support_tu15(m,entry,abi,fp,services,observe);
        else source_run_sandbox_special_tu15(m,entry,abi,fp,services,observe);
#else
        native_weapon_effect_dispatch_tu15_run(m,entry,abi,services,observe,reset);
#endif
    }
};
}
