#pragma once
#include "halo2/multiplayer.hpp"

namespace halo2::test {
// Supplied scene setup belongs to the test fixture, never the shipped game API.
inline void place_unit(Unit& unit,const Spawn& spawn) {
    unit.body.position=spawn.position;
    unit.body.velocity={};
    unit.body.grounded=false;
    unit.yaw=spawn.facing;
    unit.pitch=0;
    unit.route.clear();unit.waypoint=0;
}
inline void place_player(MultiplayerMatch& match,unsigned slot,const Spawn& spawn) {
    place_unit(const_cast<Unit&>(match.unit(slot)),spawn);
}
inline void place_player(Simulation& simulation,const Spawn& spawn) {
    place_unit(const_cast<Unit&>(simulation.player()),spawn);
}
inline void restore_vitality(MultiplayerMatch& match,unsigned slot) {
    auto& unit=const_cast<Unit&>(match.unit(slot));
    unit.vitality.body=unit.definition.vitality.health>0?unit.definition.vitality.body_max:0;
    unit.vitality.shield=unit.definition.vitality.shield>0?1.f:0;
    unit.vitality.body_stun=unit.vitality.shield_stun=0;
}
} // namespace halo2::test
