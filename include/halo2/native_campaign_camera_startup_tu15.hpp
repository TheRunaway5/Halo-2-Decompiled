#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Complete startup constructors1888A0,157E10,13E9E0. Storage is the actual
// previously owned game arena; size-accounting CRC consumes native values.
void native_campaign_camera_input_storage_tu15(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRecordObserver & = {});
void native_campaign_camera_director_storage_tu15(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRecordObserver & = {});
void native_campaign_cinematic_storage_tu15(SourcePhysicsContactMemory &,
    const SourcePhysicsContactRecordObserver & = {});
enum class NativeCampaignCameraDefaultTu15 {Angle,Minimum,Maximum};
void native_campaign_camera_default_tu15(SourcePhysicsContactMemory &,NativeCampaignCameraDefaultTu15,
    const SourcePhysicsContactRecordObserver & = {});
}
