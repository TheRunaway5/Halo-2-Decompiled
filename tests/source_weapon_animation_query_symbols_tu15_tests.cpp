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
struct BodyAction {std::uint32_t original,mcc,caller;const char *name;};
constexpr std::array body_actions{
    BodyAction{0x0b00053e,0x0b00054a,0xe9a83,"fire_1_left"},
    BodyAction{0x0b00053f,0x0b00054b,0xe9a9d,"fire_2_left"},
    BodyAction{0x0e00053c,0x0e000548,0xe9ab7,"chamber_1_left"},
    BodyAction{0x0e00053d,0x0e000549,0xe9ad1,"chamber_2_left"},
    BodyAction{0x0e000540,0x0e00054c,0xe9aeb,"charged_1_left"},
    BodyAction{0x0e000541,0x0e00054d,0xe9b05,"charged_2_left"},
    BodyAction{0x0d00053a,0x0d000546,0xe97a9,"reload_1_left"},
    BodyAction{0x0d00053b,0x0d000547,0xe97a9,"reload_2_left"},
    BodyAction{0x1000067e,0x1000068a,0xe9b6d,"overheating_left"},
    BodyAction{0x0f00067f,0x0f00068b,0xe9b87,"overheated_left"},
    BodyAction{0x0d000680,0x0d00068c,0xe9ba1,"o_h_exit_left"},
    BodyAction{0x0a000537,0x0a000543,0xeaef3,"ready_left"},
    BodyAction{0x15000538,0x15000544,0xeaec2,"ready_left_overheated"},
    BodyAction{0x0d000539,0x0d000545,0xeab2e,"put_away_left"},
    BodyAction{0x0a00022d,0x0a00023b,0xee9ae,"smash_left"},
    BodyAction{0x0b00022e,0x0b00023c,0xee9ae,"smash_right"}};
