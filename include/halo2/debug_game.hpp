#pragma once
#include "halo2/cache.hpp"
#include <array>
#include <string>
#include <vector>

namespace halo2 {
// Optional native workbench controls. These are not retail gameplay rules.
struct DebugPlayerSettings {
    bool invulnerable{}, unlimited_ammo{}, noclip{}, enemies_ignore{}, maximum_damage{};
};
struct DebugGameSettings {
    std::array<DebugPlayerSettings,16> players{};
};
struct DebugPlayerObservation {
    unsigned slot{};
    std::string name;
    Vec3 position;
    float health{}, shield{};
    int loaded{}, reserve{};
    bool dead{};
};
struct DebugTeleport { std::string name; Spawn spawn; };
struct DebugGameObservation {
    std::string mode = "Frontend", limitation = "Gameplay controls become available in a local session.";
    std::uint64_t tick{};
    bool available{}, ai_available{}, snapshots_available{}, checkpoint_available{};
    std::vector<DebugPlayerObservation> players;
    std::vector<DebugTeleport> teleports;
    std::vector<std::string> snapshots;
};
struct DebugGameRequest {
    enum Kind { Heal, Teleport, SaveCheckpoint, RevertCheckpoint, SaveSnapshot, LoadSnapshot, DeleteSnapshot } kind;
    unsigned player{}, index{};
    std::string name;
};
class Simulation;
DebugGameObservation observe_debug_game(const Simulation&,std::span<const Spawn>);
void apply_debug_request(Simulation&,const DebugGameRequest&,std::span<const Spawn>);
} // namespace halo2
