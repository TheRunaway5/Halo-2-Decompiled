#include "halo2/vista_session_readiness.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <initializer_list>
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include <xmmintrin.h>
#endif

namespace halo2 {
namespace {
void call(VistaSessionMapReadinessResult& result,VistaSessionMapReadinessService service,
    std::initializer_list<std::uint32_t> args={}) {
  auto& item=result.calls[result.call_count++];
  item.service=service;item.argument_count=unsigned(args.size());
  unsigned n{};for(auto value:args)item.arguments[n++]=value;
}
std::int32_t progress(std::uint32_t value,std::uint32_t context) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
  if((context&0x1f80)!=0x1f80 || (context&0xffff0000))
    throw FormatError("Original Vista readiness SSE context unsupported");
  const auto saved=_mm_getcsr();_mm_setcsr(context);
  const auto scaled=_mm_mul_ss(_mm_set_ss(std::bit_cast<float>(value)),
      _mm_set_ss(std::bit_cast<float>(std::uint32_t(0x42c80000))));
  const auto converted=_mm_cvttss_si32(scaled);
  _mm_setcsr(saved);
  return converted;
#else
  (void)value;(void)context;
  throw FormatError("Original Vista readiness SSE backend unavailable");
#endif
}
void put(VistaSessionPeerMapPayload& payload,unsigned offset,std::uint32_t word) noexcept {
  for(unsigned i=0;i<4;++i)payload[offset+i]=std::uint8_t(word>>(i*8));
}
}

bool vista_session_map_paths_equal(const VistaSessionMapPath& left,
    const VistaSessionMapPath& right) noexcept {
  // 68B052..68B086: unsigned UTF-16 code units, count=260, first-NUL stop.
  for(unsigned i=0;i<left.size();++i) {
    if(left[i]!=right[i])return false;
    if(left[i]==0)return true;
  }
  return true;
}

VistaSessionMapReadinessResult vista_session_map_readiness(
    VistaSessionMapReadinessState& state,const VistaSessionMapReadinessQueries& query) {
  VistaSessionMapReadinessResult result;
  auto next=state;
  std::int32_t status{};
  std::uint32_t fraction{};
  call(result,VistaSessionMapReadinessService::lifecycle);
  // 5B294D ADD -1 / unsigned JA 3; retain wrapping source admission.
  if(query.lifecycle_slot-1u<=3u) {
    call(result,VistaSessionMapReadinessService::current_session);
    if(query.session && query.session->state!=0) {
      call(result,VistaSessionMapReadinessService::session_admission);
      if(query.session->state>=3 && query.session->state<=8) {
        call(result,VistaSessionMapReadinessService::selection_kind);
        const auto& selected=query.session->selected;
        if(selected.id==-1 || selected.path.front()==0)
          throw FormatError("Original Vista alternate map selection unsupported");
        call(result,VistaSessionMapReadinessService::selected_map);
        next.selected_id=selected.id;
        call(result,VistaSessionMapReadinessService::loaded_world);
        bool loaded{};
        if(query.loaded_world_valid) {
          call(result,VistaSessionMapReadinessService::loaded_options);
          if(!query.loaded_options)
            throw FormatError("Original Vista loaded map metadata unavailable");
          const auto& options=*query.loaded_options;
          if(options.type==selected.type && options.id==selected.id) {
            call(result,VistaSessionMapReadinessService::path_compare,{0x104});
            loaded=vista_session_map_paths_equal(options.path,selected.path);
          }
        }
        if(loaded) {
          status=4;fraction=0x3f800000;next.map_query_flag=0;
        } else {
          call(result,VistaSessionMapReadinessService::map_query);
          call(result,VistaSessionMapReadinessService::path_conversion,{65001,0,0xffffffffu,0x104});
          call(result,VistaSessionMapReadinessService::map_progress,{selected.type==0xffffffffu?1u:0u});
          fraction=query.map_progress_bits;
          next.map_query_flag=std::uint8_t(query.map_query==3);
          if(query.map_query==1)status=2;
          else if(query.map_query==2)status=3;
          else if(query.map_query==3)status=1;
          else call(result,VistaSessionMapReadinessService::map_request,{1});
        }
      }
    }
  }
  next.status=status;
  next.progress=progress(fraction,query.mxcsr);
  state=next;
  return result;
}

void vista_session_map_readiness_payload(VistaSessionPeerMapPayload& payload,
    const VistaSessionMapReadinessState& state) noexcept {
  put(payload,0x60,std::bit_cast<std::uint32_t>(state.status));
  put(payload,0x64,std::bit_cast<std::uint32_t>(state.progress));
}

bool vista_session_publish_local_map_readiness(std::uint32_t session_state,
    std::uint32_t host_peer,std::span<VistaSessionPeerMapRecord> peers,
    std::uint32_t& membership_revision,std::uint32_t& change_revision,
    const VistaSessionPeerMapPayload& payload) {
  if(session_state<5 || session_state>8)
    throw FormatError("Original Vista remote map record delivery unsupported");
  if(host_peer>=peers.size())
    throw FormatError("Original Vista host map record unavailable");
  auto& record=peers[host_peer];
  record.presence[0]=1;
  record.payload=payload;
  ++membership_revision;++change_revision;
  return true;
}
} // namespace halo2
