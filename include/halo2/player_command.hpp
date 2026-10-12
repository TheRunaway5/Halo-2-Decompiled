#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace halo2 {
// Literal original 14E780 output and 14E7E0 validator. These do not read a
// device, produce a codec/submission record or establish player ownership.
void initialize_source_neutral_command(std::span<std::byte, 0x5c>);
bool source_player_command_valid(std::span<const std::byte, 0x5c>);

// 85460's measured cached-command branch: the caller supplies the actual
// simulation record and source-owned clock/configuration inputs. Watcher
// refresh kinds 4/5 require a separate producer and fail here. Failed delivery
// leaves all output bytes untouched, as in the original measured branch.
bool deliver_source_cached_command(std::span<const std::byte, 0x88> record,
    std::span<std::byte, 0x5c> output, std::uint32_t tick,
    float seconds_per_tick, float expiration);

// The player's actual current object, independently of the target unit passed
// to 151C90. These are source-owned inputs, not Body.grounded/world velocity.
// restricted_e5680 is the actual result of that separate unit/clock consumer.
// parent_seat_flags is required only when related_object_13c and seat are set.
struct SourceCommandCurrentUnit {
  std::uint32_t datum;
  std::uint8_t object_type;
  std::uint32_t parent;
  std::uint8_t physical_state;
  bool restricted_e5680;
  std::array<float,3> forward;
  std::uint32_t related_object_13c;
  std::int16_t seat;
  std::optional<std::uint32_t> parent_seat_flags;
};
struct SourcePlayerControl {
  std::array<std::byte,0x7c> packet;
  std::uint32_t target_unit_flags134;
  bool movement_basis_admitted;
};

// Bounded 151C90/151AA0, including 151980, 30BD0, E7B10, 1218C0 and
// C8250. TU1.5 has independently checked corresponding instructions.
// Produces the complete control packet and the preceding target+134 store;
// the caller next invokes the separate C8140/unit_input consumer.
// The parent-seat CCC60 transform is unavailable: returns nullopt before any
// output when its unmeasured positive branch or missing seat flags are met.
// Input is an already delivered command; this neither validates nor applies
// device scaling, submission codec, DE960 deadzone or locomotion selection.
// Stored outputs use the measured 53-bit x87/masked, nearest FP environment;
// CPU flags/exceptions and Xbox transcendental hardware parity are not outputs.
std::optional<SourcePlayerControl> produce_source_player_control(
    std::span<const std::byte,0x5c> command,
    const SourceCommandCurrentUnit &current,
    std::uint32_t target_unit_flags134);
} // namespace halo2
