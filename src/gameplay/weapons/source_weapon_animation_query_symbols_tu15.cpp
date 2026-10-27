#include "halo2/source_weapon_animation_query_symbols_tu15.hpp"
#include <array>
#include <algorithm>
#include <map>
#include <mutex>
#include <exception>
#include <stdexcept>
#include <string_view>
#include <span>

namespace halo2 {
struct SourceWeaponAnimationQuerySymbolBindingTu15 {const SourcePhysicsContactMemory *memory;};
namespace {
struct Symbol {std::uint32_t original,mcc;std::string_view name;};
// Changed first-person activity keys from the authenticated full-name join.
// The first four belong to EBF20's availability queries. The final four also
// qualify16B44A's staged-magazine timing lookup; all twelve qualify inside
// the original first-person setter's lookup scope. EC230 additionally queries
// the ground/air lunge's authored impact events after the dash releases.
// Source action activities and EBF20's other fields remain canonical.
constexpr std::array symbols{
    Symbol{0x0e000607u,0x0e000613u,"melee_strike_1"},
    Symbol{0x0e000608u,0x0e000614u,"melee_strike_2"},
    Symbol{0x0e000609u,0x0e000615u,"melee_strike_3"},
    Symbol{0x0e00060au,0x0e000616u,"melee_strike_4"},
    Symbol{0x0a0005bbu,0x0a0005c7u,"melee_dash"},
    Symbol{0x0b0005b2u,0x0b0005beu,"melee_lunge"},
    Symbol{0x130005bcu,0x130005c8u,"melee_dash_airborne"},
    Symbol{0x140005b3u,0x140005bfu,"melee_lunge_airborne"},
    Symbol{0x0c0005aeu,0x0c0005bau,"reload_enter"},
    Symbol{0x150005afu,0x150005bbu,"reload_continue_empty"},
    Symbol{0x140005b0u,0x140005bcu,"reload_continue_full"},
    Symbol{0x0b0005b1u,0x0b0005bdu,"reload_exit"}
};
constexpr std::size_t melee_candidate_count=4;
constexpr std::size_t staged_reload_begin=8;
constexpr Symbol equip_dual_class{0x0400054bu,0x04000557u,"dual"};
//16AD45 caches these channels using169432's canonical dual/any class.
// Qualify the copied class-search operand only inside these exact native calls.
//1D45A0's original dual exception must see its canonical class before any
// hierarchy lookup. These exact availability/setter calls alone qualify the
// copied2A019A class-search operand; requests and manager state remain raw.
constexpr std::array body_equip_lookup_origins{
    std::pair{0x112406u,0x0400054bu}, //1123B0 availability: requested class
    std::pair{0x1124e8u,0x07000101u}, //112420 setter: retained activity sentinel
    std::pair{0x1125e0u,0x0400054bu}  //112420 overlay availability: requested class
};
constexpr std::array first_person_extra_origins{
    std::pair{0x16b0d6u,0x12000068u}, // overcharged_jitter
    std::pair{0x16b0eau,0x08000061u}, // overlays
    std::pair{0x16b10du,0x0a000069u}, // ammunition
    std::pair{0x16b12du,0x0e0000deu}  // pitch_and_turn
};
constexpr std::array body_overlay_symbols{
    Symbol{0x0b00054au,0x0b000556u,"arm_offsets"},
    Symbol{0x0400060bu,0x04000617u,"grip"}
};
constexpr std::array body_query_operands{
    std::pair{0x115ccfu,body_overlay_symbols[0]},std::pair{0x115ce8u,body_overlay_symbols[0]},
    std::pair{0x115d0cu,body_overlay_symbols[1]},std::pair{0x115d2au,body_overlay_symbols[1]},
    std::pair{0x115d68u,body_overlay_symbols[1]},std::pair{0x115d83u,body_overlay_symbols[1]},
    std::pair{0x115cd4u,equip_dual_class},std::pair{0x115cedu,equip_dual_class},
    std::pair{0x115d11u,equip_dual_class},std::pair{0x115d2fu,equip_dual_class}
};
constexpr std::array body_action_symbols{
    Symbol{0x0b00053e,0x0b00054a,"fire_1_left"},Symbol{0x0b00053f,0x0b00054b,"fire_2_left"},
    Symbol{0x0e00053c,0x0e000548,"chamber_1_left"},Symbol{0x0e00053d,0x0e000549,"chamber_2_left"},
    Symbol{0x0e000540,0x0e00054c,"charged_1_left"},Symbol{0x0e000541,0x0e00054d,"charged_2_left"},
    Symbol{0x0d00053a,0x0d000546,"reload_1_left"},Symbol{0x0d00053b,0x0d000547,"reload_2_left"},
    Symbol{0x1000067e,0x1000068a,"overheating_left"},Symbol{0x0f00067f,0x0f00068b,"overheated_left"},
    Symbol{0x0d000680,0x0d00068c,"o_h_exit_left"},Symbol{0x0a000537,0x0a000543,"ready_left"},
    Symbol{0x15000538,0x15000544,"ready_left_overheated"},Symbol{0x0d000539,0x0d000545,"put_away_left"},
    Symbol{0x0a00022d,0x0a00023b,"smash_left"},Symbol{0x0b00022e,0x0b00023c,"smash_right"}
};
// Exact native return/activity pairs. E9660 and the three put-away owners
// synthesize their canonical keys arithmetically; those calculations stay raw.
constexpr std::array body_request_origins{
    std::pair{0xe99e7u,0x06000006u},std::pair{0xe9a01u,0x06000007u},
    std::pair{0xe9a1bu,0x09000004u},std::pair{0xe9a35u,0x09000005u},
    std::pair{0xe9a4fu,0x09000008u},std::pair{0xe9a69u,0x09000009u},
    std::pair{0xe9a83u,0x0b00053eu},std::pair{0xe9a9du,0x0b00053fu},
    std::pair{0xe9ab7u,0x0e00053cu},std::pair{0xe9ad1u,0x0e00053du},
    std::pair{0xe9aebu,0x0e000540u},std::pair{0xe9b05u,0x0e000541u},
    std::pair{0xe9b1fu,0x0b00006du},std::pair{0xe9b39u,0x0a000066u},
    std::pair{0xe9b53u,0x08000071u},std::pair{0xe9b6du,0x1000067eu},
    std::pair{0xe9b87u,0x0f00067fu},std::pair{0xe9ba1u,0x0d000680u},
    std::pair{0xe97a9u,0x08000002u},std::pair{0xe97a9u,0x08000003u},
    std::pair{0xe97a9u,0x0d00053au},std::pair{0xe97a9u,0x0d00053bu},
    std::pair{0xeab2eu,0x08000025u},std::pair{0xeab2eu,0x0d000539u},
    std::pair{0xeb269u,0x08000025u},std::pair{0xeb269u,0x0d000539u},
    std::pair{0xeb337u,0x08000025u},std::pair{0xeb337u,0x0d000539u},
    std::pair{0xeaec2u,0x1000006eu},std::pair{0xeaec2u,0x15000538u},
    std::pair{0xeaef3u,0x05000024u},std::pair{0xeaef3u,0x0a000537u},
    std::pair{0xea78fu,0x1000006cu},std::pair{0xea7aau,0x0d000021u},
    std::pair{0xee9aeu,0x0a00022du},std::pair{0xee9aeu,0x0b00022eu}
};
std::mutex registry_mutex;
std::map<const SourcePhysicsContactMemory *,std::weak_ptr<const SourceWeaponAnimationQuerySymbolBindingTu15>> registry;
thread_local SourceWeaponAnimationQueryScopeTu15 *query_scope;
}
SourceWeaponAnimationQuerySymbolsTu15::SourceWeaponAnimationQuerySymbolsTu15(
    const SourcePhysicsContactMemory &memory,bool mcc,const Name &name) {
    if(!mcc)return;
    if(!name)throw std::invalid_argument("MCC weapon timing symbols require their Cache name resolver");
    for(const auto &symbol:symbols)
        if(name(symbol.mcc)!=symbol.name)
            throw std::invalid_argument("MCC weapon timing symbol differs from its authenticated name join");
    if(name(equip_dual_class.mcc)!=equip_dual_class.name)
        throw std::invalid_argument("MCC dual equip class differs from its authenticated name join");
    for(const auto &symbol:body_overlay_symbols)
        if(name(symbol.mcc)!=symbol.name)
            throw std::invalid_argument("MCC body pose symbol differs from its authenticated name join");
    for(const auto &symbol:body_action_symbols)
        if(name(symbol.mcc)!=symbol.name)
            throw std::invalid_argument("MCC weapon-body action differs from its authenticated name join");
    auto binding=std::make_shared<const SourceWeaponAnimationQuerySymbolBindingTu15>(
        SourceWeaponAnimationQuerySymbolBindingTu15{&memory});
    std::lock_guard lock(registry_mutex);const auto previous=registry.find(&memory);
    if(previous!=registry.end()&&!previous->second.expired())
        throw std::invalid_argument("Source bank already owns its weapon timing namespace");
    registry[&memory]=binding;binding_=std::move(binding);
}
SourceWeaponAnimationQuerySymbolsTu15::~SourceWeaponAnimationQuerySymbolsTu15() {
    if(!binding_)return;
    std::lock_guard lock(registry_mutex);const auto at=registry.find(binding_->memory);
    if(at!=registry.end()&&at->second.lock()==binding_)registry.erase(at);
}
std::uint32_t source_weapon_animation_query_symbol_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t caller,std::uint32_t original) {
    const bool staged_timing=caller==0x16b45du;
    if(!staged_timing && caller!=0xebf41u && caller!=0xec154u && caller!=0xec230u)return original;
    // EBFA0's EC22B ->16B31B lookup owns the strike impact/recovery clock.
    // MCC relocated these names independently of the playback lookup. Resolve
    // the named timing record without changing the retained source activity.
    const bool lunge_timing=caller==0xec230u &&
        (original==0x0b0005b2u || original==0x140005b3u);
    const auto begin=symbols.begin()+(staged_timing?staged_reload_begin:
        lunge_timing?melee_candidate_count:0u);
    const auto end=staged_timing?symbols.end():symbols.begin()+
        (lunge_timing?staged_reload_begin:melee_candidate_count);
    const auto symbol=std::find_if(begin,end,
        [&](const auto &candidate){return candidate.original==original;});
    if(symbol==end)return original;
    std::lock_guard lock(registry_mutex);const auto at=registry.find(&memory);
    if(at==registry.end()||at->second.expired())return original;
    return symbol->mcc;
}
std::uint32_t source_weapon_animation_body_query_symbol_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t instruction,std::uint32_t original) {
    const auto operand=std::find_if(body_query_operands.begin(),body_query_operands.end(),
        [&](const auto &candidate){return candidate.first==instruction && candidate.second.original==original;});
    if(operand==body_query_operands.end())return original;
    std::lock_guard lock(registry_mutex);const auto at=registry.find(&memory);
    if(at==registry.end() || at->second.expired())return original;
    return operand->second.mcc;
}
SourceWeaponAnimationQueryScopeTu15::SourceWeaponAnimationQueryScopeTu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t caller,std::uint32_t activity)
    :memory_(&memory),activity_(activity),previous_(query_scope) {
    bool bind{};
    if(caller==0x169a56u) {kind_=Kind::first_person;class_query_=true;bind=true;}
    else if(caller==0x16b402u) {class_query_=true;bind=true;}
    else if(std::find(first_person_extra_origins.begin(),first_person_extra_origins.end(),
        std::pair{caller,activity})!=first_person_extra_origins.end()) {class_query_=true;bind=true;}
    else if(std::find(body_equip_lookup_origins.begin(),body_equip_lookup_origins.end(),
        std::pair{caller,activity})!=body_equip_lookup_origins.end()) {class_query_=true;bind=true;}
    else if(std::find(body_request_origins.begin(),body_request_origins.end(),std::pair{caller,activity})!=body_request_origins.end()) {
        kind_=Kind::body_origin;bind=true;
    }else if(previous_ && previous_->memory_==&memory && previous_->activity_==activity) {
        if(previous_->kind_==Kind::body_origin && caller==0x116654u) {
            kind_=Kind::body_overlay;class_query_=true;binding_=previous_->binding_;
        }else if(previous_->kind_==Kind::body_origin && caller==0x1166e4u) {
            kind_=Kind::body_fallback;binding_=previous_->binding_;
        }else if(previous_->kind_==Kind::body_fallback && caller==0x111bceu) {
            kind_=Kind::body_playback;class_query_=true;binding_=previous_->binding_;
        }
    }
    if(bind) {
        std::lock_guard lock(registry_mutex);const auto at=registry.find(&memory);
        if(at!=registry.end())binding_=at->second;
    }
    // Even an unqualified nested request masks its parent's namespace.
    query_scope=this;
}
SourceWeaponAnimationQueryScopeTu15::~SourceWeaponAnimationQueryScopeTu15() {
    if(query_scope!=this)std::terminate();
    query_scope=previous_;
}
std::uint32_t source_weapon_animation_scoped_query_symbol_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t original) {
    auto *scope=query_scope;
    while(scope && scope->memory_!=&memory)scope=scope->previous_;
    if(!scope || scope->activity_!=original)return original;
    std::span<const Symbol> keys;
    if(scope->kind_==SourceWeaponAnimationQueryScopeTu15::Kind::first_person)keys=symbols;
    else if(scope->kind_==SourceWeaponAnimationQueryScopeTu15::Kind::body_overlay ||
        scope->kind_==SourceWeaponAnimationQueryScopeTu15::Kind::body_playback)keys=body_action_symbols;
    else return original;
    const auto symbol=std::find_if(keys.begin(),keys.end(),
        [&](const auto &candidate){return candidate.original==original;});
    if(symbol==keys.end())return original;
    std::lock_guard lock(registry_mutex);const auto at=registry.find(&memory);
    const auto binding=scope->binding_.lock();
    if(!binding || at==registry.end() || at->second.lock()!=binding)return original;
    return symbol->mcc;
}
std::uint32_t source_weapon_animation_scoped_class_symbol_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t original) {
    if(original!=equip_dual_class.original)return original;
    auto *scope=query_scope;
    while(scope && scope->memory_!=&memory)scope=scope->previous_;
    if(!scope || !scope->class_query_)return original;
    std::lock_guard lock(registry_mutex);const auto at=registry.find(&memory);
    const auto binding=scope->binding_.lock();
    if(!binding || at==registry.end() || at->second.lock()!=binding)return original;
    return equip_dual_class.mcc;
}
std::uint32_t source_weapon_animation_scoped_overlay_symbol_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t original) {
    auto *scope=query_scope;
    while(scope && scope->memory_!=&memory)scope=scope->previous_;
    if(!scope || scope->kind_!=SourceWeaponAnimationQueryScopeTu15::Kind::body_overlay)return original;
    return source_weapon_animation_scoped_query_symbol_tu15(memory,original);
}
}
