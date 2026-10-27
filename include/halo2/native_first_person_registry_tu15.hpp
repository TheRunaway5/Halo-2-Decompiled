#pragma once
#include "halo2/source_first_person_action_tu15.hpp"
#include "halo2/source_first_person_packet_tu15.hpp"
#include <array>
#include <memory>
#include <optional>
#include <span>

namespace halo2 {
class Cache;
class PhysicsLiveWorldTu15;
// Authority-only owning continuation. Recipient packets remain the separate
// portable input DTO; these private native cache ranges never cross that wire.
struct NativeFirstPersonRegistryEntryTu15 {
    std::uint64_t generation{},action_sequence{};
    std::optional<unsigned> local_view;
    std::optional<SourceFirstPersonContextTu15::Checkpoint> context;
    std::optional<SourceFirstPersonPosePacketTu15> packet;
};
struct NativeFirstPersonRegistryCheckpointTu15 {
    SourceRuntimeResources::Identity resources;
    std::array<NativeFirstPersonRegistryEntryTu15,16> players;
};
class NativeFirstPersonRegistryTu15 {
public:
    struct Binding {
        std::uint32_t player=0xffffffffu,unit_definition=0xffffffffu;
        int representation=-1;
        bool enabled{};
    };
    using Bindings=std::array<Binding,16>;
    explicit NativeFirstPersonRegistryTu15(std::shared_ptr<const Cache>);
    ~NativeFirstPersonRegistryTu15();
    void action(const SourcePhysicsContactMemory&,const SourcePhysicsWorldRuntimeAbi&,
        const SourcePhysicsWorldRuntimeServices&,const SourceFirstPersonNativeActionTu15&,
        const Bindings&,std::span<const unsigned> local_players);
    // Exact169C46 ->16A4DD precomposition boundary for an actual local hand.
    void composition(const SourcePhysicsContactMemory&,unsigned view,unsigned hand,
        const Bindings&,std::span<const unsigned> local_players);
    void phase(const SourcePhysicsContactMemory&,const SourcePhysicsWorldRuntimeAbi&,
        const SourcePhysicsWorldRuntimeServices&,const Bindings&,std::span<const unsigned> local_players);
    void retire(unsigned slot)noexcept;
    void install_local_views(PhysicsLiveWorldTu15&,std::span<const unsigned> local_players);
    std::optional<SourceFirstPersonPosePacketTu15> packet(unsigned slot,std::uint32_t completed_tick,
        std::uint32_t player,std::uint32_t unit)const;
    NativeFirstPersonRegistryCheckpointTu15 checkpoint()const;
    std::unique_ptr<NativeFirstPersonRegistryTu15> prepare_restore(
        const SourcePhysicsContactMemory&,const SourcePhysicsWorldRuntimeAbi&,
        const NativeFirstPersonRegistryCheckpointTu15&,
        const Bindings&,std::uint32_t completed_tick)const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
