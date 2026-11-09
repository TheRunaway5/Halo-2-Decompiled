#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct SigninFlowFacts;struct SigninFlowEffect;struct SigninFlowSnapshot;struct SigninFlowFrame;
class NativeFrontendSigninIo {
public:
 virtual ~NativeFrontendSigninIo()=default;
 virtual SigninFlowFacts signin_flow_facts()const=0;
 virtual void signin_flow_effect(const SigninFlowEffect&)const=0;
 virtual rust::Vec<std::uint8_t> signin_flow_random()const=0;
 virtual bool signin_flow_steam()const=0;
 virtual bool signin_flow_authorized()const=0;
 virtual SigninFlowSnapshot signin_flow_snapshot()const=0;
 virtual bool signin_flow_identity()const=0;
 virtual rust::Vec<std::uint8_t> signin_flow_profile_name()const=0;
 virtual SigninFlowFrame signin_flow_build(std::uint32_t,std::uint32_t)const=0;
};
}
