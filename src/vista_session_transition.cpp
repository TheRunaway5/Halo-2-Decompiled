#include "halo2/vista_session_transition.hpp"
#include "halo2/cache.hpp"

namespace halo2 {
namespace {
std::uint32_t read(std::span<const std::byte> b,std::size_t at,unsigned n=4) {
  if(b.size()<vista_session_source_size)throw FormatError("Truncated Vista transition session owner");
  std::uint32_t v{};
  for(unsigned i=0;i<n;++i)v|=std::uint32_t(std::to_integer<unsigned char>(b[at+i]))<<(8*i);
  return v;
}
template<class F> const F& required(const F& f) {
  if(!f)throw FormatError("Missing reached Vista transition service owner");
  return f;
}
bool admitted(std::span<const std::byte> b) {
  const auto state=read(b,0x73a0);return state>=3 && state<=8;
}
bool authority(std::span<const std::byte> b) {
  const auto state=read(b,0x73a0);return state>=5 && state<=8;
}
// Actual5AD457 table: states2/4/6 return1,7/8 return raw transition byte.
std::uint8_t transitioning(std::span<const std::byte> b) {
  switch(read(b,0x73a0)) {
  case 2:case 4:case 6:return 1;
  case 7:case 8:return std::uint8_t(read(b,0x73a4,1));
  default:return 0;
  }
}
void request(VistaLifecycleManager& m,unsigned kind) {
  vista_lifecycle_request(m,kind,0,{});
}
}
std::uint8_t vista_session_transition_preflight(VistaLifecycleManager& m,
    std::span<std::byte> b,const VistaSessionTransitionServices& s) {
  if(read(b,0x73a0)==10) {
    if(required(s.termination_reason)()==1)required(s.notify_termination)(1);
    required(s.remote_close)(0);
  }
  // The actual3/4 constructors pass literal1 for handler+0C.
  if(read(b,0x73a0)==0 || transitioning(b))request(m,0);
  return 0; // Literal5D76C1 XOR AL,AL, independent of requests above.
}
std::uint8_t vista_session_in_game_update(VistaLifecycleManager& m,
    std::span<std::byte> b,VistaSessionTransitionState& state,const VistaSessionTransitionServices& s) {
  auto repeat=vista_session_transition_preflight(m,b,s);
  if(repeat || m.pending || !admitted(b))return repeat;
  const auto phase=read(b,0x4c68);
  if(phase!=4){request(m,phase==5?4:1);return repeat;}
  const auto flags=required(s.closing_flags)();
  if(flags[0] && flags[1]) {
    if(authority(b))vista_session_phase_write(b,1,s.phase);
    else required(s.remote_close)(0);
    return 1;
  }
  if(authority(b)) {
    if(required(s.game_over_complete)()) {
      vista_session_phase_write(b,5,s.phase);repeat=1;
    } else if(admitted(b) && read(b,0x4cb0,1))required(s.force_end)();
  }
  if(!required(s.world_ready)())return repeat;
  if(required(s.world_options)().identity!=state.world_identity)return repeat;
  // Actual5C23A7 initializes outputs to NONE before its admission check.
  if(!admitted(b) || read(b,0x4dd8)==0xffffffffu || read(b,0x4cbc,2)==0)return repeat;
  const std::array<std::uint32_t,2> selected{read(b,0x4cb8),read(b,0x4dd8)};
  if(required(s.world_options)().map!=selected)required(s.map_change)(state);
  return repeat;
}
std::uint8_t vista_session_post_game_update(VistaLifecycleManager& m,
    std::span<std::byte> b,const VistaSessionTransitionServices& s) {
  const auto repeat=vista_session_transition_preflight(m,b,s);
  if(repeat || m.pending || read(b,0x4c68)==5)return repeat;
  request(m,1);return 1;
}
void vista_session_in_game_enter(VistaSessionTransitionState& state,const VistaSessionTransitionServices& s) {
  required(s.map_change)(state); //5D763E base enter is a literal no-op.
}
void vista_session_in_game_exit(VistaSessionTransitionState& state,const VistaSessionTransitionServices& s) {
  //5D7641 base exit and this image's404541 are literal no-ops/false.
  if(required(s.world_ready)() && required(s.world_options)().identity==state.world_identity)
    required(s.network_close)();
}
void vista_session_post_game_enter(const VistaSessionTransitionServices& s) {
  required(s.postgame_external)(5);
}
void vista_session_bind_transition_handlers(std::span<VistaLifecycleHandler> h,
    std::span<std::byte> b,VistaSessionTransitionState& state,const VistaSessionTransitionServices& s) {
  if(h.size()<7 || b.size()<vista_session_source_size)
    throw FormatError("Vista transition requires its seven-handler table and complete session");
  h[3].update=[b,&state,&s](VistaLifecycleManager& m){return vista_session_in_game_update(m,b,state,s);};
  h[3].enter=[&state,&s](VistaLifecycleManager&,std::uint32_t,std::int32_t,std::span<const std::byte>){vista_session_in_game_enter(state,s);};
  h[3].exit=[&state,&s](VistaLifecycleManager&,std::uint32_t){vista_session_in_game_exit(state,s);};
  h[4].update=[b,&s](VistaLifecycleManager& m){return vista_session_post_game_update(m,b,s);};
  h[4].enter=[&s](VistaLifecycleManager&,std::uint32_t,std::int32_t,std::span<const std::byte>){vista_session_post_game_enter(s);};
  h[4].exit=[](VistaLifecycleManager&,std::uint32_t){};
}
} // namespace halo2
