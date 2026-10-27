#pragma once
#include "halo2/source_first_person_context_tu15.hpp"
#include "halo2/source_first_person_pose_tu15.hpp"
#include "halo2/fp_registration.hpp"
#include <span>

namespace halo2 {
// Typed dynamic operands of the original FP/ammunition routines. Definitions,
// graph bytes and codec resources are resolved from the recipient's Cache.
struct SourceFirstPersonWeaponInputTu15 {
    std::uint32_t object=0xffffffffu,definition=0xffffffffu,owner=0xffffffffu,item_flags{};
    float charge{};
    std::array<std::byte,32> magazines{};
};
struct SourceFirstPersonActorInputTu15 {
    std::uint32_t game_kind{};
    // Original BAC30 input to168DED's looking-down body, distinct from cached
    // FP outputs and the lazy-completed third-person rendering bank.
    std::vector<Transform> unit_world_nodes;
    // Original16B4E8 single-weapon IK weight: the unit extension's native
    // transition clock+80..82, consumed unchanged by1E1620. Its source body
    // address/relative layout stays local to each endpoint.
    std::array<std::byte,3> single_hand_ik_transition{};
    std::array<std::uint32_t,4> inventory{0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu};
    std::array<std::int8_t,2> current{-1,-1},requested{-1,-1};
    std::vector<SourceFirstPersonWeaponInputTu15> weapons;
};
struct SourceFirstPersonPosePacketTu15 {
    SourceRuntimeResources::Identity resources;
    // Codec4 carries the per-hand16A4DD entry inputs, including the shared
    // feedback clock BEFORE169C46 advances it. Authority continuation retains
    // the separate completed temporal state for the next native tick.
    SourceFirstPersonTemporalStateTu15 temporal;
    SourceFirstPersonActorInputTu15 actors;
};
SourceFirstPersonPosePacketTu15 source_capture_first_person_packet_tu15(
    const SourceRuntimeResources::Identity &,
    const SourceFirstPersonTemporalStateTu15 &,
    const SourcePhysicsContactMemory &);
void source_validate_first_person_packet_tu15(const SourceFirstPersonPosePacketTu15 &,
    const SourceFirstPersonContextIdentityTu15 &expected);
std::vector<std::byte> source_encode_first_person_packet_tu15(
    const SourceFirstPersonPosePacketTu15 &,
    const SourceFirstPersonContextIdentityTu15 &expected);
struct SourceFirstPersonPacketDecodeTu15 {
    SourceFirstPersonPosePacketTu15 packet;
    std::size_t consumed{};
};
SourceFirstPersonPacketDecodeTu15 source_decode_first_person_packet_tu15(
    std::span<const std::byte>,
    const std::optional<SourceFirstPersonContextIdentityTu15> &expected = {});
FirstPersonViewBinding source_first_person_packet_view_tu15(const SourceFirstPersonPosePacketTu15 &);

// A persistent local resource/binding owner. Each call installs only validated
// typed actor/control inputs, then invokes the original readonly native sampler.
// Viewer-local camera transforms never enter the portable packet.
class SourceFirstPersonPoseReceiverTu15 {
public:
    explicit SourceFirstPersonPoseReceiverTu15(const Cache &,
        std::shared_ptr<const SourceRuntimeResources> = {});
    ~SourceFirstPersonPoseReceiverTu15();
    std::vector<SourceFirstPersonModelTu15> sample(
        const SourceRuntimeMemory &local_assets,const SourcePhysicsWorldRuntimeAbi &,
        const SourceFirstPersonPosePacketTu15 &,
        const SourceFirstPersonContextIdentityTu15 &expected,
        Vec3 position,Vec3 forward,Vec3 up,
        const SourcePhysicsWorldRuntimeServices &);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
