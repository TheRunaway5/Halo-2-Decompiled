#pragma once
#include "halo2/vista_session_birth.hpp"
#include <cstdint>
#include <optional>
#include <span>

namespace halo2 {
// Literal supplied-PC5D991C with unchanged getters5C2513/5C20C8/5C24DB,
// row builder5D9852, address copy5B5933, and setters5C5EB7/5C5CE3/5C5D3F.
// Consumes the actual existing0x7B88 session bank owned by the birth caller.
// It constructs no membership/player identity/profile. The source copies
// opaque fields directly; team-byte handling remains the literal source rule.
// Returns actual AL truth. State!=5 or an already prepared bank preserves all
// bytes. Positive peer counts above18 or a consumed row peer index above17
// reject atomically:18 is this bounded scratch bank's capacity, not a claim
// about an original valid network membership limit. Negative counts preserve
// the original no-loop and modulo32 shift behavior. Bytes beyond0x7B88 retain.
bool vista_session_prepare_players(std::span<std::byte> actual_session);

struct VistaSessionParameterRefreshResult {
  bool accepted{}; // Actual5C5BC5 AL; state5 returns true even for count<=1.
  bool clock_consumed{};
};
// Literal5C5BC5 joined with5C4F33 for its actual first argument0. Actualstate5
// and existing peer count are read from the same owned bank. Count>1 consumes
// an actual network-clock sample, clears58Ch bytes at+73A4, copies the exact
// refresh record and enters state7. It does not acknowledge or complete that
// state: real transport/update callbacks own its eventual departure. target
// -1 selects all peers except the source host; shifts wrap modulo32. Missing
// consumed clock/truncated bank rejects atomically. Unconsumed clock ignored.
VistaSessionParameterRefreshResult vista_session_refresh_parameters(
    std::span<std::byte> actual_session,std::int32_t target,
    std::uint8_t flag,std::optional<std::uint32_t> actual_network_clock={});
} // namespace halo2
