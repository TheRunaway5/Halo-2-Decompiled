#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct ClansFlowFacts;struct ClansFlowEffect;struct ClansFlowCommand;struct PartyFlowCommand;
class NativeFrontendClansIo {
public:
 virtual ~NativeFrontendClansIo()=default;
 virtual ClansFlowFacts clans_flow_facts()const=0;
 virtual void clans_flow_effect(const ClansFlowEffect&)const=0;
 virtual bool clans_flow_command(const ClansFlowCommand&)const=0;
 virtual void clans_flow_party_command(const PartyFlowCommand&)const=0;
 virtual void clans_flow_update_account()const=0;
 virtual void clans_flow_select(std::uint32_t)const=0;
 virtual void clans_flow_prepare_pane(std::uint32_t,std::uint32_t)const=0;
 virtual rust::Vec<std::uint8_t> clans_flow_localized(std::uint32_t)const=0;
 virtual rust::Vec<std::uint8_t> clans_flow_localized_direct(std::uint32_t,std::uint32_t,std::uint32_t)const=0;
};
}
