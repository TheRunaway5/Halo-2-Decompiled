#pragma once
// Boundary adapter for the item light's placing entry, 1D9080
// (native_tu15_item_light_systems.hpp; the shared adapter in
// native_tu15_item_light_common.hpp).
// HALO2_REFERENCE_ITEM_LIGHT_PLACE=1 restores the translated body.
// HALO2_VERIFY_ITEM_LIGHT_PLACE=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "rendering/renderer/native_tu15_item_light_common.hpp"

namespace halo2::native_item_light_detail {
namespace item_light_place {
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_ITEM_LIGHT_PLACE");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_ITEM_LIGHT_PLACE");return v;}
enum Branch : unsigned {skipped, placed, refreshed, branch_count};
static_assert(unsigned(logic::item_light::Outcome::refreshed)==Branch::refreshed);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Item light place"};return v;}
// record, flags, items, light, off, x, x_store, y, y_store, z, z_store, w_store, light_again, settled, pending, object, table, function
inline constexpr Sites sites{{0x1d9089u,0u,0x1d908cu,0x1d9096u,0x1d909au,0x1d90a1u,0x1d90a5u,0x1d90abu,0x1d90b0u,0x1d90b6u,0x1d90bfu,0x1d90cbu,0u,0x1d90d6u,0x1d90dbu,0x1d90e9u,0x1d90ecu,0x1d90f3u},0x1d9095u,0x1d90c8u,0x1d90d1u,0x1d90e4u,0x1d90f2u,0x1d90f3u,0x1d90f6u,0x1d9080u};
template<class State,class Reference,class Settled,class Refresh,class Route>
bool bridge(State &s,Reference reference,Settled settled,Refresh refresh,Route route) {
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    std::uint32_t *cursor;
    if(!native_object_tick_detail::admitted(s,0x1d9080u,0x1d90fau,cursor,reference_mode()))return false;
    native_object_tick_detail::execute(s,cursor,verify_mode(),coverage(),summary().calls,summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){
            struct Entry : Adapter<State,Settled,Refresh,Route> {
                using Adapter<State,Settled,Refresh,Route>::Adapter;
                void run(){this->reached(this->run_entry());}
            };
            return Entry(v,c,t,sites,logic::item_light::placing,settled,refresh,route);
        });
    summary().hit=coverage().load();
    return true;
}
} // namespace item_light_place
} // namespace halo2::native_item_light_detail
