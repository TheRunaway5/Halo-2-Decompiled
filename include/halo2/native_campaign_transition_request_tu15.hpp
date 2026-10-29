#pragma once
#include "halo2/physics_contact_record.hpp"
#include <array>
#include <functional>
#include <stdexcept>
namespace halo2 {
// A platform input owner, called after the genuine41B774 pointer read and
// immediately before the volatile DWORD load. Production must refresh that
// actual imported clock through its existing monotonic input journal.
using NativeCampaignTransitionClockTu15=std::function<void(std::uint32_t clock_cell)>;
struct NativeCampaignTransitionOptionsTu15 {std::array<std::byte,0x1118> bytes;};
struct NativeCampaignTransitionRequestRequiredOwnerTu15:std::runtime_error {
    std::uint32_t site=0x1915ebu,target=0x5a080u;
    NativeCampaignTransitionRequestRequiredOwnerTu15()
        :std::runtime_error("Native transition request requires its actual UI/session continuation5A080"){}
};
//191580: exact forward DWORD publication, pending flags and real33B590 clock
// read. Every original ordinary return branch is native; reached5A080 stops
// after its actual preceding publications, never acknowledging that owner.
void native_campaign_transition_request_tu15(SourcePhysicsContactMemory &,
    std::uint32_t retained_options,const NativeCampaignTransitionClockTu15 &);
// The actual191750 caller's private/disjoint options can remain a native
// value. No guest stack or temporary retained bank is reconstructed.
void native_campaign_transition_request_value_tu15(SourcePhysicsContactMemory &,
    const NativeCampaignTransitionOptionsTu15 &,const NativeCampaignTransitionClockTu15 &);
}
