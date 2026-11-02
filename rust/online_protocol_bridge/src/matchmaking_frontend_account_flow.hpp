#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct AccountFlowFacts;
struct AccountFlowAccount;
struct AccountFlowEffect;
struct LobbyViewBytes;
class NativeFrontendAccountFlowIo {
public:
  virtual ~NativeFrontendAccountFlowIo()=default;
  virtual AccountFlowFacts account_flow_facts()const=0;
  virtual LobbyViewBytes account_flow_identity()const=0;
  virtual void account_flow_profile(rust::Slice<const std::uint8_t>,rust::Slice<const std::uint8_t>)const=0;
  virtual void account_flow_connect(rust::Slice<const std::uint8_t>)const=0;
  virtual void account_flow_sign_out()const=0;
  virtual AccountFlowAccount account_flow_snapshot()const=0;
  virtual std::uint64_t account_flow_stage_voice(bool)const=0;
  virtual bool account_flow_native_friends()const=0;
  virtual void account_flow_effect(const AccountFlowEffect&)const=0;
};
}
