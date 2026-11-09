#pragma once
#include "halo2/native_tu15_model_region_state.hpp"
#include "halo2/source_effect_compiled_symbols_tu15.hpp"
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <bit>

namespace halo2::physics_world_runtime_detail {
// Legacy ABI boundary for the complete 1704C0 parent. All candidate records,
// count cells and effect packets stay in their actual source stack DATA.
// Invoke executes each remaining owner on current memory and the real ABI.
template<class State,class Site,class Invoke>
void model_region_state_native_tu15(State& s,Site site,Invoke invoke) {
    struct Storage final:NativeTu15ModelRegionStateStorage {
        State& s;Site& site;Invoke& invoke;
        Storage(State& state,Site& position,Invoke& call):s(state),site(position),invoke(call){}
        std::uint32_t at(std::uint32_t pc){return site(pc);}
        static std::int32_t signed_word(std::uint32_t value){return std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value)));}
        static std::int32_t signed_byte(std::uint32_t value){return std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value)));}
        void call(std::uint32_t pc,std::uint32_t owner,std::uint32_t after){s.push(at(pc),after);invoke(pc,owner);}
        std::int32_t begin()override {
            at(0x1704c0u);s.gp[4]=s.flags.minus(s.gp[4],0x3f4u,4);
            at(0x1704c6u);s.gp[0]=r(s.memory,0x4f0414u,4);
            s.push(at(0x1704cbu),s.gp[6]);
            s.push(at(0x1704ccu),s.gp[7]);
            at(0x1704cdu);s.gp[7]=r(s.memory,s.gp[4]+0x404u,4);
            at(0x1704d4u);s.gp[7]=s.flags.logic(s.gp[7]&0xffffu,4);
            at(0x1704dau);s.gp[7]=s.flags.shift(s.gp[7],4u,4);
            at(0x1704ddu);s.gp[1]=r(s.memory,s.gp[7]+s.gp[0]+8u,4);
            at(0x1704e1u);s.gp[2]=r(s.memory,s.gp[1]+0x70u,4);
            at(0x1704e4u);s.set(0,s.flags.logic(0u,1),1,0);
            at(0x1704e6u);s.gp[6]=s.flags.logic(0u,4);
            at(0x1704e8u);s.flags.logic(s.gp[2],4);
            w(s.memory,s.observe,at(0x1704eau),s.gp[4]+0x14u,4,s.gp[7]);
            w(s.memory,s.observe,at(0x1704eeu),s.gp[4]+0x18u,4,s.gp[1]);
            w(s.memory,s.observe,at(0x1704f2u),s.gp[4]+0xbu,1,s.gp[0]&255u);
            w(s.memory,s.observe,at(0x1704f6u),s.gp[4]+0x10u,4,s.gp[6]);
            at(0x1704fau);return std::bit_cast<std::int32_t>(s.gp[2]);
        }
        void enter_regions()override {
            s.push(at(0x170500u),s.gp[3]);
            s.push(at(0x170501u),s.gp[5]);
            at(0x170502u);s.gp[5]=r(s.memory,s.gp[4]+0x42cu,4);
            at(0x170509u);s.gp[5]=s.flags.increment(s.gp[5],4,false);
            at(0x17050au);
        }
        std::uint32_t requested_state()override {
            at(0x170510u);s.gp[0]=r(s.memory,s.gp[4]+0x418u,4);
            at(0x170517u);s.flags.minus(s.gp[0],0xffffffffu,4);
            at(0x17051au);return s.gp[0];
        }
        void retain_current_state()override {
            at(0x17051cu);s.gp[1]=std::uint32_t(signed_byte(r(s.memory,s.gp[5],1)));
            w(s.memory,s.observe,at(0x170520u),s.gp[4]+0x14u,4,s.gp[1]);at(0x170524u);
        }
        void use_requested_state()override {w(s.memory,s.observe,at(0x170526u),s.gp[4]+0x14u,4,s.gp[0]);}
        std::uint32_t property_bit()override {
            at(0x17052au);s.gp[0]=r(s.memory,s.gp[4]+0x420u,4);
            at(0x170531u);s.flags.minus(s.gp[0],0xffffffffu,4);
            at(0x170534u);s.gp[2]=r(s.memory,s.gp[5]+1u,1);
            at(0x170538u);return s.gp[0];
        }
        bool property_region_selected()override {
            at(0x17053au);s.gp[1]=s.gp[6];
            at(0x17053cu);s.gp[3]=1u;
            at(0x170541u);s.gp[3]=s.flags.shift(s.gp[3],s.gp[1]&255u,4);
            at(0x170543u);s.gp[1]=std::uint32_t(signed_word(r(s.memory,s.gp[4]+0x424u,2)));
            at(0x17054bu);const bool selected=(s.gp[1]&s.gp[3])!=0;s.flags.logic(s.gp[1]&s.gp[3],4);
            at(0x17054du);s.gp[1]=s.gp[0];
            at(0x17054fu);s.gp[3]=1u;
            at(0x170554u);return selected;
        }
        void set_property_bit()override {
            at(0x170556u);s.gp[3]=s.flags.shift(s.gp[3],s.gp[1]&255u,4);
            at(0x170558u);s.gp[2]=s.flags.logic(s.gp[2]|s.gp[3],4);at(0x17055au);
        }
        void clear_property_bit()override {
            at(0x17055cu);s.gp[3]=s.flags.shift(s.gp[3],s.gp[1]&255u,4);
            at(0x17055eu);s.gp[3]=~s.gp[3];
            at(0x170560u);s.gp[2]=s.flags.logic(s.gp[2]&s.gp[3],4);
        }
        NativeTu15ModelRegionStateComparison state_comparison()override {
            at(0x170562u);s.gp[0]=std::uint32_t(signed_byte(r(s.memory,s.gp[5],1)));
            at(0x170566u);s.gp[3]=r(s.memory,s.gp[4]+0x14u,4);
            at(0x17056au);s.flags.minus(s.gp[3],s.gp[0],4);
            at(0x17056cu);return {std::bit_cast<std::int32_t>(s.gp[3]),std::bit_cast<std::int32_t>(s.gp[0])};
        }
        std::uint8_t allow_lower_state()override {
            at(0x17056eu);const auto value=r(s.memory,s.gp[4]+0x41cu,1);s.set(0,value,1,0);
            at(0x170575u);s.flags.logic(value,1);at(0x170577u);return std::uint8_t(value);
        }
        std::uint32_t region_filter()override {
            at(0x17057du);s.gp[0]=r(s.memory,s.gp[4]+0x414u,4);
            at(0x170584u);s.flags.minus(s.gp[0],0xffffffffu,4);at(0x170587u);return s.gp[0];
        }
        bool region_matches_filter()override {
            at(0x170589u);s.flags.minus(s.gp[0],s.gp[6],4);at(0x17058bu);return s.gp[0]==s.gp[6];
        }
        std::uint32_t model_reference()override {
            at(0x170591u);s.gp[1]=r(s.memory,s.gp[4]+0x40cu,4);const auto model=s.gp[1];
            at(0x170598u);s.gp[0]=s.flags.logic(0xffffffffu,4);
            at(0x17059bu);s.flags.minus(s.gp[1],0xffffffffu,4);
            at(0x17059eu);s.gp[1]=r(s.memory,s.gp[4]+0x410u,4);
            at(0x1705a5u);return model;
        }
        std::uint32_t variant_index()override {
            at(0x1705a7u);s.flags.minus(s.gp[1],0xffffffffu,4);at(0x1705aau);return s.gp[1];
        }
        std::uint32_t mapping_region_index()override {
            at(0x1705acu);s.flags.minus(s.gp[6],0xffffffffu,4);at(0x1705afu);return s.gp[6];
        }
        void lookup_mapping()override {
            at(0x1705b1u);s.gp[0]=r(s.memory,0x4f0414u,4);
            at(0x1705b6u);s.gp[0]=r(s.memory,s.gp[7]+s.gp[0]+8u,4);
            at(0x1705bau);s.gp[0]=r(s.memory,s.gp[0]+0x54u,4);
            at(0x1705bdu);s.gp[7]=s.gp[1];
            at(0x1705bfu);s.gp[7]=s.flags.product(s.gp[7],0x38u);
            at(0x1705c2u);s.gp[0]=s.flags.plus(s.gp[0],s.gp[7],4);
            at(0x1705c4u);s.gp[0]=std::uint32_t(signed_byte(r(s.memory,s.gp[0]+s.gp[6]+4u,1)));
        }
        std::int32_t mapping_and_clear_candidates()override {
            at(0x1705c9u);s.flags.minus(s.gp[0],0xffffffffu,4);
            w(s.memory,s.observe,at(0x1705ccu),s.gp[4]+0x14u,4,0u);
            at(0x1705d4u);return std::bit_cast<std::int32_t>(s.gp[0]);
        }
        void collect_candidates()override {
            s.push(at(0x1705d6u),32u);
            at(0x1705d8u);s.gp[1]=s.flags.product(s.gp[1],0x38u);
            s.push(at(0x1705dbu),0u);
            s.push(at(0x1705ddu),s.gp[2]);
            at(0x1705deu);s.gp[2]=std::uint32_t(signed_byte(r(s.memory,s.gp[5]-1u,1)));
            s.push(at(0x1705e2u),s.gp[3]);
            s.push(at(0x1705e3u),s.gp[2]);
            at(0x1705e4u);s.gp[2]=r(s.memory,s.gp[4]+0x34u,4);
            at(0x1705e8u);s.gp[2]=r(s.memory,s.gp[2]+0x54u,4);
            at(0x1705ebu);s.gp[1]=r(s.memory,s.gp[2]+s.gp[1]+0x18u,4);
            at(0x1705efu);s.gp[0]*=5u;
            at(0x1705f2u);s.gp[2]=s.gp[1]+s.gp[0]*4u;
            s.push(at(0x1705f5u),s.gp[2]);
            at(0x1705f6u);s.gp[7]=s.gp[4]+0x9cu;
            at(0x1705fdu);s.gp[2]=s.gp[4]+0x2cu;
            call(0x170601u,0x170930u,0x170606u);
        }
        std::uint32_t select_candidate()override {
            at(0x170606u);s.gp[2]=r(s.memory,s.gp[4]+0x14u,4);
            at(0x17060au);s.gp[3]=s.gp[4]+0x84u;
            call(0x170611u,0x170b40u,0x170616u);
            at(0x170616u);s.flags.minus(s.gp[0],0xffffffffu,4);at(0x170619u);return s.gp[0];
        }
        NativeTu15ModelRegionStateComparison write_permutations()override {
            at(0x17061fu);s.gp[1]=r(s.memory,s.gp[4]+0x18u,4);
            at(0x170623u);s.gp[0]=s.flags.product(s.gp[0],0x1cu);
            at(0x170626u);s.gp[2]=r(s.memory,s.gp[4]+0x428u,4);
            at(0x17062du);s.gp[6]=s.gp[4]+s.gp[0]+0x84u;
            at(0x170634u);s.set(0,r(s.memory,s.gp[6],1),1,0);
            w(s.memory,s.observe,at(0x170636u),s.gp[1]+s.gp[2],1,s.gp[0]&255u);
            at(0x170639u);s.set(0,r(s.memory,s.gp[6]+4u,1),1,0);
            at(0x17063cu);s.gp[1]=std::uint32_t(signed_byte(r(s.memory,s.gp[5],1)));
            w(s.memory,s.observe,at(0x170640u),s.gp[5]-1u,1,s.gp[0]&255u);
            at(0x170643u);const auto desired=r(s.memory,s.gp[6]+8u,4);s.flags.minus(s.gp[1],desired,4);
            at(0x170646u);return {std::bit_cast<std::int32_t>(desired),std::bit_cast<std::int32_t>(s.gp[1])};
        }
        std::uint32_t old_effect()override {
            at(0x170648u);s.gp[7]=r(s.memory,s.gp[5]+3u,4);
            at(0x17064bu);s.flags.minus(s.gp[7],0xffffffffu,4);at(0x17064eu);return s.gp[7];
        }
        void stop_effect()override {
            s.push(at(0x170650u),1u);call(0x170652u,0x17a0f0u,0x170657u);
        }
        std::uint32_t new_effect_tag()override {
            at(0x170657u);s.gp[7]=r(s.memory,s.gp[6]+0x10u,4);
            at(0x17065au);s.flags.minus(s.gp[7],0xffffffffu,4);at(0x17065du);return s.gp[7];
        }
        void prepare_effect()override {
            at(0x17065fu);s.gp[2]=s.gp[4]+0x24u;call(0x170663u,0x178d80u,0x170668u);
        }
        void create_effect()override {
            at(0x170668u);s.gp[0]=r(s.memory,s.gp[6]+0x14u,4);
            at(0x17066bu);s.gp[2]=r(s.memory,s.gp[4]+0x408u,4);
            w(s.memory,s.observe,at(0x170672u),s.gp[4]+0x54u,4,s.gp[0]);
            at(0x170676u);s.gp[0]=source_object_function_compiled_symbol_tu15(s.memory,0x170676u,0x30005a6u);
            at(0x17067bu);s.gp[1]=s.gp[4]+0x24u;
            s.push(at(0x17067fu),s.gp[1]);
            w(s.memory,s.observe,at(0x170680u),s.gp[4]+0x2cu,4,s.gp[7]);
            w(s.memory,s.observe,at(0x170684u),s.gp[4]+0x30u,4,s.gp[2]);
            w(s.memory,s.observe,at(0x170688u),s.gp[4]+0x5cu,4,s.gp[0]);
            w(s.memory,s.observe,at(0x17068cu),s.gp[4]+0x60u,4,s.gp[0]);
            w(s.memory,s.observe,at(0x170690u),s.gp[4]+0x28u,4,3u);
            call(0x170698u,0x178e30u,0x17069du);
            w(s.memory,s.observe,at(0x17069du),s.gp[5]+3u,4,s.gp[0]);
        }
        void commit_state()override {
            at(0x1706a0u);s.set(2,r(s.memory,s.gp[6]+8u,1),1,0);
            at(0x1706a3u);s.set(0,r(s.memory,s.gp[6]+0xcu,1),1,0);
            at(0x1706a6u);s.gp[6]=r(s.memory,s.gp[4]+0x18u,4);
            w(s.memory,s.observe,at(0x1706aau),s.gp[5],1,s.gp[2]&255u);
            w(s.memory,s.observe,at(0x1706adu),s.gp[5]+1u,1,s.gp[0]&255u);
            w(s.memory,s.observe,at(0x1706b0u),s.gp[4]+0x13u,1,1u);
        }
        void restore_model_index()override {at(0x1706b5u);s.gp[7]=r(s.memory,s.gp[4]+0x1cu,4);}
        bool advance_region()override {
            at(0x1706b9u);s.gp[1]=r(s.memory,s.gp[4]+0x20u,4);
            at(0x1706bdu);s.gp[0]=r(s.memory,s.gp[1]+0x70u,4);
            at(0x1706c0u);s.gp[6]=s.flags.increment(s.gp[6],4,false);
            at(0x1706c1u);s.gp[5]=s.flags.plus(s.gp[5],8u,4);
            at(0x1706c4u);s.flags.minus(s.gp[6],s.gp[0],4);
            w(s.memory,s.observe,at(0x1706c6u),s.gp[4]+0x18u,4,s.gp[6]);
            at(0x1706cau);return std::bit_cast<std::int32_t>(s.gp[6])<std::bit_cast<std::int32_t>(s.gp[0]);
        }
        void finish(bool entered)override {
            if(entered){
                at(0x1706d0u);s.set(0,r(s.memory,s.gp[4]+0x13u,1),1,0);
                at(0x1706d4u);s.gp[5]=s.pop();at(0x1706d5u);s.gp[3]=s.pop();
            }
            at(0x1706d6u);s.gp[7]=s.pop();at(0x1706d7u);s.gp[6]=s.pop();
            at(0x1706d8u);s.gp[4]=s.flags.plus(s.gp[4],0x3f4u,4);
            at(0x1706deu);s.pop();s.gp[4]+=40u;
        }
    } storage(s,site,invoke);
    native_tu15_model_region_state(storage);
}
}
