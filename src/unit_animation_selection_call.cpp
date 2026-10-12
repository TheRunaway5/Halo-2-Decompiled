#include "halo2/unit_animation_selection_runtime.hpp"
#include <stdexcept>

namespace halo2 {
std::uint32_t source_call_unit_animation_selection_1_1(
    SourcePhysicsContactMemory &memory, const SourceRuntimeCall &q,
    SourcePhysicsFactoryFloatingPoint fp, const SourceRuntimeOwner &owner,
    const SourcePhysicsContactRecordObserver &observe) {
  const auto require = [](bool b) {
    if (!b)
      throw std::runtime_error(
          "Missing defined original animation selection input");
  };
  switch (q.entry) {
  case 0x110b00:
    require(q.eax && q.arguments.size() == 2);
    break;
  case 0x1cf310:
    require(q.eax && q.esi && q.arguments.empty());
    break;
  case 0x1cf360:
    require(q.eax && q.edx && q.esi && q.arguments.size() == 2);
    break;
  case 0x1d1bd0:
    require(q.eax && q.arguments.size() == 1);
    break;
  case 0x111b60:
    require(q.arguments.size() == 3);
    break;
  case 0xbb240:
    require(q.esi && q.arguments.size() == 1);
    break;
  case 0x114bd0:
    require(q.arguments.size() == 1);
    break;
  case 0x1cec20:
    require(q.ecx && q.edx && q.esi && q.arguments.size() == 1);
    break;
  case 0x1cfa90:
    require(q.esi && q.arguments.empty());
    break;
  case 0x1cefe0:
    require(q.eax && q.arguments.size() == 4);
    break;
  case 0x1c9de0:
    require(q.eax && q.arguments.empty());
    break;
  case 0x1cfde0:
    require(q.eax && q.ecx && q.edx && q.arguments.size() == 6);
    break;
  case 0x1d0790:
    require(q.ebx && q.arguments.size() == 4);
    break;
  case 0x1ceda0:
    require(q.esi && q.edi && q.arguments.size() == 1);
    break;
  case 0x1d1b70:
    require(q.ecx && q.edx && q.edi && q.arguments.size() == 3);
    break;
  default:
    throw std::runtime_error(
        "Unsupported original animation selection call ABI");
  }
  SourceUnitAnimationRuntimeAbi abi{};
  // All undefined incoming volatile/preserved register values are incidental
  // bookkeeping in these complete bodies. Their outputs are not exposed.
  const std::array regs{q.eax,
                        q.ecx,
                        q.edx,
                        q.ebx,
                        std::optional(q.entry_esp),
                        std::optional<std::uint32_t>{},
                        q.esi,
                        q.edi};
  for (unsigned i = 0; i < regs.size(); ++i)
    if (regs[i])
      abi.gp[i] = *regs[i];
  abi.eflags = 2;
  for (unsigned i = 0; i < q.arguments.size(); ++i) {
    std::array<std::byte, 4> bytes;
    for (unsigned j = 0; j < 4; ++j)
      bytes[j] = std::byte(q.arguments[i] >> (8 * j));
    if (!q.argument_masks.empty()) {
      const auto mask = q.argument_masks.at(i);
      std::array<std::byte, 4> before;
      memory.read(q.entry_esp + 4 + 4 * i, before);
      for (unsigned j = 0; j < 4; ++j)
        bytes[j] = (bytes[j] & std::byte(mask >> (8 * j))) |
                   (before[j] & ~std::byte(mask >> (8 * j)));
    }
    memory.write(q.entry_esp + 4 + 4 * i, bytes);
  }
  SourceUnitAnimationRuntimeServices services;
  services.required_owner = [&](auto &m, const auto &call, auto &state,
                                const auto &observer) {
    if (!owner)
      throw std::runtime_error(
          "Required original animation cache/clock owner unavailable");
    if (call.target != 0x125120 && call.target != 0x337a40)
      throw std::runtime_error(
          "Unknown original animation selection dependency");
    // Both actual lower ABIs have no stack/register inputs. The scheduler
    // retains its global banks and the clock returns its real DWORD in EAX.
    state.gp[0] =
        owner(m, {.entry = call.target, .entry_esp = state.gp[4]}, observer);
    state.gp[4] += 4;
  };
  source_run_unit_animation_selection_1_1(memory, q.entry, abi, fp, services,
                                          observe);
  if (abi.gp[4] != q.entry_esp + 4 + 4 * q.arguments.size())
    throw std::runtime_error(
        "Original animation selection stack ownership mismatch");
  return abi.gp[0];
}
std::uint32_t source_call_unit_animation_pose_1_1(
    SourcePhysicsContactMemory &memory, const SourceRuntimeCall &q,
    SourcePhysicsFactoryFloatingPoint fp,
    const SourceUnitAnimationRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
  bool valid = false;
  switch (q.entry) {
  case 0xc04f0:
    valid = q.arguments.size() == 3;
    break;
  case 0xe8050:
    valid = q.eax && q.esi && q.arguments.size() == 2;
    break;
  case 0xe63c0:
    valid = q.edx && q.arguments.size() == 3;
    break;
  case 0xe1800:
    valid = q.arguments.size() == 2;
    break;
  case 0xe4f40:
  case 0xdf7c0:
    valid = q.eax && q.arguments.empty();
    break;
  case 0x142e80:
  case 0x143f60:
    valid = q.eax && q.ecx && q.edx && q.arguments.empty();
    break;
  case 0x1d1290:
    valid = q.eax && q.esi && q.arguments.size() == 1;
    break;
  case 0xddbc0:
    valid = q.arguments.size() == 2;
    break;
  case 0x10d260:
    valid = q.ebx && q.arguments.size() == 1;
    break;
  case 0x1d7310:
    valid = q.arguments.size() == 3;
    break;
  case 0xbea50:
    valid = q.eax && q.ecx && q.edx && q.arguments.size() == 3;
    break;
  case 0xbe860:
    valid = q.arguments.size() == 6;
    break;
  case 0x10d1e0:
    valid = q.ebx && q.arguments.size() == 3;
    break;
  case 0x1c9e30:
    valid = q.ecx && q.arguments.empty();
    break;
  case 0x1ca950:
    valid = q.ebx && q.arguments.size() == 4;
    break;
  case 0x1d0f80:
    valid = q.esi && q.arguments.size() == 7;
    break;
  case 0x280030:
    valid = q.eax && q.arguments.size() == 8;
    break;
  case 0x1d1a00:
    valid = q.ebx && q.arguments.empty();
    break;
  default:
    throw std::runtime_error("Unsupported original animation pose call ABI");
  }
  if (!valid)
    throw std::runtime_error("Missing defined original animation pose input");
  if (!q.argument_masks.empty())
    throw std::runtime_error("Partial original pose ABI argument unsupported");
  SourceUnitAnimationRuntimeAbi abi{};
  const std::array regs{q.eax,
                        q.ecx,
                        q.edx,
                        q.ebx,
                        std::optional(q.entry_esp),
                        std::optional<std::uint32_t>{},
                        q.esi,
                        q.edi};
  for (unsigned i = 0; i < regs.size(); ++i)
    if (regs[i])
      abi.gp[i] = *regs[i];
  abi.eflags = 2;
  for (unsigned i = 0; i < q.arguments.size(); ++i) {
    std::array<std::byte, 4> bytes;
    for (unsigned j = 0; j < 4; ++j)
      bytes[j] = std::byte(q.arguments[i] >> (8 * j));
    memory.write(q.entry_esp + 4 + 4 * i, bytes);
  }
  source_run_unit_animation_selection_1_1(memory, q.entry, abi, fp, services,
                                          observe);
  if (abi.gp[4] != q.entry_esp + 4 + 4 * q.arguments.size())
    throw std::runtime_error(
        "Original animation pose stack ownership mismatch");
  return abi.gp[0];
}
} // namespace halo2
