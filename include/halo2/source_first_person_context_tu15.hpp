#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/source_runtime_resources.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

namespace halo2 {
class Cache;
class PhysicsLiveWorldTu15;
// Portable source values, never native asset/residency addresses. Runtime
// providers resolve these full owner identities against the recipient snapshot.
struct SourceFirstPersonContextIdentityTu15 {
    std::uint32_t player=0xffffffffu,unit=0xffffffffu,unit_definition=0xffffffffu;
    std::int32_t representation=-1;
    std::array<std::uint32_t,2> weapons{0xffffffffu,0xffffffffu};
    std::array<std::uint32_t,2> weapon_definitions{0xffffffffu,0xffffffffu};
    bool operator==(const SourceFirstPersonContextIdentityTu15&)const=default;
};
struct SourceFirstPersonTemporalHandTu15 {
    // Exact canonical source h[0..300): flags, full animation manager/extra
    // channels, feedback/timers, and the two authored64-entry node maps.
    // The current h+300 count and h+304..1008 matrix output are excluded.
    // The h+1008 sound handle and h+100C sound owner cookie are excluded.
    std::array<std::byte,0x300> control{};
    // Original32-byte local records, in native graph node order. Preserve
    // their source layout; C++ BonePose has a different member order.
    std::vector<std::array<std::byte,32>> saved_locals;
    bool operator==(const SourceFirstPersonTemporalHandTu15&)const=default;
};
struct SourceFirstPersonTemporalStateTu15 {
    std::uint64_t generation{},accepted_action_sequence{};
    SourceFirstPersonContextIdentityTu15 identity;
    std::uint32_t source_tick{},view_flags{};
    // Original clock owner[0..8): rate/flags and accepted step inputs. The
    // provider recreates its own clock root; no source address crosses a wire.
    std::array<std::byte,8> clock_controls{};
    // Source view+202C..205F filters/counters and2094 camera-node selection.
    // Neither2060 saved camera transform nor2098 camera result is transferred.
    std::array<std::byte,0x34> view_feedback{};
    std::int32_t camera_node=-1;
    std::array<SourceFirstPersonTemporalHandTu15,2> hands;
    bool operator==(const SourceFirstPersonTemporalStateTu15&)const=default;
};
// Capture at original F0DF1 ->16B736, before local/NONE-view dispatch.
// A remote producer supplies its private view binding; it never replays the
// gameplay owner which emitted this request or infers it from ammunition.
struct SourceFirstPersonActionRequestTu15 {
    std::uint64_t sequence{};
    std::uint32_t source_tick{},unit=0xffffffffu,weapon=0xffffffffu;
    std::int16_t action{};
    std::uint8_t hand{};
};
struct SourceFirstPersonContextInputTu15 {
    SourceFirstPersonContextIdentityTu15 identity;
    // Actual per-player18A0C0/18A300 record from prepare_player_look. This is
    // a producer input only; it is not the host's local-view/camera packet.
    std::array<std::byte,0x94> look{};
    // Declared original RNG words at context birth/map change. Native code
    // retains its private stream across ticks; it never writes gameplay RNG.
    std::array<std::uint32_t,2> initial_rng{};
};
// Persistent original temporal owner, separate from the read-only render
// sampler. Calls borrow a coherent authority input bank only for their duration.
// The Cache and admitted immutable assets outlive this owner. No whole World,
// host camera, inventory tick, weapon firing or real audio device is copied.
class SourceFirstPersonContextTu15 {
public:
    explicit SourceFirstPersonContextTu15(const Cache&,
        std::shared_ptr<const SourceRuntimeResources> resources={},
        SourcePhysicsWorldRuntimeServices cpu={});
    const SourceRuntimeResources::Identity &resources_identity()const noexcept;
    // Rebind only actual CPU operands at each genuine native owner callback;
    // no stale World/executor or required-owner callback is retained.
    void bind_cpu_services(const SourcePhysicsWorldRuntimeServices&);
    ~SourceFirstPersonContextTu15();
    // Authority continuation only. Unlike recipient pose packets, these own
    // native private cache/temporal ranges and require matching restored roots.
    struct PrivateRange {std::uint32_t address{};std::vector<std::byte> bytes;};
    struct PrivateReadJob {
        std::uint32_t identity{},graph{},index{},descriptor{},destination{},bytes{};
    };
    struct Checkpoint {
        SourceRuntimeResources::Identity resources;
        SourceFirstPersonTemporalStateTu15 temporal;
        bool initialized{};
        std::optional<std::uint32_t> last_advanced_tick;
        std::uint64_t next_job{};
        std::array<std::uint32_t,10> owner_roots{};
        std::uint32_t caller_stack{};
        std::vector<PrivateRange> private_ranges;
        std::vector<PrivateReadJob> read_jobs;
    };
    Checkpoint checkpoint()const;
    // Prepare a fresh owner before the Match registry's no-throw pointer swap.
    // This validates; it never advances a native clock or emits an event.
    void prepare_restore(const SourcePhysicsContactMemory&,const SourcePhysicsWorldRuntimeAbi&,
        const Checkpoint&);
    void adopt_local(const SourcePhysicsContactMemory&,const SourcePhysicsWorldRuntimeAbi&,unsigned view,
        const SourceFirstPersonContextIdentityTu15&,std::uint64_t generation,
        std::uint64_t action_sequence=0);
    // Staged authority role rebind only. Keep actual local audio/resource/RNG
    // owners; install portable temporal inputs and rebuild native current locals.
    static void install_local(const Cache&,PhysicsLiveWorldTu15&,unsigned view,
        const SourceFirstPersonTemporalStateTu15 &continuation,
        const SourceFirstPersonTemporalStateTu15 &sampling_inputs);
    static SourceFirstPersonTemporalStateTu15 capture_local(const Cache&,
        const SourcePhysicsContactMemory&,unsigned view,
        const SourceFirstPersonContextIdentityTu15&,std::uint64_t generation,
        std::uint64_t action_sequence=0);
    SourceFirstPersonContextTu15(const SourceFirstPersonContextTu15&)=delete;
    SourceFirstPersonContextTu15&operator=(const SourceFirstPersonContextTu15&)=delete;
    void reset(const SourcePhysicsContactMemory&,const SourcePhysicsWorldRuntimeAbi&,
        const SourceFirstPersonContextInputTu15&);
    void prepare_identity(const SourcePhysicsContactMemory&,const SourcePhysicsWorldRuntimeAbi&,
        const SourceFirstPersonContextInputTu15&);
    void apply_action(const SourcePhysicsContactMemory&,const SourcePhysicsWorldRuntimeAbi&,
        const SourceFirstPersonActionRequestTu15&);
    void advance_tick(const SourcePhysicsContactMemory&,const SourcePhysicsWorldRuntimeAbi&,
        const SourceFirstPersonContextInputTu15&);
    SourceFirstPersonTemporalStateTu15 state()const;
    // Inputs retained at each actual169C46 ->16A4DD call, before the shared
    // feedback clock advances. state()/checkpoint() remain completed native
    // continuation. Registry packets own these inputs across restoration.
    SourceFirstPersonTemporalStateTu15 pose_inputs()const;
    static SourceFirstPersonTemporalStateTu15 composition_inputs(
        const SourceFirstPersonTemporalStateTu15 &completed,
        const std::array<std::optional<SourceFirstPersonTemporalStateTu15>,2> &per_hand);
    // Receiver-only native binding/resource admission, followed by a readonly
    // borrow. No temporal update, action or event owner runs on this path.
    void with_sampling_inputs(const SourcePhysicsContactMemory&,
        const SourcePhysicsWorldRuntimeAbi&,const SourceFirstPersonTemporalStateTu15&,
        const std::function<void(const SourcePhysicsContactMemory&)> &,
        const SourcePhysicsWorldRuntimeServices &cpu={});
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace halo2
