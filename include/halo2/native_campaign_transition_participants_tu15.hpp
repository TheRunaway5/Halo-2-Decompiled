#pragma once
#include "halo2/native_campaign_transition_request_tu15.hpp"
#include "halo2/native_tu15_physics_frame.hpp"

namespace halo2 {
//154F80: capture the actual players, their identities and configuration. The
// retained form preserves forward-copy overlap and lazy source reads. The
// value form is for191750's authenticated disjoint private1118-byte options.
// Both routines use the ordinary forward-direction ABI over retained RAM;
// no reverse-string/device-memory or guest private-register adapter is implied.
void native_campaign_transition_capture_participants_tu15(
    SourcePhysicsContactMemory &,std::uint32_t retained_options);
void native_campaign_transition_capture_participants_value_tu15(
    SourcePhysicsContactMemory &,NativeCampaignTransitionOptionsTu15 &);
//155190: merge the retained baseline and actual current-unit equipment into
// matching option records. No participants or identities are synthesized.
// FP is the actual caller context, with masked exceptions and one free x87
// slot. Dense occupied older values are untouched; no fresh HS FP is assumed.
// Ambient CW/SW/TAG, all80 value bytes and MXCSR are restored. Host x87
// instruction/data-pointer metadata is outside this preservation contract.
void native_campaign_transition_prepare_participants_tu15(
    SourcePhysicsContactMemory &,std::uint32_t retained_options,
    NativeTu15PhysicsFrameFp &,unsigned occupied_x87);
void native_campaign_transition_prepare_participants_value_tu15(
    SourcePhysicsContactMemory &,NativeCampaignTransitionOptionsTu15 &,
    NativeTu15PhysicsFrameFp &,unsigned occupied_x87);
// Actual call-free children CE340 and1550B0, also useful to native callers.
std::uint32_t native_campaign_transition_spare_equipment_tu15(
    const SourcePhysicsContactMemory &,std::uint32_t unit);
void native_campaign_transition_equipment_tu15(
    SourcePhysicsContactMemory &,std::uint32_t item,std::uint32_t destination,
    NativeTu15PhysicsFrameFp &,unsigned occupied_x87);
}
