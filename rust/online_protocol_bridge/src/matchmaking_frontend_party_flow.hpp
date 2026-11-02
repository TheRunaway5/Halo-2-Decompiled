#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct PartyFlowFacts;
struct PartyMemberData;
struct PartyFlowCommand;
struct PartyFlowUi;
class NativeFrontendPartyFlowIo {
public:
  virtual ~NativeFrontendPartyFlowIo()=default;
  virtual PartyFlowFacts party_flow_facts()const=0;
  virtual rust::Vec<PartyMemberData> party_flow_members()const=0;
  virtual bool party_flow_command(const PartyFlowCommand&)const=0;
  virtual std::uint64_t party_flow_serial()const=0;
  virtual void party_flow_update()const=0;
  virtual void party_flow_cancel()const=0;
  virtual bool party_flow_lobby(std::int32_t)const=0;
  virtual void party_flow_ui(const PartyFlowUi&)const=0;
};
}
