#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct NativeResultState;
struct NativeResultAdmission;
struct NativeResultRecording;
struct NativeResultRanks;
struct NativeResultLivePlayer;
struct NativeResultPublication;
// Actual native game observations and original numeric/profile/ranking IO.
// Rust retains no native pointer or view after the synchronous caller returns.
class NativeResultIo {
public:
  virtual ~NativeResultIo() = default;
  virtual NativeResultState result_state() const = 0;
  virtual rust::Vec<NativeResultAdmission> result_roster() const = 0;
  virtual std::uint32_t result_round_flags() const = 0;
  virtual rust::Vec<std::uint8_t> result_variant() const = 0;
  virtual NativeResultRecording result_recording() const = 0;
  virtual NativeResultRanks result_ranks(bool,std::uint32_t) const = 0;
  virtual bool result_changing_teams() const = 0;
  virtual NativeResultLivePlayer result_live_player(std::uint32_t) const = 0;
  virtual rust::Vec<std::uint8_t> result_configuration(std::uint32_t) const = 0;
  virtual rust::Vec<std::uint8_t> result_name(std::uint32_t) const = 0;
  virtual std::int32_t result_team_score(std::uint32_t) const = 0;
  virtual std::int16_t result_accumulated_score(std::uint32_t) const = 0;
  virtual void result_write(const NativeResultPublication&) const = 0;
};
}
