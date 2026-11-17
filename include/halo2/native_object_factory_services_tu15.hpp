#pragma once
#include "halo2/native_campaign_script_builtins_tu15.hpp"
#include "halo2/native_object_factory_birth_tu15.hpp"

// Track "services": ObjectFactoryServices bound to every native factory
// track's owners. Owners no track has ported yet stay named refusals. Build
// with all factory tracks (the default, unfiltered build).
namespace halo2::native_object_factory_tu15 {
ObjectFactoryServices native_object_factory_services(std::uint16_t x87_control,std::uint32_t mxcsr);

// Script host evaluator tails over the unitactions track's vehicle exit
// cores. Reads come from the bank's memory; stores go through the bank (its
// journaled writer when it has one); the FP controls are the bank's.
// 2ADE60 vehicle_unload(object, unit_seat_mapping): 11D990 (ECX mapping, EDX
// vehicle); returns AX, the riders that started action 0x1D.
std::uint16_t script_vehicle_unload(NativeCampaignScriptBank&,std::uint32_t vehicle,std::uint32_t mapping);
// 2ADC30 unit_exit_vehicle(unit, short): 11D670 (EAX unit, DX mode).
void script_unit_exit_vehicle(NativeCampaignScriptBank&,std::uint32_t unit,std::uint16_t mode);
// 2AFE00 ai_vehicle_exit(ai): 27EAB0 (ECX reference, EBX NONE mapping) over
// the 27CAA0/27CB30 actor walk (native_campaign_script_tu15::ai_reference_actors).
void script_ai_vehicle_exit(NativeCampaignScriptBank&,std::uint32_t reference);
}
