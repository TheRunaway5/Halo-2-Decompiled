#include "halo2/source_replay_batch.hpp"
#include "halo2/cache.hpp"
#include "halo2-online-protocol-bridge/src/lib.rs.h"
namespace halo2 {
using Kind=SourceExecutionCallKind;
static_assert(static_cast<unsigned>(Kind::DepartPlayer)==14&&static_cast<unsigned>(Kind::WriteSourceCallInput)==15&&
  static_cast<unsigned>(Kind::RenderGeometry)==23&&static_cast<unsigned>(Kind::MatchState)==24&&
  static_cast<unsigned>(Kind::PlayerCamera)==25&&static_cast<unsigned>(Kind::ParticleCollision)==29,
  "Update the Rust network capability policy when the call wire enum changes");
void validate_source_network_replay_calls(std::span<const SourceExecutionCall> calls){
  for(const auto &call:calls)
    if(!online_protocol_bridge::network_replay_call(static_cast<std::uint8_t>(call.kind)))
      throw FormatError("Source network replay contains a local-only execution capability: "+std::to_string(static_cast<unsigned>(call.kind)));
}
}
