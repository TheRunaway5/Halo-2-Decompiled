#pragma once
#include "halo2/native_campaign_camera_commands_tu15.hpp"
namespace halo2 {
struct NativeCampaignCameraUnitModeTu15 {bool seat_camera{};std::uint16_t mode{};};
// ActualBCDF0 salted/unit-type validation. Unlike unchecked object roots this
// reads high-water, stride, rows, signed salt and type in original lazy order.
std::uint32_t native_campaign_camera_validated_unit_tu15(const SourcePhysicsContactMemory &,std::uint32_t unit);
NativeCampaignCameraUnitModeTu15 native_campaign_camera_unit_mode_value_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t unit);
bool native_campaign_camera_unit_mode_tu15(SourcePhysicsContactMemory &,std::uint32_t unit,
    std::uint32_t mode_destination,const SourcePhysicsContactRecordObserver & = {});
class NativeCampaignCameraBooleanArgumentTu15 {
public:
    virtual ~NativeCampaignCameraBooleanArgumentTu15()=default;
    virtual std::uint8_t value()const=0;
};
void native_campaign_camera_set_mode_tu15(SourcePhysicsContactMemory &,std::int32_t view,
    std::uint32_t callback,const NativeCampaignCameraBooleanArgumentTu15 &transition,
    NativeCampaignCameraCommandFpTu15 &,const SourcePhysicsContactRecordObserver & = {});
// Full158560, including true scripted activation and false restoration from
// actual registered player units/parent seat flags/animation state. The
// public argument is reread at the original late restoration phases.
void native_campaign_camera_control_tu15(SourcePhysicsContactMemory &,
    const NativeCampaignCameraBooleanArgumentTu15 &,NativeCampaignCameraCommandFpTu15 &,
    const SourcePhysicsContactRecordObserver & = {});
}
