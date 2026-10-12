#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace halo2 {
// An actual object-header/body/definition view, resolved by full datum.
// These are source fields, not inferred presentation or grounded state.
struct WeaponEffectObject {
  std::uint8_t header_flags{}; // object header +02.
  std::span<const std::byte> body;
  std::uint32_t definition_model{}; // object definition +38.
};
struct WeaponEffectInput {
  std::uint32_t effect{}, weapon{}, previous_handle{};
  std::uint32_t a_bits{}, b_bits{}; // unchanged source scalar words.
};
struct AttachedEffectRequest {
  std::array<std::byte, 0x60> packet{};
  // Two original 1C-byte direction/location entries referenced by packet+1C.
  std::array<std::byte, 0x38> directions{};
};
struct WeaponEffectServices {
  std::function<WeaponEffectObject(std::uint32_t)> object;
  std::function<std::uint32_t(std::uint32_t)> tag_group;
  // Original17CBD0, including retained-pool salt/type validation, flag clear
  // and178D80 reactivation. It is needed only for a non-NONE prior handle.
  std::function<bool(std::uint32_t handle, std::uint32_t effect)> refresh;
  // Original BACC0 on the selected source object, including parent/current
  // matrices. No guessed muzzle point or fallback transform is accepted.
  std::function<std::array<std::byte, 12>(std::uint32_t)> center;
  std::array<std::byte, 12> gravity{}, up{}; // original46FC5C/46FC50 vectors.
  // Literal owning address of the caller's direction entries, if packet bytes
  // are used in the source address space. The creator receives both buffers.
  std::uint32_t directions_address{};
  // Original177A10 and its allocation, attachment, visibility, event/particle
  // owners. Absence throws; this function never supplies creation success.
  std::function<std::uint32_t(const AttachedEffectRequest &)> create;
  // Original18AA90: EBX tag, owner and caller scale. Audio is separate.
  std::function<void(std::uint32_t tag, std::uint32_t owner,
                     std::uint32_t scale_bits)> sound;
};

// Original106070: select the actual model object or its retained parent.
std::uint32_t source_weapon_effect_object(std::uint32_t weapon,
                                          const WeaponEffectServices &);
// Original1781F0/177DA0 construction used by the stock-SMG normal effect path.
AttachedEffectRequest source_attached_effect_request(std::uint32_t effect,
    std::uint32_t object, std::uint32_t a_bits, std::uint32_t b_bits,
    const WeaponEffectServices &);
// Original107B50. Call at the prepared-shot phase, before projectile postpass.
// Its return is the actual generic effect owner's result, not flash admission.
std::uint32_t dispatch_source_weapon_effect(const WeaponEffectInput &,
                                            const WeaponEffectServices &);

struct EffectVisibilityServices {
  // Original3EB10 and actual518440+5. The caller owns those states.
  bool special_view{}, camera_override{};
  // Original11D620/11D650: selected cluster bit from actual game+1178/+1138.
  std::span<const std::byte> local_pvs, all_player_pvs;
  // Original179080 may remove the actual effect and locations. Its result is
  // mandatory when called; removal or survival is never assumed.
  std::function<bool()> retire;
};
// Whole original17B1A0 visibility consumer on literal400-byte effect storage
// and the original effect definition flags. Returns the original AL meaning
// that the record has been removed; a false result can still be suppressed.
bool update_source_effect_visibility(std::span<std::byte> effect,
    std::uint32_t definition_flags, const EffectVisibilityServices &);
} // namespace halo2
