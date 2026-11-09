#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct PostgameFlowFacts;
class NativeFrontendPostgameIo {
public:
 virtual ~NativeFrontendPostgameIo()=default;
 virtual PostgameFlowFacts postgame_flow_facts()const=0;
 virtual void postgame_flow_effect(std::uint32_t)const=0;
 virtual bool postgame_flow_hold(std::uint32_t)const=0;
 virtual void postgame_flow_continue()const=0;
 virtual bool postgame_flow_request_leave()const=0;
 virtual void postgame_flow_leave_held()const=0;
 virtual void postgame_flow_unavailable(rust::Str)const=0;
};
}
