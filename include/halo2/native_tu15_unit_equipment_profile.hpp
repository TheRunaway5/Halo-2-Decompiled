#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <optional>
namespace halo2 {
struct SourceUnitEquipmentWeaponInputTu15 {
    std::uint32_t definition=0xffffffffu;
    std::int16_t rounds_loaded{},rounds_total{}; // TOTAL includes loaded.
    std::optional<std::uint32_t> source_tag_group;
};
struct SourceUnitEquipmentProfileInputTu15 {
    std::array<char,32> name{};
    float health_damage{},shield_damage{};
    std::array<SourceUnitEquipmentWeaponInputTu15,2> weapons;
    std::array<std::uint8_t,2> grenades{};
    // Retained original0x44-row shape; actualCF2E0 consumes two grenade types.
    std::array<std::uint8_t,2> source_tail{};
    bool mark_garbage{};
};
std::array<std::byte,0x44> source_unit_equipment_profile_bytes_tu15(
    const SourceUnitEquipmentProfileInputTu15&);
// Resolved profile INPUT content. Authored-index lookup and declared runtime
// input are different adapters; neither changes an immutable scenario tag.
class NativeTu15UnitEquipmentProfileInput {
public:
    virtual ~NativeTu15UnitEquipmentProfileInput()=default;
    virtual bool selected()const=0;
    virtual std::uint32_t data()const=0;
};
struct NativeTu15UnitEquipmentProfileServices {
    std::function<void(std::uint32_t unit,NativeTu15PhysicsFrameFp&)> clear;
    std::function<std::uint32_t(std::uint32_t unit,std::uint32_t weapon_input,bool mark_garbage,
        NativeTu15PhysicsFrameFp&)> create;
    std::function<bool(std::uint32_t unit,std::uint32_t weapon,NativeTu15PhysicsFrameFp&)> carry;
    std::function<void(std::uint32_t weapon,NativeTu15PhysicsFrameFp&)> retire;
};
struct NativeTu15UnitEquipmentProfileStats {
    std::uint64_t calls{},absent_inputs{},clears{},weapon_requests{},created{},carried{},
        retired{},completed{},unbound_services{},stores{};
};
// CF2E0's complete selected-profile algorithm over ordinary INPUT content.
// Factory/carry/clear/retirement are real typed side-effect owners, never
// supplied successful returns. Missing services refuse at their actual phase.
void native_tu15_apply_unit_equipment_profile(SourcePhysicsContactMemory&,std::uint32_t unit,
    const NativeTu15UnitEquipmentProfileInput&,bool clear_previous,bool mark_garbage,
    NativeTu15PhysicsFrameFp&,const NativeTu15UnitEquipmentProfileServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15UnitEquipmentProfileStats* = nullptr);
}
