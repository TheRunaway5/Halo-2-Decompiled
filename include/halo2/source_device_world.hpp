#pragma once
#include "halo2/source_device.hpp"
#include "halo2/source_device_motion.hpp"
#include "halo2/source_control.hpp"
#include "halo2/source_control_target.hpp"
#include "halo2/content.hpp"
#include "halo2/model_markers.hpp"
#include "halo2/native_authority_boundary.hpp"
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
    std::shared_ptr<const RenderModel> door_hierarchy;
    std::vector<std::uint8_t> door_permutations;
    std::vector<ObjectAttachmentMarker> one_sided_markers;
};
std::shared_ptr<const SourceDeviceWorldDefinition> load_source_device_world_definition(
    const Cache &,const ScenarioObject &);
struct SourceDeviceWorldOwner {
    std::uint32_t handle=~0u;
    std::size_t placement{};
    std::uint64_t created_tick{};
    SourceDeviceMotionState motion;
    std::uint32_t control_flags{};
    SourceMachineDoorState door;
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
    // Completed-frame accepted-world replacement. Baseline the validated
    // incoming journal without replaying its old requests. A later event may
    // reuse a serial from the discarded future. Rejection preserves this
    // cursor; authentication and generation agreement belong to the caller.
    void rebase(std::span<const SourceDeviceWorldEvent>,std::uint64_t tick);
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
    struct DoorWorld {
        std::uint32_t tick{};
        bool editor{};
        std::function<std::span<const SourceMachineDoorUnit>(std::uint32_t machine,
            Vec3 center,float radius)> query;
    };
    // Live worlds supply their current unit query and game clock. The scalar
    // overload retains the existing common-device/gear-only fixture seam.
    void step(float dt,const DoorWorld &);
    bool synchronize_collision();
    std::span<const CollisionTriangle> triangles()const{return triangles_;}
    const SourceDeviceWorldOwner *find(std::uint32_t)const;
    const SourceDeviceGroup &position_group(const SourceDeviceWorldOwner &)const;
    const SourceDeviceGroup &power_group(const SourceDeviceWorldOwner &)const;
    std::span<const SourceDeviceGroup> groups()const{return groups_;}
    const std::map<std::uint32_t,SourceDeviceWorldOwner> &owners()const{return owners_;}
    SourceDeviceWorldSnapshot snapshot(std::uint64_t tick)const;
    void apply_snapshot(const SourceDeviceWorldSnapshot &);
    // Read back completed live TU1.5 state without binding a network replica.
    void observe_source_snapshot(const SourceDeviceWorldSnapshot &);
    // Complete native device owner, separate from a display/replica snapshot.
    // A match continuation must validate all other owners and its completed
    // tick before publishing this restored authority.
    struct AuthorityCheckpoint {
        std::uint32_t version=2;
        SourceDeviceWorldSnapshot state;
        std::vector<SourceDeviceWorldEvent> pending_events;
        std::array<std::byte,32> definition_binding{};
        // Exact last-built native geometry and its pose inputs. A dirty owner
        // must retain the old cache until the ordinary synchronize boundary.
        struct CollisionPose {
            std::uint32_t handle=~0u;
            std::size_t placement{};
            std::uint64_t created_tick{};
            Transform root;
            float position{},power{};
            bool bsp_hidden{};
        };
        std::map<std::uint32_t,CollisionPose> collision_poses;
        std::vector<CollisionTriangle> triangles;
        bool collision_dirty{};
    };
    class PreparedAuthority {
    public:
        ~PreparedAuthority();
        PreparedAuthority(PreparedAuthority &&) noexcept;
        PreparedAuthority &operator=(PreparedAuthority &&) noexcept;
        bool for_owner(const SourceDeviceWorld &)const noexcept;
    private:
        friend class SourceDeviceWorld;
        PreparedAuthority();
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
    AuthorityCheckpoint authority_checkpoint(std::uint64_t tick)const;
    PreparedAuthority prepare_authority(const AuthorityCheckpoint &,std::uint64_t completed_tick);
    bool commit_authority(PreparedAuthority &&) noexcept;
    // Convenience wrapper over staged publication. The root still owns the
    // barrier over scene identities, borrowers and its composed collision.
    void restore_authority(const AuthorityCheckpoint &,std::uint64_t completed_tick);
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
    friend class Simulation;
    NativeAuthorityBoundary authority_;
    void script_immediate(SourceDeviceGroup &,float);
    const LevelContent *content_;
    std::vector<SourceDeviceGroup> groups_;
    std::map<std::uint32_t,SourceDeviceWorldOwner> owners_;
    std::vector<SourceDeviceWorldEvent> events_;
    bool collision_dirty_=true;
    std::vector<CollisionTriangle> triangles_;
    std::map<std::uint32_t,AuthorityCheckpoint::CollisionPose> collision_poses_;
    std::optional<std::uint64_t> replica_tick_;
    SourceDeviceGroup &group(SourceDeviceWorldOwner &,bool power);
    void step_impl(float dt,const DoorWorld *);
    void apply_state(const SourceDeviceWorldSnapshot &,bool replica);
};
}
