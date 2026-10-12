#pragma once
#include "halo2/cache.hpp"
#include <SDL.h>
#include <functional>
namespace halo2 {
struct RumbleMotor {
  float duration{};
  std::vector<std::byte> curve;
};
struct RumbleEffect {
  std::int16_t type{};
  std::array<RumbleMotor, 2> motors;
};
using RumbleDefinition = std::vector<RumbleEffect>;
RumbleDefinition load_rumble_definition(const Cache &, std::uint32_t);
struct RumbleEvent {
  std::uint32_t definition = 0xffffffffu;
  bool shielded = true;
  float scale = 1;
  int owner = -1;
};
struct RumbleJournalEvent {
  RumbleEvent event;
  std::uint64_t serial{}, tick{};
};
void validate_rumble_journal(std::span<const RumbleJournalEvent>,
                             std::uint64_t tick);
struct RumbleOutput {
  std::uint16_t left{}, right{};
};
int send_controller_rumble(SDL_GameController *, RumbleOutput, bool enabled,
                           bool paused, bool focused);
// Whole222CB0/222E90: eight oldest-timer slots; additive motor curves.
class RumblePlayer {
public:
  void play(std::shared_ptr<const RumbleDefinition>, unsigned effect,
            float scale);
  void play(std::shared_ptr<const RumbleDefinition>, bool shielded,
            float scale);
  RumbleOutput evaluate(const ShaderFunctionTables &) const;
  void advance(float seconds);
  void clear();

private:
  struct Slot {
    std::shared_ptr<const RumbleDefinition> definition;
    unsigned effect{};
    float time{}, scale = 1;
  };
  std::array<Slot, 8> slots_;
};
} // namespace halo2
