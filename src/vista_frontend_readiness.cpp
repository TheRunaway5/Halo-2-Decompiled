#include "halo2/vista_frontend_readiness.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <bit>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
using Service=VistaFrontendReadinessService;
void call(VistaFrontendReadinessResult& result,Service service,std::uint32_t argument=0) {
  result.calls[result.call_count++]={service,argument};
}
std::uint32_t progress(std::int32_t input,std::uint32_t mxcsr) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if((mxcsr&0x1f80)!=0x1f80 || (mxcsr&0xffff0000))
    throw FormatError("Original Vista frontend readiness SSE context unsupported");
  const auto clamped=std::clamp(input,0,100);
  const std::uint32_t scale=0x3c23d70a;
  std::uint32_t saved,bits;
  // Keep the environment and consumed instructions in one compiler-visible
  // unit. Intrinsics alone permit an optimizing compiler to move MULSS after
  // restoring MXCSR, which the original directed-rounding controls reject.
  asm volatile("stmxcsr %[saved]\n\t"
      "ldmxcsr %[context]\n\t"
      "cvtsi2ss %[input], %%xmm0\n\t"
      "mulss %[scale], %%xmm0\n\t"
      "movd %%xmm0, %[bits]\n\t"
      "ldmxcsr %[saved]"
      :[saved] "=m"(saved),[bits] "=r"(bits)
      :[context] "m"(mxcsr),[input] "r"(clamped),[scale] "m"(scale)
      :"xmm0","memory");
  return bits;
#else
  (void)input;(void)mxcsr;
  throw FormatError("Original Vista frontend readiness SSE backend unavailable");
#endif
}
std::uint32_t datum_index(std::uint32_t datum) noexcept {return datum&0xffffu;}
}

bool vista_frontend_player_present(std::uint8_t query_al,std::uint32_t mask,
    std::uint32_t datum) noexcept {
  if(datum==0xffffffffu || !query_al)return false;
  const auto index=datum_index(datum);
  return index<16 && (mask&(1u<<index));
}
bool vista_frontend_player_eligible(std::uint8_t query_al,std::uint32_t mask,
    std::uint32_t datum,std::int32_t local_peer,
    std::span<const std::int32_t> peers) {
  const auto index=datum_index(datum);
  // The original leaf uses x86 SHL CL, including its modulo32 mask bit.
  if(!query_al || !(mask&(1u<<(index&31))))return false;
  if(index>=peers.size())throw FormatError("Original Vista player peer record unavailable");
  return peers[index]==local_peer;
}
std::optional<std::int8_t> vista_frontend_player_team(std::uint8_t query_al,
    std::uint32_t mask,std::uint32_t datum,std::span<const std::int8_t> teams) {
  const auto index=datum_index(datum);
  if(!query_al || !(mask&(1u<<(index&31))))return {};
  if(index>=teams.size())throw FormatError("Original Vista player configuration unavailable");
  return teams[index];
}

VistaFrontendReadinessResult vista_frontend_readiness(
    const VistaFrontendReadinessQueries& q,bool progress_output) {
  VistaFrontendReadinessResult out;
  if(progress_output)out.progress_bits=0;
  call(out,Service::lifecycle_context);call(out,Service::lifecycle_active);
  std::int32_t context{};
  if(q.lifecycle_active) {
    call(out,Service::lifecycle_slot);
    // Actual 615520 seven-entry branch table; unsigned out-of-range returns0.
    constexpr std::array<std::int32_t,7> slots{0,2,2,3,5,6,7};
    if(q.lifecycle_slot<slots.size())context=slots[q.lifecycle_slot];
  }
  if(!context)return out;
  call(out,Service::current_session);
  if(!q.current_session)return out;
  if(!q.session_state)throw FormatError("Original Vista current session fields unavailable");
  call(out,Service::session_admission);
  if(*q.session_state<3 || *q.session_state>8)return out;
  if(context!=2) {out.state=1;return out;}
  out.state=10;
  call(out,Service::mode);
  const bool paired=q.mode==0 || q.mode==2 || q.mode==4;
  call(out,Service::map_readiness);
  // Original computes this for every map query, even when output stays zero.
  const auto fraction=progress(q.map_progress,q.mxcsr);
  switch(q.map_status) {
  case 1:
    call(out,Service::local_map_readiness);
    out.state=q.local_map_status==1?7:6;return out;
  case 2:out.state=5;[[fallthrough]];
  case 3:case 4:
    if(progress_output)out.progress_bits=fraction;
    break;
  case 5:out.state=4;return out;
  default:break;
  }
  if(paired) {
    unsigned present{},eligible{};
    for(unsigned i=0;i<q.players.size();++i) {
      call(out,Service::player_present,i);
      if(q.players[i].present) {
        ++present;call(out,Service::player_eligible,i);
        if(q.players[i].eligible)++eligible;
      }
    }
    if(eligible!=2 || present!=eligible) {out.state=8;return out;}
  } else {
    call(out,Service::variant);
    if(q.variant && (q.variant->flags&1)) {
      call(out,Service::map_selection);
      if(!q.map)throw FormatError("Original Vista map selection outputs unavailable");
      unsigned capacity=1;
      const auto engine=q.variant->engine_type;
      if(q.map->custom_map_id==0) {
        call(out,Service::builtin_map_metadata,std::bit_cast<std::uint32_t>(q.map->map_id));
        if(q.builtin_metadata && engine>=1 && engine<=9)
          capacity=(*q.builtin_metadata)[unsigned(engine)];
      } else {
        call(out,Service::custom_metadata_clear,0xc54);
        call(out,Service::custom_map_metadata,q.map->custom_map_id);
        if(q.custom_metadata_loaded) {
          if(!q.custom_metadata)throw FormatError("Original Vista custom map metadata unavailable");
          if(engine>=1 && engine<=9)capacity=(*q.custom_metadata)[unsigned(engine)];
        }
      }
      capacity=std::max(capacity,1u);
      std::uint32_t teams{};
      unsigned count{};
      for(unsigned i=0;i<q.players.size();++i) {
        call(out,Service::player_present,i);
        if(!q.players[i].present)continue;
        call(out,Service::player_configuration,i);
        const auto team=q.players[i].team;
        if(!team || *team<0 || *team>=16)continue;
        const auto bit=1u<<unsigned(*team);
        if(!(teams&bit)) {teams|=bit;++count;}
      }
      if(count<1 || count>capacity) {out.state=8;return out;}
    }
  }
  call(out,Service::countdown);
  if(q.countdown>=0)out.state=11;
  else {
    call(out,Service::host_permission);
    if(q.host_permission)out.state=10;
    else if(out.state==10)out.state=9;
  }
  return out;
}
} // namespace halo2
