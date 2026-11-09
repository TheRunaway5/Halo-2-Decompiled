#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct CouchFlowFacts;struct CouchFlowEffect;struct CouchFlowSnapshot;struct CouchFlowFrame;
class NativeFrontendCouchIo {
public:
 virtual ~NativeFrontendCouchIo()=default;
 virtual CouchFlowFacts couch_flow_facts()const=0;
 virtual void couch_flow_effect(const CouchFlowEffect&)const=0;
 virtual bool couch_flow_hold(std::uint32_t)const=0;
 virtual void couch_flow_release()const=0;
 virtual CouchFlowSnapshot couch_flow_snapshot(std::uint32_t,std::uint32_t)const=0;
 virtual bool couch_flow_authorized(std::uint32_t,std::uint32_t)const=0;
 virtual bool couch_flow_steam(std::uint32_t,std::uint32_t)const=0;
 virtual void couch_flow_cancel(std::uint32_t,std::uint32_t)const=0;
 virtual bool couch_flow_identity()const=0;
 virtual rust::Vec<std::uint8_t>couch_flow_description()const=0;
 virtual CouchFlowFrame couch_flow_build()const=0;
 virtual rust::Vec<std::uint32_t>couch_flow_players()const=0;
};
}
