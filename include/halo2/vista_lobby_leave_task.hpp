#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
namespace halo2 {
inline constexpr std::size_t vista_lobby_leave_task_size=0xa70;
inline constexpr std::uint32_t vista_lobby_leave_task_screen=182;
// Literal60C776 descriptor for the actual sole-controller Leave callback.
// This does not allocate/construct a widget or pretend its model is available.
std::array<std::uint32_t,8> vista_lobby_leave_task_descriptor(std::int32_t controller);
// Actual caller-installed63939E/639384/639391/+A64 fields, after a genuine
// native screen182 factory has provided its complete source-sized owner.
void vista_lobby_leave_task_bind(std::span<std::byte>);
struct VistaLobbyLeaveTaskFrameServices {
  std::function<std::uint32_t()> lifecycle_context; //6154D8.
  std::function<void(std::uint32_t)> set_status; //639408/child2/61BF85.
  std::function<void(std::uint32_t)> close; //612604 reason3.
  std::function<void()> base_update; //612CD8, every invocation.
};
// Actual63956E with A68==NONE and callback642E8C or null. Completion clears
// A5C and the following invocation closes unless raw flag42 bit1 is set.
// Other async/callback owners reject before effects. Never opens another menu.
void vista_lobby_leave_task_frame(std::span<std::byte>,const VistaLobbyLeaveTaskFrameServices&);
} //namespace halo2