std::string names(std::uint32_t id) {
    if(id==0x04000557u)return "dual";
    if(id==0x0b000556u)return "arm_offsets";
    if(id==0x04000617u)return "grip";
    for(const auto &action:body_actions)if(id==action.mcc)return action.name;
    for(const auto &activity:shifted_fp_activities)if(id==activity.mcc)return activity.name;
    throw std::runtime_error("Foreign named namespace input");
}
template<class F>void rejects(F &&f) {bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}need(rejected);}
}
int main()try {
    Memory a,b;
    constexpr std::array body_sites{
        std::array{0x115ccfu,0x0b00054au,0x0b000556u},std::array{0x115ce8u,0x0b00054au,0x0b000556u},
        std::array{0x115d0cu,0x0400060bu,0x04000617u},std::array{0x115d2au,0x0400060bu,0x04000617u},
        std::array{0x115d68u,0x0400060bu,0x04000617u},std::array{0x115d83u,0x0400060bu,0x04000617u},
        std::array{0x115cd4u,0x0400054bu,0x04000557u},std::array{0x115cedu,0x0400054bu,0x04000557u},
        std::array{0x115d11u,0x0400054bu,0x04000557u},std::array{0x115d2fu,0x0400054bu,0x04000557u}};
    for(const auto [pc,original,mcc]:body_sites)
        need(source_weapon_animation_body_query_symbol_tu15(a,pc,original)==original);
    for(auto caller:{0xebf41u,0xec154u,0xec230u})
        need(source_weapon_animation_query_symbol_tu15(a,caller,0x0e000607u)==0x0e000607u);
    {SourceWeaponAnimationQuerySymbolsTu15 original(a,false,{});
        need(source_weapon_animation_query_symbol_tu15(a,0xebf41u,0x0e000607u)==0x0e000607u);
        for(std::size_t index=8;index<shifted_fp_activities.size();++index) {
            const auto &activity=shifted_fp_activities[index];
            need(source_weapon_animation_query_symbol_tu15(a,0x16b45du,activity.original)==activity.original);
        }
        SourceWeaponAnimationQueryScopeTu15 scope(a,0x169a56u,0x0e000607u);
        need(source_weapon_animation_scoped_query_symbol_tu15(a,0x0e000607u)==0x0e000607u);
        need(source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu)==0x0400054bu);}
    rejects([&]{SourceWeaponAnimationQuerySymbolsTu15 missing(a,true,{});});
    for(const auto &activity:shifted_fp_activities)
        rejects([&]{SourceWeaponAnimationQuerySymbolsTu15 foreign(a,true,[&](auto id){return id==activity.mcc?"foreign":names(id);});});
    rejects([&]{SourceWeaponAnimationQuerySymbolsTu15 foreign(a,true,[](auto id){return id==0x04000557u?"stun":names(id);});});
    for(const auto id:{0x0b000556u,0x04000617u})
        rejects([&]{SourceWeaponAnimationQuerySymbolsTu15 foreign(a,true,[&](auto key){return key==id?"foreign":names(key);});});
    {
        SourceWeaponAnimationQuerySymbolsTu15 lease(a,true,names);
        for(const auto [caller,activity]:std::array{
            std::pair{0x16b0d6u,0x12000068u},
            std::pair{0x16b0eau,0x08000061u},std::pair{0x16b10du,0x0a000069u},
            std::pair{0x16b12du,0x0e0000deu}}) {
            SourceWeaponAnimationQueryScopeTu15 extra(a,caller,activity);
            need(source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu)==0x04000557u);
            need(source_weapon_animation_scoped_class_symbol_tu15(b,0x0400054bu)==0x0400054bu);
            need(source_weapon_animation_scoped_query_symbol_tu15(a,activity)==activity);
            {
                SourceWeaponAnimationQueryScopeTu15 wrong_activity(a,caller,activity+1u);
                need(source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu)==0x0400054bu);
            }
            {
                SourceWeaponAnimationQueryScopeTu15 wrong_caller(a,caller-1u,activity);
                need(source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu)==0x0400054bu);
            }
        }
        for(const auto &action:body_actions) {
            {
                SourceWeaponAnimationQueryScopeTu15 bare(a,0x116654u,action.original);
                need(source_weapon_animation_scoped_overlay_symbol_tu15(a,action.original)==action.original);
            }
            {
                SourceWeaponAnimationQueryScopeTu15 foreign_origin(a,action.caller-1u,action.original);
                SourceWeaponAnimationQueryScopeTu15 overlay(a,0x116654u,action.original);
                need(source_weapon_animation_scoped_overlay_symbol_tu15(a,action.original)==action.original);
            }
            SourceWeaponAnimationQueryScopeTu15 origin(a,action.caller,action.original);
            need(source_weapon_animation_scoped_overlay_symbol_tu15(a,action.original)==action.original);
            need(source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu)==0x0400054bu);
            {
                SourceWeaponAnimationQueryScopeTu15 overlay(a,0x116654u,action.original);
                need(source_weapon_animation_scoped_overlay_symbol_tu15(a,action.original)==action.mcc);
                need(source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu)==0x04000557u);
                need(source_weapon_animation_scoped_overlay_symbol_tu15(b,action.original)==action.original);
                need(source_weapon_animation_scoped_overlay_symbol_tu15(a,action.mcc)==action.mcc);
                {
                    SourceWeaponAnimationQueryScopeTu15 unqualified(a,0x12345678u,action.original);
                    need(source_weapon_animation_scoped_overlay_symbol_tu15(a,action.original)==action.original);
                }
                std::uint32_t threaded{};
                std::thread worker([&]{threaded=source_weapon_animation_scoped_overlay_symbol_tu15(a,action.original);});
                worker.join();need(threaded==action.original);
            }
            {
                SourceWeaponAnimationQueryScopeTu15 fallback(a,0x1166e4u,action.original);
                SourceWeaponAnimationQueryScopeTu15 playback(a,0x111bceu,action.original);
                need(source_weapon_animation_scoped_query_symbol_tu15(a,action.original)==action.mcc);
                need(source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu)==0x04000557u);
            }
            {
                SourceWeaponAnimationQueryScopeTu15 missing_fallback(a,0x111bceu,action.original);
                need(source_weapon_animation_scoped_query_symbol_tu15(a,action.original)==action.original);
            }
        }
        for(const auto [pc,original,mcc]:body_sites) {
            need(source_weapon_animation_body_query_symbol_tu15(a,pc,original)==mcc);
            need(source_weapon_animation_body_query_symbol_tu15(b,pc,original)==original);
            for(const auto neighbor:{pc-1u,pc+1u})
                need(source_weapon_animation_body_query_symbol_tu15(a,neighbor,original)==original);
            for(const auto key:{mcc,0x030000d9u,0xffffffffu})
                need(source_weapon_animation_body_query_symbol_tu15(a,pc,key)==key);
        }
        for(const auto caller:{0x169a56u,0x16b402u})
            for(const auto activity:{0x0400000cu,0x06000006u,0x0b000065u,0x05000024u,0x0c0005aeu}) {
                SourceWeaponAnimationQueryScopeTu15 scope(a,caller,activity);
                need(source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu)==0x04000557u);
                need(source_weapon_animation_scoped_class_symbol_tu15(b,0x0400054bu)==0x0400054bu);
                for(const auto key:{0x04000557u,0x030000d9u,0x07000101u,0xffffffffu})
                    need(source_weapon_animation_scoped_class_symbol_tu15(a,key)==key);
                {
                    SourceWeaponAnimationQueryScopeTu15 foreign(a,caller-1u,activity);
                    need(source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu)==0x0400054bu);
                }
                need(source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu)==0x04000557u);
                if(caller==0x16b402u)
                    need(source_weapon_animation_scoped_query_symbol_tu15(a,activity)==activity);
                std::uint32_t threaded{};
                std::thread worker([&]{threaded=source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu);});
                worker.join();need(threaded==0x0400054bu);
            }
        need(source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu)==0x0400054bu);
        // The original normalizer must receive canonical dual. Only these
        // actual availability/setter scopes may translate a copied search key.
        for(const auto [caller,key]:std::array{
            std::pair{0x112406u,0x0400054bu},std::pair{0x1124e8u,0x07000101u},
            std::pair{0x1125e0u,0x0400054bu}}) {
            SourceWeaponAnimationQueryScopeTu15 equip(a,caller,key);
            need(source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu)==0x04000557u);
            need(source_weapon_animation_scoped_class_symbol_tu15(b,0x0400054bu)==0x0400054bu);
            need(source_weapon_animation_scoped_query_symbol_tu15(a,key)==key);
            for(const auto &action:body_actions)
                need(source_weapon_animation_scoped_query_symbol_tu15(a,action.original)==action.original);
            {
                SourceWeaponAnimationQueryScopeTu15 wrong_key(a,caller,key+1u);
                need(source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu)==0x0400054bu);
            }
            for(const auto neighbor:{caller-1u,caller+1u}) {
                SourceWeaponAnimationQueryScopeTu15 wrong_caller(a,neighbor,key);
                need(source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu)==0x0400054bu);
            }
            need(source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu)==0x04000557u);
        }
        need(source_weapon_animation_query_symbol_tu15(a,0x16b45du,0x0400054bu)==0x0400054bu);
        for(auto caller:{0xebf41u,0xec154u,0xec230u})for(unsigned index=0;index<4;++index) {
            need(source_weapon_animation_query_symbol_tu15(a,caller,0x0e000607u+index)==0x0e000613u+index);
            need(source_weapon_animation_query_symbol_tu15(b,caller,0x0e000607u+index)==0x0e000607u+index);
            need(source_weapon_animation_query_symbol_tu15(a,caller,0x0e000613u+index)==0x0e000613u+index);
        }
        for(auto caller:{0u,0xebf40u,0xec153u,0xec22fu,0x16b45du})
            need(source_weapon_animation_query_symbol_tu15(a,caller,0x0e000607u)==0x0e000607u);
        // EBFA0's dash branch has no impact event. Its kind0 release selects
        // one of these canonical lunge activities, then EC22B asks16B31B for
        // the strike's actual metadata. Only that copied timing-query operand
        // may enter the MCC namespace; retained source activities stay raw.
        for(const auto index:{5u,7u}) {
            const auto &activity=shifted_fp_activities[index];
            for(const auto caller:{0xebf41u,0xec154u,0xec22fu,0xec231u})
                need(source_weapon_animation_query_symbol_tu15(a,caller,activity.original)==activity.original);
            need(source_weapon_animation_query_symbol_tu15(b,0xec230u,activity.original)==activity.original);
            need(source_weapon_animation_query_symbol_tu15(a,0xec230u,activity.mcc)==activity.mcc);
            if(source_weapon_animation_query_symbol_tu15(a,0xec230u,activity.original)!=activity.mcc)
                throw std::runtime_error(std::string("lunge impact timing EC230 did not select the authenticated MCC name: ")+activity.name);
        }
        for(const auto index:{4u,6u})for(const auto caller:{0xebf41u,0xec154u,0xec230u})
            need(source_weapon_animation_query_symbol_tu15(a,caller,shifted_fp_activities[index].original)==
                shifted_fp_activities[index].original);
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
    for(const auto [pc,original,mcc]:body_sites)
        need(source_weapon_animation_body_query_symbol_tu15(a,pc,original)==original);
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
        need(source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu)==0x04000557u);
        lease.reset();
        need(source_weapon_animation_scoped_query_symbol_tu15(a,0x0e000607u)==0x0e000607u);
        need(source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu)==0x0400054bu);
        SourceWeaponAnimationQuerySymbolsTu15 replacement(a,true,names);
        need(source_weapon_animation_scoped_query_symbol_tu15(a,0x0e000607u)==0x0e000607u);
        need(source_weapon_animation_scoped_class_symbol_tu15(a,0x0400054bu)==0x0400054bu);
        {SourceWeaponAnimationQueryScopeTu15 current(a,0x169a56u,0x0e000607u);
            need(source_weapon_animation_scoped_query_symbol_tu15(a,0x0e000607u)==0x0e000613u);}
        need(source_weapon_animation_scoped_query_symbol_tu15(a,0x0e000607u)==0x0e000607u);
    }
    std::cout<<"PASS native FP/body operand sites, origin-overlay/fallback guards, named admission and bank/thread/lifetime isolation\n";
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
