#pragma once
#include "halo2/physics_contact_record.hpp"
#include "halo2/scripts.hpp"
#include <array>
#include <bit>

namespace halo2 {
using ScriptTickRate = std::function<std::int16_t()>;

// TU1.5's game-time owner (51DF54) retains the same signed-short rate used
// by MCC's 706770/706780 getters. Resolve it on each call: checkpoint restore
// may replace the bank and the game-time record behind this pointer.
inline std::int16_t source_script_tick_rate(const SourcePhysicsContactMemory &memory) {
  std::array<std::byte,4> pointer{};
  memory.read(0x51df54u,pointer);
  std::uint32_t address{};
  for(unsigned i=0;i<pointer.size();++i)
    address|=std::uint32_t(std::to_integer<unsigned>(pointer[i]))<<(i*8u);
  if(!address || address>UINT32_MAX-3u)
    throw FormatError("Script clock requires a live original game-time owner");
  std::array<std::byte,2> rate{};
  memory.read(address+2u,rate);
  const auto bits=std::uint16_t(std::to_integer<unsigned>(rate[0]) |
      (std::to_integer<unsigned>(rate[1])<<8u));
  return std::bit_cast<std::int16_t>(bits);
}

inline ScriptVm::InitialBuiltin mcc_script_clock_builtins(ScriptTickRate tick_rate) {
  if(!tick_rate)return {};
  return [tick_rate=std::move(tick_rate)](const std::string &name,
      const std::vector<ScriptValue> &args)->std::optional<ScriptValue> {
    // The Xbox same-name function is VOID(LONG), not this LONG() getter.
    if(!args.empty() || (name!="game_tick_rate" && name!="game_tick_rate_scalar"))return {};
    const std::int32_t rate=tick_rate();
    const std::int32_t value=name=="game_tick_rate"?rate:rate/30;
    return ScriptValue{8,std::uint32_t(value),{}};
  };
}
}
