#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace halo2 {
class SourcePhysicsContactMemory;
struct SourceWeaponAnimationQuerySymbolBindingTu15;
// Bank-owned namespace of changed compiled weapon activity/class query keys.
// Actual Cache names qualify the lease; graph bytes and source actions stay raw.
class SourceWeaponAnimationQuerySymbolsTu15 {
public:
    using Name=std::function<std::string(std::uint32_t)>;
    SourceWeaponAnimationQuerySymbolsTu15(const SourcePhysicsContactMemory &,bool mcc,const Name &);
    ~SourceWeaponAnimationQuerySymbolsTu15();
    SourceWeaponAnimationQuerySymbolsTu15(const SourceWeaponAnimationQuerySymbolsTu15 &)=delete;
    SourceWeaponAnimationQuerySymbolsTu15 &operator=(const SourceWeaponAnimationQuerySymbolsTu15 &)=delete;
private:
    std::shared_ptr<const SourceWeaponAnimationQuerySymbolBindingTu15> binding_;
};
// Only16B3F1's copied query argument may use this boundary: four strike keys
// from EBF3C/EC14F/EC22B,or four staged-reload keys from16B458. The latter
// preserves16B44A's canonical activity, timing selector and original algorithm.
// Every other caller,unbound/original bank and authored identifier is unchanged.
std::uint32_t source_weapon_animation_query_symbol_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t caller_return,std::uint32_t original);
// Three original equip-owner literals name the dual body class. Qualify only
// those program operands against the actual Cache; authored classes, action
// records, availability results and graph/channel algorithms stay unchanged.
std::uint32_t source_weapon_animation_equip_class_symbol_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t instruction,std::uint32_t original);

//169A51 passes a canonical activity to1D5320. Its channel state retains that
// activity; only the copied1E34A3 record-search key may use the MCC namespace.
// This includes named strike, dash/lunge and staged reload activities.
class SourceWeaponAnimationQueryScopeTu15 {
public:
    SourceWeaponAnimationQueryScopeTu15(const SourcePhysicsContactMemory &,
        std::uint32_t caller_return,std::uint32_t canonical_activity);
    ~SourceWeaponAnimationQueryScopeTu15();
    SourceWeaponAnimationQueryScopeTu15(const SourceWeaponAnimationQueryScopeTu15 &)=delete;
    SourceWeaponAnimationQueryScopeTu15 &operator=(const SourceWeaponAnimationQueryScopeTu15 &)=delete;
private:
    friend std::uint32_t source_weapon_animation_scoped_query_symbol_tu15(
        const SourcePhysicsContactMemory &,std::uint32_t);
    const SourcePhysicsContactMemory *memory_;
    std::uint32_t activity_;
    std::weak_ptr<const SourceWeaponAnimationQuerySymbolBindingTu15> binding_;
    SourceWeaponAnimationQueryScopeTu15 *previous_;
};
std::uint32_t source_weapon_animation_scoped_query_symbol_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t original);
}
