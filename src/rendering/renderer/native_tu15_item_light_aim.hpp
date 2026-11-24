#pragma once
// Boundary adapter for the item light's aiming entry, 1D9100
// (native_tu15_item_light_systems.hpp; the shared adapter in
// native_tu15_item_light_common.hpp).
// HALO2_REFERENCE_ITEM_LIGHT_AIM=1 restores the translated body.
// HALO2_VERIFY_ITEM_LIGHT_AIM=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "rendering/renderer/native_tu15_item_light_common.hpp"

namespace halo2::native_item_light_detail {
namespace item_light_aim {
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ITEM_LIGHT_AIM");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ITEM_LIGHT_AIM");return v;}
enum Branch : unsigned {skipped, placed, refreshed, branch_count};
static_assert(unsigned(logic::item_light::Outcome::refreshed)==Branch::refreshed);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Item light aim"};return v;}
// record, flags, items, light, off, x, x_store, y, y_store, z, z_store, w_store, light_again, settled, pending, object, table, function
inline constexpr Sites sites{{0x1d910au,0x1d910du,0x1d9117u,0x1d9124u,0x1d9126u,0x1d912du,0x1d9131u,0x1d9137u,0x1d913cu,0x1d9142u,0x1d9147u,0x1d9150u,0x1d9156u,0x1d9164u,0x1d9169u,0x1d9177u,0x1d917au,0x1d9181u},0x1d9109u,0x1d915cu,0x1d915fu,0x1d9172u,0x1d9180u,0x1d9181u,0x1d9184u,0x1d9100u};
template<class State,class Reference,class Settled,class Refresh,class Route>
bool bridge(State &s,Reference reference,Settled settled,Refresh refresh,Route route) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1d9100u,0x1d9188u,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            struct Entry : Adapter<State,Settled,Refresh,Route> {
                using Adapter<State,Settled,Refresh,Route>::Adapter;
                void run(){this->reached(this->run_entry());}
            };
            return Entry(v,c,t,sites,logic::item_light::aiming,settled,refresh,route);
        });
    summary().hit=coverage().load();
    return true;
}
} // namespace item_light_aim
} // namespace halo2::native_item_light_detail
