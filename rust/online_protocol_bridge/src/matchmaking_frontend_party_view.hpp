#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct PartyViewFacts;
struct PartyViewLayout;
struct PartyViewSnapshot;
struct PartyViewEffect;
struct PartyViewMember;
class NativeFrontendPartyViewIo {
public:
  virtual ~NativeFrontendPartyViewIo()=default;
  virtual PartyViewFacts party_view_facts()const=0;
  virtual PartyViewLayout party_view_layout(std::uint32_t,std::uint32_t,bool,bool)const=0;
  virtual rust::Vec<std::uint8_t> party_view_read(bool,std::uint32_t,std::uint32_t)const=0;
  virtual rust::Vec<PartyViewMember> party_view_members()const=0;
  virtual std::int32_t party_view_seconds()const=0;
  virtual bool party_view_matchmaking()const=0;
  virtual rust::Vec<std::uint8_t> party_view_selected()const=0;
  virtual PartyViewSnapshot party_view_snapshot()const=0;
  virtual bool party_view_has_stats()const=0;
  virtual rust::Vec<std::uint8_t> party_view_start_error()const=0;
  virtual rust::Vec<std::uint8_t> party_view_expand(rust::Slice<const std::uint8_t>)const=0;
  virtual void party_view_effect(const PartyViewEffect&)const=0;
  virtual bool party_view_entries_empty()const=0;
};
}
