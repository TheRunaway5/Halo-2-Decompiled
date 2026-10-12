#include "halo2/training_controls.hpp"
#include "halo2/cache.hpp"
namespace halo2 {
namespace {
std::string utf8(unsigned c) {
  if (c < 128)
    return std::string(1, char(c));
  if (c < 2048)
    return {char(0xc0 | (c >> 6)), char(0x80 | (c & 63))};
  return {char(0xe0 | (c >> 12)), char(0x80 | ((c >> 6) & 63)),
          char(0x80 | (c & 63))};
}
} // namespace
std::string training_control_text(const std::string &text,
                                  const ControllerSettings &pad,
                                  const PcKeyboardSettings &keys,
                                  bool gamepad) {
  const auto key = [&](PcAction action) {
    const auto i = unsigned(action);
    return pc_binding_name(keys.bindings[i] ? keys.bindings[i]
                                            : keys.secondary[i]);
  };
  const auto glyph = [&](unsigned token) -> std::string {
    if (token == 0xe456 || token == 0xe457) {
      if (!gamepad)
        return token == 0xe457
                   ? "Mouse"
                   : key(PcAction::Forward) + " / " + key(PcAction::Left) +
                         " / " + key(PcAction::Back) + " / " +
                         key(PcAction::Right);
      const bool left =
          token == 0xe456 ? !(pad.sticks & 1) : bool(pad.sticks & 1);
      return utf8(left ? 0xe110 : 0xe111);
    }
    if(token==0xe45e) {
      if(!gamepad)return key(keys.dual_wield_invert?PcAction::DualPrimary:PcAction::DualSecondary);
      for(unsigned hardware=0;hardware<16;++hardware) {
        ControllerSample sample;sample.hardware[hardware]=1;
        if(controller_input(sample,pad).secondary) {
          constexpr std::array<unsigned,16> icons{0xe106,0xe107,0xe108,0xe109,
              0xe10a,0xe10b,0xe10c,0xe10d,0xe10e,0xe10f,0xe104,0xe105,0xe100,0xe101,0xe102,0xe103};
          return utf8(icons[hardware]);
        }
      }
      return "?";
    }
    if (token < 0xe446 || token > 0xe455)
      return utf8(token);
    const unsigned action = token - 0xe446;
    if (!gamepad) {
      constexpr std::array actions{
          PcAction::Jump,         PcAction::SwitchGrenade, PcAction::Pickup,
          PcAction::SwitchWeapon, PcAction::Melee,         PcAction::Flashlight,
          PcAction::Grenade,      PcAction::Fire,          PcAction::Count,
          PcAction::Scoreboard,   PcAction::Crouch,        PcAction::Zoom,
          PcAction::Count,        PcAction::Count,         PcAction::Count,
          PcAction::Count};
      if (action == 8)
        return "Escape";
      if (action == 14)
        return "Enter";
      if (action == 15)
        return "Escape";
      return actions[action] == PcAction::Count ? "?" : key(actions[action]);
    }
    constexpr std::array<unsigned, 16> icons{
        0xe106, 0xe107, 0xe108, 0xe109, 0xe10a, 0xe10b, 0xe10c, 0xe10d,
        0xe10e, 0xe10f, 0xe104, 0xe105, 0xe100, 0xe101, 0xe102, 0xe103};
    for (unsigned hardware = 0; hardware < 16; ++hardware) {
      ControllerSample sample;
      sample.hardware[hardware] = 1;
      const auto v = controller_input(sample, pad);
      const std::array<bool, 16> active{v.jump,        v.switch_grenade,
                                        v.pickup,      v.switch_weapon,
                                        v.melee,       v.flashlight,
                                        v.grenade,     v.fire,
                                        hardware == 6, hardware == 7,
                                        v.crouch,      v.zoom,
                                        false,         false,
                                        v.accept,      v.cancel};
      if (active[action])
        return utf8(icons[hardware]);
    }
    return "?";
  };
  std::string result;
  for (std::size_t i = 0; i < text.size();) {
    const auto begin = i;
    unsigned c = std::uint8_t(text[i++]), n = 0;
    if ((c & 0xe0) == 0xc0) {
      c &= 31;
      n = 1;
    } else if ((c & 0xf0) == 0xe0) {
      c &= 15;
      n = 2;
    } else if ((c & 0xf8) == 0xf0) {
      c &= 7;
      n = 3;
    }
    if (i + n > text.size())
      throw FormatError("Truncated training UTF8 token");
    while (n--)
      c = (c << 6) | (std::uint8_t(text[i++]) & 63);
    if ((c >= 0xe446 && c <= 0xe457) || c==0xe45e)
      result += glyph(c);
    else
      result.append(text, begin, i - begin);
  }
  return result;
}
} // namespace halo2
