#pragma once
#include "halo2/source_device.hpp"
#include "halo2/source_device_motion.hpp"
#include "halo2/source_control.hpp"
#include "halo2/source_control_target.hpp"
#include "halo2/content.hpp"
#include "halo2/model_markers.hpp"
#include <map>

namespace halo2 {
struct LevelContent;
class SceneObjects;
struct StaticSceneryCollisionDefinition;
struct CollisionTriangle;
struct SourceDeviceWorldDefinition {
    SourceDeviceMotionDefinition motion;
    SourceMachinePlacement placement; // Common device fields of mach/ctrl.
    std::optional<SourceMachineDefinition> machine;
    std::optional<SourceControlDefinition> control;
    std::shared_ptr<const StaticSceneryCollisionDefinition> collision;
    // Controls share the device position/power channels. Target bounds and
    // use markers consume the current simulation channel values.
    ShadowCasterDefinition control_bounds;
    std::vector<Transform> control_nodes;
    std::shared_ptr<const RenderModel> control_hierarchy;
    std::optional<SourceMachineDefinition> control_pose;
    std::vector<ObjectAttachmentMarker> control_markers;
    std::vector<std::uint8_t> control_permutations;
};
std::shared_ptr<const SourceDeviceWorldDefinition> load_source_device_world_definition(
    const Cache &,const ScenarioObject &);
struct SourceDeviceWorldOwner {
    std::uint32_t handle=~0u;
    std::size_t placement{};
    std::uint64_t created_tick{};
    SourceDeviceMotionState motion;
    std::uint32_t control_flags{};
    SourceDeviceGroup private_power,private_position;
    Transform root;
    bool bsp_hidden{},visible=true;
};
std::vector<Transform> source_control_world_nodes(const SourceDeviceWorldDefinition &,
                                               const SourceDeviceWorldOwner &);
// Native host transport of the retained source state. This is not an Xbox
// wire/save format. A complete bank replaces the previous client bank.
struct SourceDeviceWorldSnapshot {
    struct Owner {
        std::uint32_t definition=~0u;
        SourceDeviceWorldOwner state;
    };
    std::uint64_t tick{};
    std::vector<SourceDeviceGroup> groups;
    std::vector<Owner> owners;
};
// Structural transport checks. Definition/placement/group-count checks need
// the actual level and are performed by apply_snapshot before publication.
void validate_source_device_world_snapshot(const SourceDeviceWorldSnapshot &);
struct SourceDeviceWorldEvent {
    std::uint32_t handle=~0u,tag=~0u;
    // Native reliable-within-retention snapshot journal, assigned by the
    // simulation at its completed tick. Not an Xbox transport format.
    std::uint64_t serial{},tick{};
};
void validate_source_device_events(std::span<const SourceDeviceWorldEvent>,std::uint64_t tick);
class NativeDeviceEventCursor {
public:
    explicit NativeDeviceEventCursor(bool authority):baseline_(authority){}
    std::vector<SourceDeviceWorldEvent> observe(std::span<const SourceDeviceWorldEvent>);
private:
    bool baseline_{};
    std::uint64_t serial_{};
};
// Simulation-owned shared groups and per-generation ordinary device state.
// Rendering observes this owner; it must not run another motion clock.
class SourceDeviceWorld {
public:
    explicit SourceDeviceWorld(const LevelContent &);
    bool synchronize(const SceneObjects &);
    void step(float dt);
    bool synchronize_collision();
    std::span<const CollisionTriangle> triangles()const{return triangles_;}
    const SourceDeviceWorldOwner *find(std::uint32_t)const;
    const SourceDeviceGroup &position_group(const SourceDeviceWorldOwner &)const;
    const SourceDeviceGroup &power_group(const SourceDeviceWorldOwner &)const;
    std::span<const SourceDeviceGroup> groups()const{return groups_;}
    const std::map<std::uint32_t,SourceDeviceWorldOwner> &owners()const{return owners_;}
    SourceDeviceWorldSnapshot snapshot(std::uint64_t tick)const;
    void apply_snapshot(const SourceDeviceWorldSnapshot &);
    bool replicated()const{return replica_tick_.has_value();}
    std::optional<std::uint64_t> replica_tick()const{return replica_tick_;}
    // Geometry/marker/unit-center inputs remain actual caller services. Group
    // pointers and runtime control/device flags are bound from THIS owner;
    // callers cannot substitute a fresh private group for a used shared one.
    bool consider_control_target(const SourceControlTargetQuery &,
        SourceControlTargetCandidate,SourcePlayerActionTarget &,Vec3 current_root)const;
    // Only after original player/script action admission. No proximity or
    // interaction-priority decisions are implied by this entry point.
    SourceControlActivation activate_admitted_control(std::uint32_t);
    std::vector<SourceDeviceWorldEvent> take_events();
    float script_value(std::uint32_t handle,bool power)const;
    bool script_set(std::uint32_t handle,bool power,float value,bool immediate);
    float script_group_value(std::uint32_t group)const;
    bool script_group_set(std::uint32_t group,float value,bool immediate);
private:
    void script_immediate(SourceDeviceGroup &,float);
    const LevelContent *content_;
    std::vector<SourceDeviceGroup> groups_;
    std::map<std::uint32_t,SourceDeviceWorldOwner> owners_;
    std::vector<SourceDeviceWorldEvent> events_;
    bool collision_dirty_=true;
    std::vector<CollisionTriangle> triangles_;
    std::optional<std::uint64_t> replica_tick_;
    SourceDeviceGroup &group(SourceDeviceWorldOwner &,bool power);
};
}
