#include "halo2/vista_lifecycle.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <cstring>
#include <initializer_list>

namespace halo2 {
void vista_lifecycle_request(VistaLifecycleManager& m,std::uint32_t kind,
    std::int32_t length,std::span<const std::byte> source) {
  if(m.payload.size()<4 || (length>0 &&
      (std::size_t(length)>m.payload.size() || std::size_t(length)>source.size())))
    throw FormatError("Vista lifecycle request exceeds caller-owned payload storage");
  m.requested_kind=kind;m.pending=1;m.payload_length=length;
  std::fill_n(m.payload.begin(),4,std::byte{});
  if(length>0)std::memcpy(m.payload.data(),source.data(),std::size_t(length));
}
void vista_lifecycle_pump(VistaLifecycleManager& m,
    std::span<const VistaLifecycleHandler> handlers,
    const std::function<void(VistaLifecycleManager&)>& network_update) {
  auto handler=[&](std::uint32_t kind)->const VistaLifecycleHandler& {
    if(kind>=handlers.size())throw FormatError("Vista lifecycle handler is absent");
    return handlers[kind];
  };
  if(!network_update)throw FormatError("Vista lifecycle network-update owner is absent");
  network_update(m);
  for(;;) {
    if(m.pending) {
      const auto length=m.payload_length;
      if(length>0 && std::size_t(length)>m.payload.size())
        throw FormatError("Vista lifecycle pending payload exceeds owner storage");
      const auto payload=length>0
          ? std::span<const std::byte>(m.payload.data(),std::size_t(length))
          : std::span<const std::byte>{};
      m.pending=0;
      const auto requested=m.requested_kind;
      m.transitioning=1;
      if(m.current_kind!=requested) {
        const auto old=m.current_kind;
        const auto& from=handler(old);
        const auto& to=handler(requested);
        if(!from.exit || !to.enter)
          throw FormatError("Vista lifecycle transition callback is absent");
        from.exit(m,requested);
        m.current_kind=requested;
        to.enter(m,old,length,payload);
      }
      m.transitioning=0;
      continue;
    }
    const auto& current=handler(m.current_kind);
    if(!current.update)throw FormatError("Vista lifecycle update callback is absent");
    m.updating=1;
    const auto repeat=current.update(m);
    m.updating=0;
    if(!repeat && !m.pending)return;
  }
}
namespace {
void call(VistaLifecycleUpdate& r,VistaLifecycleService service,
    std::initializer_list<std::uint32_t> arguments={}) {
  auto& out=r.calls[r.call_count++];out.service=service;
  out.argument_count=unsigned(arguments.size());
  std::copy(arguments.begin(),arguments.end(),out.arguments.begin());
}
}
VistaLifecycleUpdate vista_lifecycle_in_game_update(const VistaLifecycleInGameQueries& q) {
  VistaLifecycleUpdate r;
  call(r,VistaLifecycleService::preflight);r.repeat=q.preflight;
  if(q.preflight || q.pending_after_preflight)return r;
  call(r,VistaLifecycleService::admitted);
  if(!q.admitted)return r;
  if(q.phase!=4) {
    call(r,VistaLifecycleService::request,{q.phase==5?4u:1u,0,0});
    return r;
  }
  call(r,VistaLifecycleService::closing);
  call(r,VistaLifecycleService::authority);
  if(q.closing) {
    call(r,q.authority?VistaLifecycleService::phase_write:VistaLifecycleService::remote_closing,
        {q.authority?1u:0u});
    r.repeat=1;return r;
  }
  if(q.authority) {
    call(r,VistaLifecycleService::game_over_complete);
    if(q.game_over_complete) {
      call(r,VistaLifecycleService::phase_write,{5});r.repeat=1;
    } else {
      call(r,VistaLifecycleService::force_end_query);
      if(q.force_end)call(r,VistaLifecycleService::force_end);
    }
  }
  call(r,VistaLifecycleService::world_ready);
  if(!q.world_ready)return r;
  call(r,VistaLifecycleService::world_identity);
  if(q.world_identity!=q.retained_identity)return r;
  call(r,VistaLifecycleService::selected_map);
  if(!q.selected_map_valid)return r;
  call(r,VistaLifecycleService::world_map);
  if(q.world_map!=q.selected_map)call(r,VistaLifecycleService::map_change);
  return r;
}
VistaLifecycleUpdate vista_lifecycle_post_game_update(const VistaLifecyclePostGameQueries& q) {
  VistaLifecycleUpdate r;
  call(r,VistaLifecycleService::preflight);r.repeat=q.preflight;
  if(q.preflight || q.pending_after_preflight || q.phase==5)return r;
  call(r,VistaLifecycleService::request,{1,0,0});r.repeat=1;
  return r;
}
VistaLifecycleUpdate vista_lifecycle_post_game_enter(std::uint32_t old_kind) {
  VistaLifecycleUpdate r;
  call(r,VistaLifecycleService::postgame_external,{5});
  call(r,VistaLifecycleService::base_enter,{old_kind,0,0});
  return r;
}
} // namespace halo2
