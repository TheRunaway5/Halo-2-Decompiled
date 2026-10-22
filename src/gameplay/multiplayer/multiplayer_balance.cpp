#include "halo2/multiplayer_balance.hpp"
#include "halo2/cache.hpp"
#include <bit>

namespace halo2 {
namespace {
class Postprocess {
public:
  Postprocess(MultiplayerBalanceMemory memory,
              std::span<const std::uint32_t, 19> values,
              std::uint32_t delta, std::vector<MultiplayerBalanceStore>* trace)
      : memory_(memory), data_(memory.bytes), values_(values), delta_(delta), trace_(trace) {}

  MultiplayerBalanceResult run() {
    if (data_.u16(offset(memory_.scenario_address + 0x10, 2)) != 1) return {};
    if (memory_.tag_count > 65536)
      throw FormatError("multiplayer balance tag table exceeds datum index capacity");
    data_.require(memory_.tag_table_offset, memory_.tag_count * 16);
    result_.admitted = true;
    const auto runtime = read(resolve(read(memory_.globals_address + 0x16c)) + 0xc);

    if (const auto id = weapon(runtime, 3); id != none) {
      const auto tag = resolve(id);
      // 16479B reads a DWORD; TEST/JLE tests its signed 32-bit value.
      if (std::bit_cast<std::int32_t>(read(tag + 0x2d0)) > 0) {
        const auto barrel = read(tag + 0x2d4);
        store(barrel + 0x74, 0, 0x1647b4);
        store(barrel + 0x78, 0, 0x1647b9);
      }
    }
    melee(runtime, 12, {1, 2, 2}, {0x1647fe, 0x164803, 0x164808});
    melee(runtime, 5, {3, 1, 1}, {0x164845, 0x16484a, 0x16484f});
    melee(runtime, 1, {3, 4, 4}, {0x16488c, 0x164891, 0x164896});

    grenade_damage(0, 10, {0x16490c, 0x164910, 0x164915, 0x16491a,
                          0x16491f, 0x164924, 0x164929, 0x16492e});
    if (const auto id = grenade(0); id != none)
      store(resolve(id) + 0xcc, 11, 0x164964);
    grenade_damage(1, 12, {0x1649b8, 0x1649bc, 0x1649c1, 0x1649c6,
                          0x1649cb, 0x1649d0, 0x1649d5, 0x1649da});
    if (const auto id = grenade(1); id != none) {
      const auto projectile = resolve(id);
      store(projectile + 0xcc, 11, 0x164a13);
      store(projectile + 0xd4, 13, 0x164a1b);
      store(projectile + 0xd8, 13, 0x164a23);
    }

    if (const auto damage = projectile_damage(runtime, 0, 0x14c); damage != none)
      damage_bounds(damage, {14, 14, 14}, {0x164a81, 0x164a86, 0x164a8b});
    if (const auto id = weapon(runtime, 7); id != none)
      store(read(resolve(id) + 0x2d4) + 0x64, 15, 0x164ab6);
    if (const auto damage = projectile_damage(runtime, 1, 0x14c); damage != none) {
      const auto tag = resolve(damage);
      store(tag + 0x20, 16, 0x164b11);
      store(tag + 0x24, 16, 0x164b16);
    }
    if (const auto damage = projectile_damage(runtime, 12, 0x104); damage != none)
      damage_bounds(damage, {17, 18, 18}, {0x164b71, 0x164b7e, 0x164b83});
    const auto vehicle = read(read(runtime + 0x74) + 7 * 8 + 4);
    if (vehicle != none) store(resolve(vehicle) + 4, 5, 0x164ba1);
    return result_;
  }

private:
  static constexpr std::uint32_t none = 0xffffffff;
  MultiplayerBalanceMemory memory_;
  Bytes data_;
  std::span<const std::uint32_t, 19> values_;
  std::uint32_t delta_;
  std::vector<MultiplayerBalanceStore>* trace_;
  MultiplayerBalanceResult result_;

  std::size_t offset(std::uint32_t address, std::size_t size) const {
    return TagAddressSpace(data_, memory_.metadata_magic).offset(address, size);
  }
  std::uint32_t read(std::uint32_t address) const { return data_.u32(offset(address, 4)); }
  std::uint32_t resolve(std::uint32_t datum) const {
    const auto index = datum & 0xffff;
    if (index >= memory_.tag_count)
      throw FormatError("multiplayer balance tag index is outside imported table");
    return data_.u32(memory_.tag_table_offset + std::size_t(index) * 16 + 8);
  }
  void store(std::uint32_t address, unsigned value, std::uint32_t instruction) {
    const auto at = offset(address, 4);
    const auto bits = values_[value];
    if (trace_) trace_->push_back({instruction + delta_, at, data_.u32(at), bits});
    for (unsigned i = 0; i < 4; ++i) memory_.bytes[at + i] = std::byte((bits >> (i * 8)) & 255);
    ++result_.stores;
  }
  std::uint32_t weapon(std::uint32_t runtime, unsigned slot) const {
    return read(read(runtime + 0x6c) + slot * 8 + 4);
  }
  void damage_bounds(std::uint32_t id, std::array<unsigned, 3> values,
                     std::array<std::uint32_t, 3> instructions) {
    const auto tag = resolve(id);
    for (unsigned i = 0; i < 3; ++i) store(tag + 0x1c + i * 4, values[i], instructions[i]);
  }
  void melee(std::uint32_t runtime, unsigned slot, std::array<unsigned, 3> values,
             std::array<std::uint32_t, 3> instructions) {
    const auto id = weapon(runtime, slot);
    if (id == none) return;
    const auto damage = read(resolve(id) + 0x1c0);
    if (damage != none) damage_bounds(damage, values, instructions);
  }
  std::uint32_t grenade(unsigned slot) const {
    // The source selects zero when count==0, then unconditionally dereferences
    // entry+28. Reject that unmapped input; do not add a clean source early-out.
    const auto entry = read(memory_.globals_address + 0x100)
        ? read(memory_.globals_address + 0x104) + slot * 0x2c : 0;
    return read(entry + 0x28);
  }
  void grenade_damage(unsigned slot, unsigned upper,
                      std::array<std::uint32_t, 8> instructions) {
    const auto projectile = grenade(slot);
    if (projectile == none) return;
    const auto damage = read(resolve(projectile) + 0x104);
    if (damage == none) return;
    const auto tag = resolve(damage);
    constexpr std::array<unsigned, 8> fields{0, 4, 0x1c, 0x20, 0x24, 0x44, 0x48, 0x4c};
    const std::array<unsigned, 8> values{5, 6, 7, upper, upper, 8, 9, 9};
    for (unsigned i = 0; i < 8; ++i) store(tag + fields[i], values[i], instructions[i]);
  }
  std::uint32_t projectile_damage(std::uint32_t runtime, unsigned slot,
                                  unsigned field) const {
    const auto id = weapon(runtime, slot);
    if (id == none) return none;
    const auto projectile = read(read(resolve(id) + 0x2d4) + 0x90);
    return projectile == none ? none : read(resolve(projectile) + field);
  }
};
} // namespace
MultiplayerBalanceResult apply_multiplayer_balance(
    MultiplayerBalanceMemory memory, std::span<const std::uint32_t, 19> constants,
    std::uint32_t instruction_delta, std::vector<MultiplayerBalanceStore>* trace) {
  return Postprocess(memory, constants, instruction_delta, trace).run();
}
} // namespace halo2
