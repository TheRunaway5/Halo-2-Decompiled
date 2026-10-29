#pragma once
#include "halo2/physics_contact_record.hpp"

namespace halo2 {
// These commands use scalar SIMD arithmetic only. x87 values, status and
// control are untouched; the caller's complete MXCSR is explicit.
struct NativeCampaignCameraCommandFpTu15 {std::uint32_t mxcsr=0x1f80u;};
class NativeCampaignCameraFovArgumentsTu15 {
public:
    virtual ~NativeCampaignCameraFovArgumentsTu15()=default;
    virtual std::int16_t ticks()const=0;
    virtual std::uint32_t degrees_bits()const=0;
};
void native_campaign_cinematic_reset_tu15(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRecordObserver & = {});
void native_campaign_cinematic_stop_tu15(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRecordObserver & = {});
void native_campaign_camera_fov_tu15(SourcePhysicsContactMemory &,
    const NativeCampaignCameraFovArgumentsTu15 &,NativeCampaignCameraCommandFpTu15 &,
    const SourcePhysicsContactRecordObserver & = {});
}
