#pragma once
#include "halo2/native_tu15_data_array_iteration.hpp"
#include "halo2/native_tu15_physics_frame.hpp"
#include "halo2/native_tu15_retained_counter_update.hpp"
#include <map>

namespace halo2 {
enum class NativeCustomMechanicsSharedOwner : std::uint32_t {
    shared_2482f0=0x2482f0, loose_weapons_15c4e0=0x15c4e0,
    netgame_rows_15d890=0x15d890, local_roster_165860=0x165860,
    teleporters_1a6830=0x1a6830, team_lifetime_15e4b0=0x15e4b0,
    player_fanout_15ea50=0x15ea50, player_status_161610=0x161610,
    countdown_161720=0x161720, player_flags_1617a0=0x1617a0
};
enum class NativeCustomMechanicsPlayerOwner : std::uint32_t {
    camouflage_15dc90=0x15dc90, lifecycle_15e540=0x15e540,
    maintenance_1649b0=0x1649b0, engine_state_1654b0=0x1654b0
};
enum class NativeCustomMechanicsPhase { player_status_completed, before_fade_tail };
struct NativeCustomRosterMaskRequest {
    std::uint32_t instruction,address;
    unsigned width;
    std::uint32_t and_bits,or_bits;
};
struct NativeCustomMechanicsFrameServices {
    // All services execute genuine original children, preserving their real
    // FP result even on unwind. argument is engine+7DC for15D890, team for
    //15E4B0, and zero for the other shared children. It is never a result.
    std::function<void(NativeCustomMechanicsSharedOwner,std::uint32_t argument,
                       NativeTu15PhysicsFrameFp&)> shared;
    std::function<void(NativeCustomMechanicsPlayerOwner,std::uint32_t full_player,
                       NativeTu15PhysicsFrameFp&)> player;
    // Exactly two native parent RMW operands. The storage owner must admit
    // the whole writable extent before reading DATA, commit the mask once,
    // and return actual before/written bytes. No fabricated outcome is legal.
    std::function<NativeTu15RetainedCounterUpdate(const NativeCustomRosterMaskRequest&)> roster_mask;
    // Bank/FP phase publication only; it does not reconstruct a CPU image.
    std::function<void(NativeCustomMechanicsPhase,NativeTu15PhysicsFrameFp&)> phase;
};
struct NativeCustomMechanicsFrameStats {
    std::uint64_t calls{},active_calls{},eligible_players{},roster_mask_writes{},
        frame_counter_writes{},fade_stores{},native_virtual_noops{},refusals{};
    std::map<std::uint32_t,std::uint64_t> completed_children;
};
// Complete1A8E50 retained inputs with a private typed iterator. The initial
//16EA60 scan and subsequent source flags2/bitmap loop preserve read phases.
bool native_custom_player_iterator_advance_tu15(SourcePhysicsContactMemory&,
    NativeTu15DataArrayIterator&);
bool native_custom_player_shield_allowed_tu15(SourcePhysicsContactMemory&,std::uint32_t full_player);
bool native_custom_team_active_tu15(SourcePhysicsContactMemory&,std::uint32_t team);
bool native_custom_player_respawn_allowed_tu15(SourcePhysicsContactMemory&,std::uint32_t full_player);
// Actual15ECB2..15ECDF retained subphase; returns the source's cached engine
// identity for the following team/engine calls. This is typed DATA state.
std::uint32_t native_custom_publish_roster_count_tu15(SourcePhysicsContactMemory&,
    std::uint32_t eligible_count,const NativeCustomMechanicsFrameServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeCustomMechanicsFrameStats* = nullptr);
// Direct retained15ED59..15EF94 tail. Kept separately callable so genuine
// original hardware can qualify its FP/read/store/fault contract without
// pretending unconverted shared children have supplied successful outputs.
void native_custom_mechanics_frame_tail_tu15(SourcePhysicsContactMemory&,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const SourcePhysicsContactRecordObserver& = {},NativeCustomMechanicsFrameStats* = nullptr);
// Shared15EBA0 scheduling/retained mechanics. Custom policy replaces ONLY
//15E870,15E440,2489F0. Every other child remains real and caller-owned, and
// whole15E540 lifecycle remains required. No CPU stack/register file or
// successful child postimage is accepted. Full instruction/private-frame
// observation is outside this typed API; retained store observation remains.
void native_custom_mechanics_frame_tu15(SourcePhysicsContactMemory&,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth,
    const NativeCustomMechanicsFrameServices&,
    const SourcePhysicsContactRecordObserver& = {},NativeCustomMechanicsFrameStats* = nullptr);
}
