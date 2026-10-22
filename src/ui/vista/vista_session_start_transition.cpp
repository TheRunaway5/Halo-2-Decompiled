#include "halo2/vista_session_start_transition.hpp"
#include "halo2/vista_session_player_preparation.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
void validate(std::span<const std::byte> b) {
  if(b.size()<vista_session_source_size)throw FormatError("Vista start transition requires its complete session owner");
}
std::uint32_t get(std::span<const std::byte> b,std::size_t at,unsigned n=4) {
  if(at>b.size() || n>b.size()-at)throw FormatError("Vista start transition addressed outside its session owner");
  std::uint32_t v{};for(unsigned i=0;i<n;++i)v|=std::uint32_t(std::to_integer<unsigned>(b[at+i]))<<(i*8);return v;
}
void put(std::span<std::byte> b,std::size_t at,std::uint32_t v,unsigned n=4) {
  for(unsigned i=0;i<n;++i)b[at+i]=std::byte(v>>(i*8));
}
template<class F> const F& required(const F& f) {
  if(!f)throw FormatError("Missing reached Vista start transition service owner");
  return f;
}
bool authority(std::span<const std::byte> b) {const auto v=get(b,0x73a0);return v>=5 && v<=8;}
bool admitted(std::span<const std::byte> b) {const auto v=get(b,0x73a0);return v>=3 && v<=8;}
bool host(std::span<const std::byte> b) {return admitted(b) && get(b,0x71c0)==get(b,0x74);}
void request(VistaLifecycleManager& m,unsigned kind) {vista_lifecycle_request(m,kind,0,{});}
void increment(std::span<std::byte> b) {put(b,0x4c60,get(b,0x4c60)+1);}
std::int32_t stage(std::span<const std::byte> b) {
  return admitted(b) && get(b,0x4c94,1)?std::bit_cast<std::int32_t>(get(b,0x4c98)):-1;
}
VistaSessionCountdownCommitState countdown(std::span<const std::byte> b) {
  return {std::uint8_t(get(b,0x4c94,1)),std::bit_cast<std::int32_t>(get(b,0x4c98)),
    std::uint8_t(get(b,0x4c9c,1)),get(b,0x4ca0),std::bit_cast<std::int32_t>(get(b,0x4ca4)),
    {get(b,0x4ca8),get(b,0x4cac)},get(b,0x4c60)};
}
bool commit(std::span<std::byte> b,std::uint32_t sender,const VistaSessionCountdownWrite& w) {
  auto c=countdown(b);
  const VistaSessionCommitContext context{get(b,0x73a0),get(b,0x74),get(b,0x71c0),std::uint8_t(get(b,0x4c74,1))};
  const auto changed=vista_session_countdown_commit(c,context,sender,w);
  put(b,0x4c94,c.running,1);put(b,0x4c98,std::bit_cast<std::uint32_t>(c.stage));put(b,0x4c9c,c.flag,1);
  put(b,0x4ca0,c.reason);put(b,0x4ca4,std::bit_cast<std::uint32_t>(c.reason_stage));
  put(b,0x4ca8,c.identity[0]);put(b,0x4cac,c.identity[1]);put(b,0x4c60,c.revision);return changed;
}
void timer(std::span<std::byte> b,VistaSessionCountdownTaskState& task,const VistaSessionStartTransitionServices& s) {
  const auto value=stage(b);
  const auto flag=std::uint8_t(admitted(b) && get(b,0x4c94,1)?get(b,0x4c9c,1):0);
  if(value!=task.last_stage) {task.last_clock=required(s.transition.phase.network_clock)();task.last_stage=value;}
  if(value<0)return;
  const auto elapsed=required(s.transition.phase.network_clock)()-task.last_clock;
  if(std::bit_cast<std::int32_t>(elapsed)<1000 || value<=0)return;
  task.last_clock=required(s.transition.phase.network_clock)();task.last_stage=value-1;
  vista_session_countdown_write(b,{1,value-1,flag,0,{}},s);
}
void zero(std::span<std::byte> b,const VistaSessionStartTransitionServices& s) {
  if(stage(b)!=0)return;
  if((admitted(b)?get(b,0x4c90):0)>1)return;
  const auto count=std::bit_cast<std::int32_t>(get(b,0x84));bool ready=true;
  for(std::int32_t i=0;i<count;++i) {
    const auto value=get(b,0x110+std::size_t(i)*0x10c);
    if(value==0 || value==1 || value==2 || value==5)ready=false;
  }
  if(ready)vista_session_phase_write(b,2,s.transition.phase);
  else vista_session_countdown_write(b,{1,1,0,2,{}},s);
}
bool refresh(std::span<std::byte> b,std::uint8_t flag,const VistaSessionStartTransitionServices& s) {
  if(get(b,0x73a0)!=5)return false;
  if(std::bit_cast<std::int32_t>(get(b,0x84))<=1)return true;
  // Original5C4F33 samples before rereading the count/host for its mask.
  const auto clock=required(s.transition.phase.network_clock)();
  const auto mask=((1u<<(get(b,0x84)&31))-1u)&~(1u<<(get(b,0x71c0)&31));
  std::fill_n(b.begin()+0x73a4,0x58c,std::byte{});
  put(b,0x73a8,mask);put(b,0x73ac,clock);put(b,0x73b0,0xffffffffu);put(b,0x73b4,flag,1);
  put(b,0x73a0,7);return true;
}
bool gate(std::span<std::byte> b,VistaSessionLoadingHandlerState& task,const VistaSessionStartTransitionServices& s) {
  if(!authority(b)){task.flag10=0;return false;}
  const auto busy=required(s.network_busy)();
  if(get(b,0x4c74,1) || !busy)return true;
  if(get(b,0x73a0)==7 && !get(b,0x73a4,1))return false;
  if(get(b,0x73a0)!=5)return false;
  if(task.flag10)return true;
  const auto previous=task.flag11;task.flag11=0;task.flag10=1;
  return !refresh(b,previous,s);
}
}
bool vista_session_countdown_write(std::span<std::byte> source,const VistaSessionCountdownWrite& w,
    const VistaSessionStartTransitionServices& s) {
  validate(source);auto b=source.first(vista_session_source_size);if(!admitted(b))return false;
  if(authority(b)){commit(b,get(b,0x71c0),w);return true;}
  VistaSessionCountdownMessage message{get(b,8),get(b,0x14),0,{}};
  put(message.payload,0,get(b,0x20));put(message.payload,4,get(b,0x24));put(message.payload,8,w.running,1);
  put(message.payload,0xc,std::bit_cast<std::uint32_t>(w.stage));put(message.payload,0x10,w.reason);
  if(w.reason==1) {
    if(!w.identity)throw FormatError("Missing consumed remote Vista countdown identity");
    put(message.payload,0x14,(*w.identity)[0]);put(message.payload,0x18,(*w.identity)[1]);
  }
  const auto at=std::uint64_t(0x71c4)+std::uint64_t(get(b,0x64))*0x1c;
  if(at+8>b.size())throw FormatError("Vista countdown association exceeds its retained owner");
  if(get(b,std::size_t(at)+1,1)) {
    message.channel=get(b,std::size_t(at)+4);required(s.countdown_send)(message);
  }return true;
}
bool vista_session_countdown_clear(std::span<std::byte> source) {
  validate(source);auto b=source.first(vista_session_source_size);if(!authority(b))return false;
  commit(b,get(b,0x74),{0,0,0,0,{}});return true;
}
std::uint8_t vista_session_null_update(VistaLifecycleManager& m,std::span<std::byte> b,const VistaSessionStartTransitionServices& s) {
  validate(b);switch(get(b,0x73a0)) {
  case 0:required(s.null_network_close)(1,0);required(s.null_network_close)(2,0);break;
  case 2:case 4:case 6:break;
  case 7:case 8:if(get(b,0x73a4,1))break;[[fallthrough]];
  default:request(m,1);
  }return 0;
}
void vista_session_pregame_enter(VistaSessionStartTransitionState& task,const VistaSessionStartTransitionServices& s) {
  task.countdown.last_stage=-1;task.countdown.last_clock=0;task.pregame_word18=0;
  task.pregame_bank.fill(std::byte{});required(s.pregame_external)(4);
}
std::uint8_t vista_session_pregame_update(VistaLifecycleManager& m,std::span<std::byte> b,
    VistaSessionStartTransitionState& task,const VistaSessionStartTransitionServices& s) {
  validate(b);const auto repeat=vista_session_transition_preflight(m,b,s.transition);if(repeat || m.pending)return repeat;
  switch(get(b,0x4c68)) {
  case 0:if(get(b,0x73a0)!=1)request(m,0);break;
  case 1:if(host(b))timer(b,task.countdown,s);if(authority(b))zero(b,s);break;
  case 2:case 3:case 4:
    if(admitted(b)) {
      const auto status=get(b,0x110+std::size_t(get(b,0x71c0))*0x10c);
      if(status==3 || status==4)request(m,2);
    }break;
  case 5:request(m,4);break;
  case 6:case 7:case 8:case 9:request(m,5);break;
  default:break;
  }return repeat;
}
void vista_session_start_game_enter(std::span<std::byte> b,VistaSessionStartTransitionState& task,
    const VistaSessionStartTransitionServices& s) {
  validate(b);if(authority(b) && get(b,0x4c68)==2) {
    const auto seed=required(s.seed)();const auto identity=required(s.random_identity)();
    if(authority(b)){put(b,0x4dc8,identity[0]);increment(b);put(b,0x4dcc,identity[1]);}
    if(authority(b)){put(b,0x4dd0,seed);increment(b);}
    if(authority(b)){put(b,0x4cb0,0,1);increment(b);}
    if(authority(b)) {
      put(b,0x5101,0,1);std::fill_n(b.begin()+0x5104,0x74,std::byte{});
      std::fill_n(b.begin()+0x5178,0xd40,std::byte{});increment(b);
    }
  }task.loading={};
}
std::uint8_t vista_session_start_game_update(VistaLifecycleManager& m,std::span<std::byte> b,
    VistaSessionStartTransitionState& task,const VistaSessionStartTransitionServices& s) {
  validate(b);const auto repeat=vista_session_transition_preflight(m,b,s.transition);
  if(repeat || m.pending || !admitted(b))return repeat;
  const auto phase=get(b,0x4c68);
  if(phase==4){request(m,3);return repeat;}
  if(phase!=2){request(m,1);return repeat;}
  if(!authority(b) || !vista_session_prepare_players(b))return repeat;
  if(!gate(b,task.loading,s))return repeat;
  vista_session_countdown_clear(b);vista_session_phase_write(b,4,s.transition.phase);return 1;
}
void vista_session_bind_start_handlers(std::span<VistaLifecycleHandler> h,std::span<std::byte> b,
    VistaSessionStartTransitionState& task,const VistaSessionStartTransitionServices& s) {
  validate(b);if(h.size()<7)throw FormatError("Vista start transition requires all seven source handlers");
  h[0].update=[b,&s](VistaLifecycleManager& m){return vista_session_null_update(m,b,s);};
  h[0].enter=[](VistaLifecycleManager&,std::uint32_t,std::int32_t,std::span<const std::byte>){};
  h[0].exit=[](VistaLifecycleManager&,std::uint32_t){};
  h[1].update=[b,&task,&s](VistaLifecycleManager& m){return vista_session_pregame_update(m,b,task,s);};
  h[1].enter=[&task,&s](VistaLifecycleManager&,std::uint32_t,std::int32_t,std::span<const std::byte>){vista_session_pregame_enter(task,s);};
  h[1].exit=[](VistaLifecycleManager&,std::uint32_t){};
  h[2].update=[b,&task,&s](VistaLifecycleManager& m){return vista_session_start_game_update(m,b,task,s);};
  h[2].enter=[b,&task,&s](VistaLifecycleManager&,std::uint32_t,std::int32_t,std::span<const std::byte>){vista_session_start_game_enter(b,task,s);};
  h[2].exit=[](VistaLifecycleManager&,std::uint32_t){};
}
} // namespace halo2
