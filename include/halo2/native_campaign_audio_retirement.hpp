#pragma once
#include <cstdint>
namespace halo2 {
// Release the low-seven-bit controller reference count while preserving its
// upper flag. A zero new count deletes that actual salted pool entry.
template<class State> void native_campaign_release_controller(State& state,std::uint32_t datum) {
    if(datum==0xffffffffu)return;
    auto pool=state.controllers();auto controller=pool.row(datum);const auto previous=controller.references();
    const auto remaining=std::uint8_t((previous&0x80u)|((previous-1u)&0x7fu));controller.references(remaining);
    if(!(remaining&0x7fu))pool.erase(datum);
}
// Final source-retirement phase. Views retain their actual pool owners, but
// the controller datum and the release helper's pool owner are reread after
// the preceding byte publication. No callback or driver result is supplied.
template<class State> void native_campaign_retire_source(State& state,std::uint32_t datum) {
    auto pool=state.sources();auto source=pool.row(datum);const auto reference=source.controller();
    if(reference!=0xffffffffu) {
        auto controllers=state.controllers();auto controller=controllers.row(reference);
        if(source.category()==0xffu && controller.owner()!=0xffffffffu) {
            const auto previous=controller.side_references();controller.side_references(std::uint8_t((previous-1u)&0x7fu));
        }
        const auto after_publication=source.controller();native_campaign_release_controller(state,after_publication);
    }
    pool.erase(datum);
}
} // namespace halo2
