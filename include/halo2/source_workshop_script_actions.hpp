#pragma once
#include "halo2/bloom.hpp"
#include <cstdint>
#include <string_view>
namespace halo2 {
class SourcePhysicsContactMemory;
class PhysicsLiveWorldTu15;
CinematicBloomOverride source_cinematic_bloom_state(const SourcePhysicsContactMemory &);
void source_cinematic_bloom_set(PhysicsLiveWorldTu15 &,std::string_view,std::uint32_t);
void source_object_cinematic_collision(PhysicsLiveWorldTu15 &,std::uint32_t,bool);
// Uses the retained original squad/group iterator and its real AI-enabled
// guard. Encoded group references must not be flattened to a single actor.
void source_ai_force_active(PhysicsLiveWorldTu15 &,std::uint32_t,bool);
void source_unit_set_enterable_by_player(PhysicsLiveWorldTu15 &,std::uint32_t,bool);
void source_object_cannot_die(PhysicsLiveWorldTu15 &,std::uint32_t,bool);
}
