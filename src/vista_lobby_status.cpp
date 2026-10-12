#include "halo2/vista_lobby_status.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <utility>

namespace halo2 {
namespace {
template<class F,class...A> decltype(auto) call(const F& f,A&&...a) {
  if(!f)throw FormatError("Missing consumed Vista lobby status service");
  return f(std::forward<A>(a)...);
}
std::uint32_t word32(const VistaLobbyStatusRecord& r,unsigned n) {
  std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::uint32_t(r.bytes[n+i])<<(8*i);return v;
}
std::uint16_t word16(const VistaLobbyStatusRecord& r,unsigned n) {
  return std::uint16_t(r.bytes[n]|(unsigned(r.bytes[n+1])<<8));
}
std::int32_t find_peer(const VistaLobbyStatusPeers& p,const std::uint8_t* id) {
  for(unsigned i=0;i<16;++i)
    if((p.mask&(1u<<i)) && std::equal(p.identities[i].begin(),p.identities[i].end(),id))return std::int32_t(i);
  return -1;
}
VistaLobbyStatusText& required(VistaLobbyStatusText* p) {
  if(!p)throw FormatError("Vista lobby status requires a constructed source text widget");
  return *p;
}
//643BE8's actual byte/jump tables. Invalid values clear the bit.
bool style(std::int32_t code) {
  constexpr std::array<bool,25> values{true,false,false,false,true,true,true,false,false,false,
    true,true,true,false,true,true,true,true,true,true,true,true,false,false,true};
  const auto i=std::uint32_t(code)-2;return i<values.size() && values[i];
}
}
bool vista_lobby_status_tuple(VistaLobbyStatusTuple& out,const VistaLobbyStatusQueryServices& s) {
  const auto source=call(s.record);
  if(!source.available || !source.record)return false;
  const auto& r=*source.record;
  out.field04=std::int32_t(word32(r,4));out.peer08=-1;
  if(out.field04==4)out.peer08=find_peer(call(s.peers),r.bytes.data()+8);
  out.field08=out.field04==3?r.bytes.data()+8:nullptr;
  out.field48=out.field04==3?r.bytes.data()+0x48:nullptr;
  out.field88=std::int32_t(word32(r,0x88));out.peer8c=find_peer(call(s.peers),r.bytes.data()+0x8c);
  out.field94=word16(r,0x94);out.field96=word16(r,0x96);out.field98=word16(r,0x98);out.field9c=word32(r,0x9c);
  return true;
}
bool vista_lobby_status_session_tuple(std::span<const std::byte> bank,std::uint8_t enabled,
    VistaLobbyStatusRecord& retained,VistaLobbyStatusTuple& out) {
  VistaLobbyStatusQueryServices s;
  s.record=[&] {
    if(!enabled)return VistaLobbyStatusRecordSource{};
    if(bank.size()<0x73a4)throw FormatError("Vista lobby status requires its actual complete session owner");
    const bool occupied=std::any_of(bank.begin()+0x73a0,bank.begin()+0x73a4,[](auto v){return v!=std::byte{};});
    if(!occupied)return VistaLobbyStatusRecordSource{};
    for(unsigned i=0;i<retained.bytes.size();++i)retained.bytes[i]=std::to_integer<std::uint8_t>(bank[0x4ae0+i]);
    return VistaLobbyStatusRecordSource{1,&retained};
  };
  s.peers=[&] {
    VistaLobbyStatusPeers p;
    for(unsigned i=0;i<4;++i)p.mask|=std::to_integer<std::uint32_t>(bank[0x1258+i])<<(8*i);
    for(unsigned i=0;i<16;++i)for(unsigned j=0;j<8;++j)
      p.identities[i][j]=std::to_integer<std::uint8_t>(bank[0x125c+i*0x128+j]);
    return p;
  };
  return vista_lobby_status_tuple(out,s);
}
void vista_lobby_button_key(std::optional<std::uint8_t> variant,std::uint8_t host,const VistaLobbyStatusServices& s) {
  if(auto* t=call(s.text,std::int16_t{1})) {
    call(s.clear_string,*t);
    std::uint32_t sid;
    if(call(s.countdown_active))sid=host?0x2700021c:0x2000021b;
    else {
      if(call(s.favorite)) {
        std::array<std::uint32_t,2> id{};
        if(call(s.identity,id))call(s.set_string,*t,call(s.favorite_identity,id)?0x11000904u:0x0e000903u);
      }
      const bool teams=variant && (*variant&1) && call(s.multiplayer);
      sid=host?(teams?0x25000218:0x26000217):(teams?0x1d00021a:0x1e000219);
    }
    call(s.append_string,*t,sid);
  }
  call(s.refresh_button_key);
}
void vista_lobby_status(VistaLobbyStatusState& state,std::uint8_t host,std::uint32_t seed,const VistaLobbyStatusServices& s) {
  auto* status=call(s.text,state.pane>=0 && state.pane<=2?std::int16_t{2}:std::int16_t{-1});
  auto* t52=call(s.text,std::int16_t{52});auto* t53=call(s.text,std::int16_t{53});auto* t54=call(s.text,std::int16_t{54});
  VistaLobbyStatusTuple tuple{};
  if(status) {
    if(!call(s.query,tuple))return;
    if(!tuple.field04) {
      if(!tuple.field88)state.previous_kind=0;
      const auto code=call(s.readiness);
      status->visible=1;required(t52).visible=0;required(t53).visible=0;required(t54).visible=0;
      if(call(s.reset_dialog))state.dialog_shown=0;
      std::uint32_t sid{};bool raw=false;std::optional<std::u16string> value;
      if(code==8) {
        if(call(s.campaign))sid=0x0e0001ed;
        else {const auto n=call(s.map_team_limit);sid=n>=3 && n<=8?0x0c0001e7u+std::uint32_t(n-3):0x0c0001e6u;}
      } else if(code==9) {
        sid=call(s.state9)?0x170001efu:(call(s.team_change)?0x100001eeu:0x0c00097bu);
      } else if(code==11) {
        const auto user=call(s.user);
        if(user!=-1)sid=call(s.user_ready,user)?0x1200020eu:0x1b00020du;
        else if(call(s.state2))sid=call(s.campaign)?0x1500015du:0x0a000209u;
        else sid=host?0x200001f1u:0x2b00020cu;
      } else {
        if(code==7)call(s.status7_dialog,state);
        sid=call(s.status_string,code);
        const bool custom_fallback=sid==0x0f0001fd;
        std::uint8_t custom{};
        if(custom_fallback)custom=call(s.custom_map,seed);
        if(code==10) {
          const auto count=call(s.count);const auto max=call(s.maximum);
          if(count>max)sid=call(s.overflow_string);
        }
        //61553C is the actual two-instruction false return. Thus only the
        //fallback(false,custom) branch can execute for this supplied PE.
        if(custom_fallback) {value=call(s.custom_text,std::uint8_t{},custom);raw=value.has_value();}
      }
      if(raw)call(s.set_raw,*status,*value);else call(s.set_string,*status,sid);
      if(style(code))status->buffer_flags|=1;else status->buffer_flags&=std::uint16_t(~1u);
    } else {
      required(status).visible=0;
      constexpr std::array<std::uint32_t,5> ids{0,0x1a000943,0x1d000942,0x1b000945,0x1a000944};
      const auto kind=std::uint32_t(tuple.field04);
      call(s.set_string,required(t52),kind<ids.size()?ids[kind]:0u);
      t52->visible=1;call(s.set_string,required(t53),0x0900093eu);t53->visible=1;
      state.previous_kind=tuple.field04;call(s.set_string,required(t54),0x13000941u);t54->visible=1;
    }
  }
  if(tuple.field88) {
    std::uint32_t sid{};
    if(tuple.field88==3)sid=0x1400094f;
    else if(std::uint32_t(state.previous_kind)<=4) {
      constexpr std::array<std::uint32_t,5> ids{0x21000946,0x1000096c,0x1300096e,0x18000970,0x1000096a};
      if(!state.previous_kind)sid=ids[0];
      else if(tuple.field88==1 || tuple.field88==2)sid=ids[unsigned(state.previous_kind)]+std::uint32_t(tuple.field88-1);
    }
    call(s.set_string,required(t54),sid);t54->visible=1;
  }
  if(call(s.team_change)) {
    auto& t=required(call(s.text,std::int16_t{55}));call(s.set_string,t,0x1300097cu);call(s.screen_a0,std::int32_t{1});
    if(call(s.factory_flag)) {t.bounds[0]=400;t.bounds[2]=320;}
  } else call(s.set_string,required(call(s.text,std::int16_t{45})),0x0c00097bu);
}
} //namespace halo2
