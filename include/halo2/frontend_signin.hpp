#pragma once
#include <array>
#include <cstdint>

namespace halo2 {
struct FrontendSignInSlot {
  bool connected{}, signed_in{}, live{};
  int gamepad_type{}, species{};
};
struct FrontendSignInPresentation {
  std::array<bool, 24> texts{};
  std::array<bool, 8> models{};
  std::array<bool, 8> bitmaps{}; // Pane bitmap indices3 through10.
  std::array<int, 4> controller_frames{9, 9, 9, 9};
  std::array<std::uint32_t, 24> text_strings{}; // Zero retains authored binding.
};
// Original252554 child indices: text widgets begin at2, models/bitmaps at0.
// This policy owns presentation only; joining/signing-in controllers is separate.
constexpr FrontendSignInPresentation frontend_signin_presentation(
    const std::array<FrontendSignInSlot, 4> &slots) {
  FrontendSignInPresentation result;
  for (unsigned i = 0; i < slots.size(); ++i) {
    const auto &slot = slots[i];
    const bool signed_in = slot.connected && slot.signed_in;
    const bool live = signed_in && slot.live;
    result.texts[i*2] = signed_in;
    result.texts[i*2+1] = signed_in;
    result.texts[8+i*3] = signed_in;
    result.texts[9+i*3] = live;
    result.texts[10+i*3] = live;
    result.texts[20+i] = !signed_in;
    if (signed_in)
      result.text_strings[i*2+1] = 0x1300028c;
    else {
      result.text_strings[20+i] = slot.connected ? 0x0f00028a : 0x1100028b;
      result.controller_frames[i] = slot.connected ? (slot.gamepad_type > 1 ? 1 : 2) : 0;
    }
    result.models[i] = signed_in && slot.species == 0;
    result.models[i+4] = signed_in && slot.species == 1;
    result.bitmaps[i] = signed_in;
    result.bitmaps[i+4] = !signed_in;
  }
  return result;
}
} // namespace halo2
