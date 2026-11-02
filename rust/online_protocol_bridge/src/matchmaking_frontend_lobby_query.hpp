#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct LobbyQueryFacts;
struct LobbyQueryMember;
struct LobbyQueryValue;
class NativeFrontendLobbyQueryIo {
public:
  virtual ~NativeFrontendLobbyQueryIo()=default;
  virtual LobbyQueryFacts lobby_query_facts()const=0;
  virtual rust::Vec<std::uint8_t> lobby_query_start_error()const=0;
  virtual std::int32_t lobby_query_seconds()const=0;
  virtual rust::Vec<LobbyQueryMember> lobby_query_members()const=0;
  virtual rust::Vec<std::int32_t> lobby_query_locals()const=0;
  virtual bool lobby_query_has_stats()const=0;
  virtual std::int32_t lobby_query_prior_count()const=0;
  virtual rust::Vec<std::uint8_t> lobby_query_campaign_file(std::uint32_t)const=0;
  virtual std::int32_t lobby_query_profile_option(std::uint32_t,std::uint32_t)const=0;
  virtual void lobby_query_variant()const=0;
  virtual bool lobby_query_defusal()const=0;
  virtual std::int32_t lobby_query_value(std::uint32_t)const=0;
  virtual void lobby_query_appearance()const=0;
  virtual bool lobby_query_custom()const=0;
  virtual std::int32_t lobby_query_capacity(std::uint32_t)const=0;
  virtual void lobby_query_quick(std::uint32_t,rust::Slice<const LobbyQueryValue>,std::uint32_t,std::uint32_t)const=0;
};
}
