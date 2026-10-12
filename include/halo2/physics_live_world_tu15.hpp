#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <filesystem>
#include <memory>
#include "halo2/transform.hpp"
#include "halo2/source_player_profile.hpp"

namespace halo2 {
class Cache;
struct SourceGameVariant;
struct ScenarioObject;
struct SourcePlayerEquipmentTu15 {
    std::array<std::uint32_t,4> weapons,definitions,ready_ticks;
    std::array<std::int8_t,2> current,requested;
    std::array<std::uint8_t,2> grenades;
    std::uint16_t inventory_sequence,replicated_sequence;
};
// One fresh TU1.5 bank and its native process/arena/pool resources. Source
// constructors own all allocator, renderer, world and body state. The host
// process binding is explicit; it is not a claim of complete Xbox boot.
class PhysicsLiveWorldTu15 {
public:
    PhysicsLiveWorldTu15(const Cache &,const std::filesystem::path &xbe,
                        const std::filesystem::path &savegame_directory,std::size_t bsp=0);
    ~PhysicsLiveWorldTu15();
    PhysicsLiveWorldTu15(const PhysicsLiveWorldTu15 &)=delete;
    PhysicsLiveWorldTu15 &operator=(const PhysicsLiveWorldTu15 &)=delete;
    SourceRuntimeMemory &memory();
    // Complete source entries over this same bank, with a retained full ABI.
    // Correspondence IDs select authenticated TU1.5 code only.
    void run(std::uint32_t canonical_entry,SourcePhysicsWorldRuntimeAbi &,
             const SourcePhysicsContactRecordObserver & = {});
    void run_actual(std::uint32_t tu15_entry,SourcePhysicsWorldRuntimeAbi &,
                    const SourcePhysicsContactRecordObserver & = {});
    std::uint32_t call(std::uint32_t canonical_entry,
                       std::span<const std::uint32_t> arguments={});
    std::uint32_t call_actual(std::uint32_t tu15_entry,
                             std::span<const std::uint32_t> arguments={});
    std::uint32_t world() const;
    void initialize_map();
    std::uint32_t object_pool() const;
    std::uint32_t player_pool() const;
    void construct_player(std::uint32_t player,unsigned native_local_user,
                          const SourcePlayerConfiguration &,const SourceMachineIdentity &);
    void bind_player(std::uint32_t player,std::uint32_t object);
    // Original152078..152098 post-birth activation and full1613B0 equipment
    // setup.1A8B50 selects authored loadouts and creates/attaches real weapons.
    SourcePlayerEquipmentTu15 initialize_player_equipment(std::uint32_t player,
                                                          std::uint32_t object);
    SourcePlayerEquipmentTu15 player_equipment(std::uint32_t object) const;
    // Native controls deliver the original command record. Complete153070
    // produces the control packet and C92C0 applies it to the same source unit.
    void deliver_command(std::uint32_t player,std::uint32_t object,
                         std::span<const std::byte,0x5c> command);
    // Actual13AAC0's five object/world phases; its clock increment follows
    // the phases at13ABD8. Full game-frame scheduling is separately owned.
    void step_objects();
    std::uint32_t object_body(std::uint32_t full_datum) const;
    void configure_session(const SourceGameVariant &,std::uint16_t simulation_mode,
                           std::int16_t tick_rate,std::uint32_t random_seed);
    // One complete original game physics frame (actual1CC8D0), using the
    // source clock configured for this session and the owner's retained ABI.
    // Command delivery and game-clock advancement belong to the game tick.
    // A constructed world and admitted session are required; missing callees
    // propagate with their real source state rather than falling back to host
    // collision movement.
    SourcePhysicsWorldRuntimeAbi step_physics(
        const SourcePhysicsContactRecordObserver & = {});
    std::uint32_t create_biped(std::uint32_t definition,const Vec3 &position,
                               const Vec3 &forward);
    // Actual C1120 receives the original placement and palette addressed by
    // the source type table. D75A0 owns request construction, including UID,
    // frozen state, authored orientation, scale and type-specific fields.
    std::uint32_t create_scenario_object(const ScenarioObject &);
    // Actual15D4F0 consumes the original netgame row and admitted variant,
    // selects its collection, constructs its body and retains its row owner.
    std::uint32_t create_netgame_object(unsigned row);
    // Complete BA3E0 deactivation and BA490 deletion precede header retirement.
    void destroy_object(std::uint32_t full_datum);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace halo2
