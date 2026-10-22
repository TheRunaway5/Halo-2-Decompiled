#pragma once
#include "halo2/physics_body_factory.hpp"
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
class SourceRuntimeMemory;
// Complete original engine entity state, built by virtual slots44/45.
// This contains original data words/datum keys, never native host pointers.
// It is a state projection; transport and client application have separate
// owners. A changed mask compares against the original slot44 defaults.
struct SourceMultiplayerEngineEntityTu15 {
  std::int32_t type{};
  std::uint32_t changed{};
  std::vector<std::byte> bytes;
  // Original198680 consumes published Juggernaut bank+2. Its zero case
  // differs from a simple role-bit test; retain the actual operand separately.
  std::uint16_t relationship_mask{};
  bool operator==(const SourceMultiplayerEngineEntityTu15&)const=default;
};
// Optional caller-owned diagnostic inputs for unchanged-Xbox comparison.
// Private query-stack reads are excluded; these are actual world reads only.
struct SourceMultiplayerEngineEntityReadTu15 {
  std::uint32_t address{};
  std::vector<std::byte> bytes;
};
std::size_t source_multiplayer_engine_entity_size_tu15(std::int32_t type);
// Original slot27:25BC70 or198680 over the native membership domain0..15.
// True means enemies; identical memberships always return false.
bool source_multiplayer_engine_entity_enemies_tu15(
    const SourceMultiplayerEngineEntityTu15&,unsigned left,unsigned right);
// Execute the unchanged native callback bodies over the actual retained
// source reads, with caller-owned argument/output storage. Every attempted
// write outside that private caller frame fails. The world is never advanced
// or changed, and no successful callee return is supplied.
SourceMultiplayerEngineEntityTu15 source_multiplayer_engine_entity_tu15(
    const SourceRuntimeMemory&,SourcePhysicsFactoryFloatingPoint,
    std::vector<SourceMultiplayerEngineEntityReadTu15>* evidence=nullptr);
} // namespace halo2
