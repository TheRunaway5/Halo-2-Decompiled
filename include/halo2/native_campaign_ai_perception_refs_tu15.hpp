#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Real2673A0: policy-controlled tracked-object/group position storage.
std::uint32_t native_campaign_ai_perception_position_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t perception);
// Real267450: optional tracked-object extension belonging to a perception.
std::uint32_t native_campaign_ai_perception_tracking_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t perception);
// Real267410: optional tracked-object extension through the real datum table.
std::uint32_t native_campaign_ai_perception_datum_tracking_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t datum);
}
