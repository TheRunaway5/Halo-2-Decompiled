#include "halo2/vista_lobby_update.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#include <utility>

namespace halo2 {
namespace {
template<class F,class... A> decltype(auto) call(const F& f,A&&... a) {
  if(!f)throw FormatError("Missing consumed Vista lobby update service");
  return f(std::forward<A>(a)...);
}
std::uint32_t word(std::span<const std::byte> b,std::size_t at) {
  if(at>b.size() || b.size()-at<4)throw FormatError("Unowned Vista lobby update source field");
  std::uint32_t value{};
  for(unsigned i=0;i<4;++i)value|=std::uint32_t(std::to_integer<unsigned char>(b[at+i]))<<(8*i);
  return value;
}
bool team_variant(const VistaLobbyUpdateVariant& v) {
  if(!v.identity)return false;
  if(v.bytes.size()<=0x48)throw FormatError("Unowned Vista lobby update variant flags");
  return (std::to_integer<unsigned char>(v.bytes[0x48])&1)!=0;
}
std::size_t controller(std::int32_t value) {
  if(value<0 || value>=4)throw FormatError("Unowned Vista lobby update controller slot");
  return std::size_t(value);
}
bool same_name(std::span<const std::byte> a,std::span<const std::byte> b) {
  for(std::size_t i=0;;i+=2) {
    if(i>a.size() || a.size()-i<2 || i>b.size() || b.size()-i<2)
      throw FormatError("Unowned Vista lobby update unterminated identifier name");
    const auto av=std::to_integer<unsigned char>(a[i])|
        (unsigned(std::to_integer<unsigned char>(a[i+1]))<<8);
    const auto bv=std::to_integer<unsigned char>(b[i])|
        (unsigned(std::to_integer<unsigned char>(b[i+1]))<<8);
    if(av!=bv)return false;
    if(av==0)return true;
  }
}
}
void vista_lobby_update(VistaLobbyUpdateState& state,VistaLobbyUpdateGlobals& globals,
    VistaLobbyUpdateScratch scratch,const VistaLobbyUpdateServices& s) {
  const auto session_enum=call(s.session_enum);
  auto variant=call(s.variant);
  scratch.host_word=(scratch.host_word&0xffffff00u)|call(s.host);
  const auto campaign=call(s.campaign);
  if(variant.identity && word(variant.bytes,0x44)==0)variant={};
  if(state.field3486) {
    const auto current=call(s.session_enum);
    if((current!=1 && current!=3 && current!=5) || call(s.retry_map))state.field3486=0;
  }
  if(globals.fieldd78df0==1 && call(s.pending_map_ready)) {
    if(globals.fieldd78ded)call(s.pending_map,globals.fieldd78de8);
    else if(call(s.host)) {
      if(!globals.fieldd78df8)call(s.notification,std::array<std::uint32_t,6>{3,0x113,4,0xffffffffu,0,0});
      else call(s.clear_map);
    }
    globals.fieldd78ded=0;globals.fieldd78de8=0;globals.fieldd78df0=0;
  }
  if(globals.fieldd78df4==1 && call(s.pending_variant_ready)) {
    if(globals.fieldd78dec)call(s.pending_variant,std::span<const std::byte,0x130>(globals.fieldd78cb8));
    else if(call(s.host)) {
      if(!globals.fieldd78df8)call(s.notification,std::array<std::uint32_t,6>{3,0x114,4,0xffffffffu,0,0});
      else call(s.reset_variant);
    }
    globals.fieldd78dec=0;globals.fieldd78cb8.fill(std::byte{});globals.fieldd78df4=0;
  }
  call(s.register_controllers,call(s.controller_mask));
  state.field08=0xffff;
  if(!team_variant(variant) || !call(s.multiplayer)) {
    auto next=call(s.next_controller,-1);
    while(next!=-1) {
      auto& entry=state.field1c6c[controller(next)];
      entry[0]=std::byte{};entry[2]=entry[3]=std::byte{0xff};
      next=call(s.next_controller,next);
    }
  }
  auto next=call(s.next_controller,-1);
  while(next!=-1) {
    scratch.local_player_ids[controller(next)]=call(s.local_player,next);
    next=call(s.next_controller,next);
  }
  std::array<std::uint32_t,16> players;players.fill(0xffffffffu);
  std::int32_t count{};
  for(std::uint32_t id=0;id<players.size();++id)
    if(call(s.occupied,id))players[std::size_t(count++)]=id;
  if(!campaign && team_variant(variant))
    call(s.sort_players,std::span<std::uint32_t>(players.data(),std::size_t(count)),4u,0x642fccu);
  call(s.dialogs);
  call(s.countdown,scratch.host_word);
  if(session_enum!=-1) {
    call(s.pane);call(s.visibility);call(s.chat);
    const VistaLobbyUpdateTuple tuple{session_enum,variant,players,scratch.local_player_ids,count,scratch.host_word};
    call(s.display,VistaLobbyUpdateDisplay::caption,tuple);
    call(s.display,VistaLobbyUpdateDisplay::status,tuple);
    call(s.display,VistaLobbyUpdateDisplay::privacy,tuple);
    if(campaign)call(s.display,VistaLobbyUpdateDisplay::campaign_details,tuple);
    else call(s.details,variant,std::array<std::uint32_t,7>{6,3,4,4,47,0,0});
    call(s.map_bitmap);call(s.help);
    call(s.display,VistaLobbyUpdateDisplay::roster,tuple);
    if(call(s.host) && call(s.state9) && !call(s.channel_in_use,4u,4u))call(s.settings_change,0u);
  }
  VistaLobbyUpdateMap map{scratch.map_first_output,-1,{}};
  state.field3578=0;
  if(call(s.selected_map,map)) {
    if(map.map_id!=state.field3514)state.field3578=1;
    if(map.map_id==-1) {
      // Source dereferences output3+40 before its later NULL check.
      if(!map.identifier || map.identifier->size()<0x60)
        throw FormatError("Vista lobby update selected NONE map lacks its consumed identifier");
      if(!std::equal(map.identifier->begin()+0x40,map.identifier->begin()+0x60,
          state.field3518.begin()+0x40) || !same_name(*map.identifier,state.field3518))state.field3578=1;
    }
    state.field3514=map.map_id;
    if(map.map_id==-1 && map.identifier && !call(s.identifier_provider,call(s.ui_context))) {
      call(s.unavailable_map);
      const auto single=call(s.peer_count)<=1;
      scratch.host_word=(scratch.host_word&0xffffff00u)|std::uint32_t(single);
      call(s.allow_single_peer,scratch.host_word);
      const auto text=call(s.unavailable_map_text);
      call(s.question,std::array<std::uint32_t,9>{3,1,4,0xffffffffu,0x643cf7u,0,0,text,scratch.host_word});
    }
    if(map.identifier) {
      if(map.identifier->size()<state.field3518.size())
        throw FormatError("Unowned Vista lobby update identifier copy extent");
      // Literal forward24-DWORD REP MOVSD, including an aliased source.
      // A generic copy algorithm's nonoverlap precondition is not equivalent.
      for(std::size_t at=0;at<state.field3518.size();at+=4) {
        const auto value=word(*map.identifier,at);
        for(unsigned i=0;i<4;++i)state.field3518[at+i]=std::byte(value>>(i*8));
      }
    } else state.field3518.fill(std::byte{});
  }
  call(s.browser);call(s.teams,variant);
  if(call(s.favorite)) {
    std::array<std::uint32_t,2> identity{};
    const auto context=call(s.favorite_context);
    const auto valid=call(s.identity,identity);
    call(s.screen_a0,valid && call(s.favorite_identity,context,identity)?1u:0u);
  }
  call(s.spinner,std::span<const std::uint32_t,16>(players),count);
  call(s.base_update);
}
} //namespace halo2
