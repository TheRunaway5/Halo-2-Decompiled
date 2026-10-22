#include "halo2/source_weapon_animation_query_symbols_tu15.hpp"
#include "halo2/physics_contact_record.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace halo2;
namespace {
struct Memory final:SourcePhysicsContactMemory {
    void read(std::uint32_t,std::span<std::byte>)const override {throw std::runtime_error("Query namespace must not read authored bytes");}
    void write(std::uint32_t,std::span<const std::byte>)override {throw std::runtime_error("Query namespace must not write authored bytes");}
};
void need(bool ok){if(!ok)throw std::runtime_error("Weapon query namespace control failed");}
struct ShiftedActivity {std::uint32_t original,mcc;const char *name;};
constexpr std::array shifted_fp_activities{
    ShiftedActivity{0x0e000607u,0x0e000613u,"melee_strike_1"},
    ShiftedActivity{0x0e000608u,0x0e000614u,"melee_strike_2"},
    ShiftedActivity{0x0e000609u,0x0e000615u,"melee_strike_3"},
    ShiftedActivity{0x0e00060au,0x0e000616u,"melee_strike_4"},
    ShiftedActivity{0x0a0005bbu,0x0a0005c7u,"melee_dash"},
    ShiftedActivity{0x0b0005b2u,0x0b0005beu,"melee_lunge"},
    ShiftedActivity{0x130005bcu,0x130005c8u,"melee_dash_airborne"},
    ShiftedActivity{0x140005b3u,0x140005bfu,"melee_lunge_airborne"},
    ShiftedActivity{0x0c0005aeu,0x0c0005bau,"reload_enter"},
    ShiftedActivity{0x150005afu,0x150005bbu,"reload_continue_empty"},
    ShiftedActivity{0x140005b0u,0x140005bcu,"reload_continue_full"},
    ShiftedActivity{0x0b0005b1u,0x0b0005bdu,"reload_exit"}
};
std::string names(std::uint32_t id) {
    if(id==0x04000557u)return "dual";
    for(const auto &activity:shifted_fp_activities)if(id==activity.mcc)return activity.name;
    throw std::runtime_error("Foreign named namespace input");
}
template<class F>void rejects(F &&f) {bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}need(rejected);}
}
int main()try {
    Memory a,b;
    constexpr std::array equip_sites{0xead6au,0x1124b7u,0x1125d6u};
    for(auto pc:equip_sites)
        need(source_weapon_animation_equip_class_symbol_tu15(a,pc,0x0400054bu)==0x0400054bu);
    for(auto caller:{0xebf41u,0xec154u,0xec230u})
        need(source_weapon_animation_query_symbol_tu15(a,caller,0x0e000607u)==0x0e000607u);
    {SourceWeaponAnimationQuerySymbolsTu15 original(a,false,{});
        need(source_weapon_animation_query_symbol_tu15(a,0xebf41u,0x0e000607u)==0x0e000607u);
        for(std::size_t index=8;index<shifted_fp_activities.size();++index) {
            const auto &activity=shifted_fp_activities[index];
            need(source_weapon_animation_query_symbol_tu15(a,0x16b45du,activity.original)==activity.original);
        }
        SourceWeaponAnimationQueryScopeTu15 scope(a,0x169a56u,0x0e000607u);
        need(source_weapon_animation_scoped_query_symbol_tu15(a,0x0e000607u)==0x0e000607u);}
    rejects([&]{SourceWeaponAnimationQuerySymbolsTu15 missing(a,true,{});});
    for(const auto &activity:shifted_fp_activities)
        rejects([&]{SourceWeaponAnimationQuerySymbolsTu15 foreign(a,true,[&](auto id){return id==activity.mcc?"foreign":names(id);});});
    rejects([&]{SourceWeaponAnimationQuerySymbolsTu15 foreign(a,true,[](auto id){return id==0x04000557u?"stun":names(id);});});
    {
        SourceWeaponAnimationQuerySymbolsTu15 lease(a,true,names);
        for(auto pc:equip_sites) {
            need(source_weapon_animation_equip_class_symbol_tu15(a,pc,0x0400054bu)==0x04000557u);
            need(source_weapon_animation_equip_class_symbol_tu15(b,pc,0x0400054bu)==0x0400054bu);
            for(auto key:{0x04000557u,0x030000d9u,0x0e000607u,0xffffffffu})
                need(source_weapon_animation_equip_class_symbol_tu15(a,pc,key)==key);
            for(auto neighbor:{pc-1u,pc+1u})
                need(source_weapon_animation_equip_class_symbol_tu15(a,neighbor,0x0400054bu)==0x0400054bu);
        }
        need(source_weapon_animation_query_symbol_tu15(a,0x16b45du,0x0400054bu)==0x0400054bu);
        for(auto caller:{0xebf41u,0xec154u,0xec230u})for(unsigned index=0;index<4;++index) {
            need(source_weapon_animation_query_symbol_tu15(a,caller,0x0e000607u+index)==0x0e000613u+index);
            need(source_weapon_animation_query_symbol_tu15(b,caller,0x0e000607u+index)==0x0e000607u+index);
            need(source_weapon_animation_query_symbol_tu15(a,caller,0x0e000613u+index)==0x0e000613u+index);
        }
        for(auto caller:{0u,0xebf40u,0xec153u,0xec22fu,0x16b45du})
            need(source_weapon_animation_query_symbol_tu15(a,caller,0x0e000607u)==0x0e000607u);
        // Original16B458 forwards the magazine's canonical staged activity to
        // its timing lookup. Only these four named keys qualify this caller.
        for(std::size_t index=8;index<shifted_fp_activities.size();++index) {
            const auto &activity=shifted_fp_activities[index];
            need(source_weapon_animation_query_symbol_tu15(a,0x16b45du,activity.original)==activity.mcc);
            need(source_weapon_animation_query_symbol_tu15(b,0x16b45du,activity.original)==activity.original);
            need(source_weapon_animation_query_symbol_tu15(a,0x16b45cu,activity.original)==activity.original);
            need(source_weapon_animation_query_symbol_tu15(a,0x16b45eu,activity.original)==activity.original);
            need(source_weapon_animation_query_symbol_tu15(a,0x16b45du,activity.mcc)==activity.mcc);
        }
        for(auto id:{0x0500000au,0x0c000073u,0x030000d9u,0xffffffffu})
            need(source_weapon_animation_query_symbol_tu15(a,0xebf41u,id)==id);
        rejects([&]{SourceWeaponAnimationQuerySymbolsTu15 duplicate(a,true,names);});
        for(const auto &activity:shifted_fp_activities) {
            SourceWeaponAnimationQueryScopeTu15 scope(a,0x169a56u,activity.original);
            need(source_weapon_animation_scoped_query_symbol_tu15(a,activity.original)==activity.mcc);
            need(source_weapon_animation_scoped_query_symbol_tu15(a,activity.mcc)==activity.mcc);
            // The EBF20 availability boundary still admits only its four
            // candidate activities, even while another FP request is active.
            if(activity.original<0x0e000607u || activity.original>0x0e00060au)
                need(source_weapon_animation_query_symbol_tu15(a,0xebf41u,activity.original)==activity.original);
        }
        for(unsigned index=0;index<4;++index) {
            const auto canonical=0x0e000607u+index;
            need(source_weapon_animation_scoped_query_symbol_tu15(a,canonical)==canonical);
            {
                SourceWeaponAnimationQueryScopeTu15 scope(a,0x169a56u,canonical);
                need(source_weapon_animation_scoped_query_symbol_tu15(a,canonical)==0x0e000613u+index);
                need(source_weapon_animation_scoped_query_symbol_tu15(b,canonical)==canonical);
                for(auto key:{0x0500000au,0x0e000613u,0xffffffffu})
                    need(source_weapon_animation_scoped_query_symbol_tu15(a,key)==key);
                {
                    SourceWeaponAnimationQueryScopeTu15 foreign(a,0x169a55u,canonical);
                    need(source_weapon_animation_scoped_query_symbol_tu15(a,canonical)==canonical);
                }
                need(source_weapon_animation_scoped_query_symbol_tu15(a,canonical)==0x0e000613u+index);
                const auto other=0x0e000607u+(index+1u)%4u;
                {
                    SourceWeaponAnimationQueryScopeTu15 nested(a,0x169a56u,other);
                    need(source_weapon_animation_scoped_query_symbol_tu15(a,other)==other+12u);
                    need(source_weapon_animation_scoped_query_symbol_tu15(a,canonical)==canonical);
                }
                std::uint32_t threaded{};
                std::thread worker([&]{threaded=source_weapon_animation_scoped_query_symbol_tu15(a,canonical);});
                worker.join();need(threaded==canonical);
            }
            need(source_weapon_animation_scoped_query_symbol_tu15(a,canonical)==canonical);
        }
        try {SourceWeaponAnimationQueryScopeTu15 scope(a,0x169a56u,0x0e000607u);throw 1;}
        catch(int) {}
        need(source_weapon_animation_scoped_query_symbol_tu15(a,0x0e000607u)==0x0e000607u);
    }
    need(source_weapon_animation_query_symbol_tu15(a,0xebf41u,0x0e000607u)==0x0e000607u);
    for(auto pc:equip_sites)
        need(source_weapon_animation_equip_class_symbol_tu15(a,pc,0x0400054bu)==0x0400054bu);
    for(std::size_t index=8;index<shifted_fp_activities.size();++index) {
        const auto &activity=shifted_fp_activities[index];
        need(source_weapon_animation_query_symbol_tu15(a,0x16b45du,activity.original)==activity.original);
    }
    {SourceWeaponAnimationQuerySymbolsTu15 replacement(a,true,names);
        need(source_weapon_animation_query_symbol_tu15(a,0xebf41u,0x0e000607u)==0x0e000613u);}
    {
        auto lease=std::make_unique<SourceWeaponAnimationQuerySymbolsTu15>(a,true,names);
        SourceWeaponAnimationQueryScopeTu15 scope(a,0x169a56u,0x0e000607u);
        need(source_weapon_animation_scoped_query_symbol_tu15(a,0x0e000607u)==0x0e000613u);
        lease.reset();
        need(source_weapon_animation_scoped_query_symbol_tu15(a,0x0e000607u)==0x0e000607u);
        SourceWeaponAnimationQuerySymbolsTu15 replacement(a,true,names);
        need(source_weapon_animation_scoped_query_symbol_tu15(a,0x0e000607u)==0x0e000607u);
        {SourceWeaponAnimationQueryScopeTu15 current(a,0x169a56u,0x0e000607u);
            need(source_weapon_animation_scoped_query_symbol_tu15(a,0x0e000607u)==0x0e000613u);}
        need(source_weapon_animation_scoped_query_symbol_tu15(a,0x0e000607u)==0x0e000607u);
    }
    std::cout<<"PASS compiled melee/FP callers,named admission,nested/thread/bank/lifetime isolation and unchanged raw/original requests\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
