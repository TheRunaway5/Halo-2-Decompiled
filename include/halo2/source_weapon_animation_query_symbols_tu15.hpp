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
// from EBF3C/EC14F/EC22B, two ground/air lunge keys from EC22B, or four
// staged-reload keys from16B458. These preserve the canonical activities,
// timing selectors and original event/clock algorithms.
// Every other caller,unbound/original bank and authored identifier is unchanged.
std::uint32_t source_weapon_animation_query_symbol_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t caller_return,std::uint32_t original);
//115AF0 copies named grip/arm-offset activities and its selected canonical
// dual class into overlay lookups. Only its ten authenticated operand sites
// enter the Cache namespace; retained class, flags and pose state stay raw.
std::uint32_t source_weapon_animation_body_query_symbol_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t instruction,std::uint32_t original);

// Native FP playback/timing,16AD45's four cached extra-channel lookups and
// exact early body-equip lookups and explicit weapon-body request origins retain
// canonical class/activity state. Only their copied record-search operands
// may enter the qualified Cache namespace. Body overlays and fallback playback
// require the complete116570 ->lookup guard chain, never a bare setter caller.
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
    friend std::uint32_t source_weapon_animation_scoped_class_symbol_tu15(
        const SourcePhysicsContactMemory &,std::uint32_t);
    friend std::uint32_t source_weapon_animation_scoped_overlay_symbol_tu15(
        const SourcePhysicsContactMemory &,std::uint32_t);
    const SourcePhysicsContactMemory *memory_;
    std::uint32_t activity_;
    enum class Kind : std::uint8_t {none,first_person,body_origin,body_overlay,body_fallback,body_playback};
    Kind kind_=Kind::none;
    bool class_query_{};
    std::weak_ptr<const SourceWeaponAnimationQuerySymbolBindingTu15> binding_;
    SourceWeaponAnimationQueryScopeTu15 *previous_;
};
std::uint32_t source_weapon_animation_scoped_query_symbol_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t original);
//2A019A copies a class key into the authored hierarchy's binary search.
// Its qualified native FP or body lookup scope may resolve compiled dual here.
std::uint32_t source_weapon_animation_scoped_class_symbol_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t original);
//116570's qualified weapon request first searches the overlay action bank.
// Only the copied1E3FB3 key may use this namespace.
std::uint32_t source_weapon_animation_scoped_overlay_symbol_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t original);
}
