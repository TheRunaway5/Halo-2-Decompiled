#pragma once
#include <array>
#include <cstdint>
#include <span>
namespace halo2 {
struct CampaignTitleDefinition {
  std::uint32_t text{}, color{}, shadow{};
  std::array<std::int16_t, 4> bounds{};
  std::int16_t justification{}, font{};
  float fade_in{}, up_time{}, fade_out{};
};
struct CampaignTitle {
  std::int16_t index = -1, ticks = -1;
};
struct CampaignSubtitle {
  std::uint32_t text{};
  float remaining{};
  void start(std::uint32_t id, float seconds);
  void advance(float seconds);
  float alpha() const;
};
// Original game-state owner51E9C0 and input owner4ED284. State survives
// checkpoints; script evaluation reads actual accumulated player input.
struct TrainingEventDefinition {
  std::uint32_t text{};
  std::uint16_t display_seconds{}, limit{}, grace_seconds{}, cooldown_seconds{};
  float delay_seconds{};
  std::uint32_t flags{};
};
// Original1E6BD0 state machine. Admission/completion are supplied by the
// actual gameplay owners; unsupported vehicle and voice owners admit nothing.
struct AutomaticTrainingState {
  struct Transition {std::int16_t state{}, ticks{};};
  std::array<Transition,32> transitions{};
  std::array<std::uint8_t,32> learned{};
  std::int32_t current=-1, elapsed{};
  bool baseline{};
  std::array<float,3> origin{};
  bool turned_left{},turned_right{};
  void advance(std::span<const TrainingEventDefinition>,
      const std::array<bool,32> &admitted,const std::array<bool,32> &completed);
  std::uint32_t text(std::span<const TrainingEventDefinition>) const;
};
struct PlayerTrainingState {
  std::uint32_t text{}, actions{}, armed{}, suppressed{};
  std::uint8_t requests{};
  bool show{}, enabled = true;
  AutomaticTrainingState automatic;
  // 2AA840/2AA7F0/2AA7A0 write the enabled/text/show fields independently.
};
} // namespace halo2
