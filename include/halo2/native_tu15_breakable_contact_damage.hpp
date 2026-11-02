#pragma once
#include "halo2/native_tu15_vector_normalize.hpp"
namespace halo2 {
struct NativeTu15DamageData {std::uint32_t identity{};};
struct NativeTu15CollisionDamageEntry {std::uint32_t identity{};};
struct NativeTu15BreakableContactDamageResult {
    bool built{};
    std::uint32_t eax{},ecx{},edx{};
    std::array<std::uint32_t,4> xmm0{},xmm1{},xmm2{};
    bool normalized{};
};
struct NativeTu15BreakableContactDamageStats {
    std::uint64_t calls{},below_threshold{},structure_keys{},instance_keys{},other_kind_keys{};
    NativeTu15VectorNormalizeStats normalize;
};
// Actual D8BB0 initializes the 0x88-byte damage payload. The enclosing
// contact entry owns twelve additional bytes, which this initializer preserves.
void native_tu15_initialize_damage_data(SourcePhysicsContactMemory&,NativeTu15DamageData,
    std::uint32_t definition,const SourcePhysicsContactRecordObserver& = {});
// Actual 1E2800: build one 0x94-byte breakable-surface damage record from a
// retained component contact. No contact, impulse, allocation or admission is
// manufactured. A rejected impulse leaves the complete output slot untouched.
NativeTu15BreakableContactDamageResult native_tu15_build_breakable_contact_damage(
    SourcePhysicsContactMemory&,std::uint32_t object_datum,std::uint32_t contact_index,
    NativeTu15CollisionDamageEntry,NativeTu15PhysicsFrameFp&,unsigned preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {},NativeTu15BreakableContactDamageStats* = nullptr);
}
