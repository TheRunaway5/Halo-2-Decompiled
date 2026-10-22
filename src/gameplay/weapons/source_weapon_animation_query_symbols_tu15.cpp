#include "halo2/source_weapon_animation_query_symbols_tu15.hpp"
#include <array>
#include <algorithm>
#include <map>
#include <mutex>
#include <exception>
#include <stdexcept>
#include <string_view>

namespace halo2 {
struct SourceWeaponAnimationQuerySymbolBindingTu15 {const SourcePhysicsContactMemory *memory;};
namespace {
struct Symbol {std::uint32_t original,mcc;std::string_view name;};
// Changed first-person activity keys from the authenticated full-name join.
// The first four belong to EBF20's availability queries. The final four also
// qualify16B44A's staged-magazine timing lookup; all twelve qualify inside
// the original first-person setter's lookup scope.
// Source actions, timing and EBF20's other fields are not converted.
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
    const auto begin=symbols.begin()+(staged_timing?staged_reload_begin:0u);
    const auto end=staged_timing?symbols.end():symbols.begin()+melee_candidate_count;
    const auto symbol=std::find_if(begin,end,
        [&](const auto &candidate){return candidate.original==original;});
    if(symbol==end)return original;
    std::lock_guard lock(registry_mutex);const auto at=registry.find(&memory);
    if(at==registry.end()||at->second.expired())return original;
    return symbol->mcc;
}
std::uint32_t source_weapon_animation_equip_class_symbol_tu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t instruction,std::uint32_t original) {
    if(original!=equip_dual_class.original ||
       (instruction!=0xead6au && instruction!=0x1124b7u && instruction!=0x1125d6u))return original;
    std::lock_guard lock(registry_mutex);const auto at=registry.find(&memory);
    if(at==registry.end()||at->second.expired())return original;
    return equip_dual_class.mcc;
}
SourceWeaponAnimationQueryScopeTu15::SourceWeaponAnimationQueryScopeTu15(
    const SourcePhysicsContactMemory &memory,std::uint32_t caller,std::uint32_t activity)
    :memory_(&memory),activity_(activity),previous_(query_scope) {
    if(caller==0x169a56u && std::ranges::any_of(symbols,
        [&](const auto &symbol){return symbol.original==activity;})) {
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
    const auto symbol=std::find_if(symbols.begin(),symbols.end(),
        [&](const auto &candidate){return candidate.original==original;});
    if(symbol==symbols.end())return original;
    std::lock_guard lock(registry_mutex);const auto at=registry.find(&memory);
    const auto binding=scope->binding_.lock();
    if(!binding || at==registry.end() || at->second.lock()!=binding)return original;
    return symbol->mcc;
}
}
