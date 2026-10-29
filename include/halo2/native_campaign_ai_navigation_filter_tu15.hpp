#pragma once
#include "halo2/physics_contact_record.hpp"
namespace halo2 {
// Complete201500: construct retained navigation restrictions from actual
// character, actor action/order and optional vehicle definitions. Bytes outside
// the original semantic fields remain untouched; no path result is supplied.
void native_campaign_ai_navigation_filter_tu15(SourcePhysicsContactMemory &,
    std::uint32_t actor,std::uint32_t destination,
    const SourcePhysicsContactRecordObserver & = {});
}
