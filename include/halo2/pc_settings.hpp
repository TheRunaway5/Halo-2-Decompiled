#pragma once
#include <array>
#include <cstdint>
#include <string>
namespace halo2 {
// Host bindings retain the previously requested native defaults. Their action
// labels are source PC string IDs; these are not Vista engine action ordinals.
enum class PcAction : unsigned {
  Left,
  Right,
  Forward,
  Back,
  Jump,
  Crouch,
  Fire,
  Grenade,
  SwitchWeapon,
  Melee,
  Reload,
  Zoom,
  Scoreboard,
  Pickup,
  DualPickup,
  Flashlight,
  SwitchGrenade,
  DualSecondary,
  DualPrimary,
  Count
};
constexpr unsigned pc_action_count = unsigned(PcAction::Count);
inline constexpr std::array<std::uint32_t, pc_action_count> pc_action_labels{
    0x1d000830, 0x1e000831, 0x1e000832, 0x1b000833, 0x16000834,
    0x18000835, 0x16000836, 0x1f000837, 0x1f00083c, 0x1e000838,
    0x18000956, 0x1600086d, 0x28000978,
    0x1f00083d,0x1d00085e,0x1c00086c,0x2000083b,0x2200085c,0x2300085d};
constexpr int pc_mouse_button(unsigned button) { return 512 + int(button); }
constexpr int pc_wheel_up =
    600; // Existing host adapter treats either direction as one pulse.
struct PcKeyboardSettings {
  PcKeyboardSettings();
  std::array<int, pc_action_count> bindings{};
  std::array<int, pc_action_count> secondary{};
  int sensitivity = 3;
  bool invert{},auto_center{},dual_wield_invert{};
  bool operator==(const PcKeyboardSettings &) const = default;
};
bool valid_pc_binding(int);
std::string pc_binding_name(int);
} // namespace halo2
