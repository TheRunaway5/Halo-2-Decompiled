#pragma once
#include "halo2/physics_contact_record.hpp"
#include <stdexcept>
namespace halo2 {
// Real unresolved mode2 owner, reached after the reset's preceding state
// publications. It is a required native controller initialization, never an
// instruction dispatcher, supplied controller result or lifted fallback.
class NativeCampaignCameraResetRequiredOwnerTu15 final:public std::runtime_error {
    std::uint32_t record_;std::int32_t view_;
public:
    NativeCampaignCameraResetRequiredOwnerTu15(std::uint32_t record,std::int32_t view);
    std::uint32_t owner()const noexcept{return 0x246190u;}
    std::uint32_t record()const noexcept{return record_;}
    std::int32_t view()const noexcept{return view_;}
};
// Actual157EB0 ordinary clear-direction caller: default cold mode0 and all
// nonmode2 restoration branches are native. Mode2 retains its genuine required
// owner; no successful debug-controller initialization is supplied.
void native_campaign_camera_director_reset_tu15(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRecordObserver & = {});
}
