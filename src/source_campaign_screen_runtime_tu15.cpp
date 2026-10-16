#include "halo2/source_campaign_screen_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include <bit>
namespace halo2 {
namespace {
std::uint32_t bank(const SourceRuntimeMemory &m) {return Bytes(m.view(0x515ec0u,4)).u32(0);}
template<class T>void put(SourceRuntimeMemory &m,std::uint32_t at,const T &v) {
    m.write(at,std::as_bytes(std::span(&v,1)));
}
}
void source_campaign_screen_start_tu15(SourceRuntimeMemory &m,bool reset) {
    const auto p=bank(m);if(!p)return;
    if(reset || m.view(p+0xadu,1)[0]==std::byte{}) {
        const std::array<std::byte,0xac> clear{};m.write(p,clear);
        put(m,p+0xadu,std::uint8_t{1});
    }
    put(m,p+0xacu,std::uint8_t{1});
}
void source_campaign_crossfade_set_tu15(SourceRuntimeMemory &m,float seconds,float strength) {
    const auto p=bank(m);if(!p)return;
    const Bytes clock_bytes(m.view(0x4920c0u,8));
    const auto clock=std::bit_cast<double>(std::uint64_t(clock_bytes.u32(0))|
        (std::uint64_t(clock_bytes.u32(4))<<32));
    // Original FLD/FST/FADD/FSTP rounds each published endpoint to float32,
    // retaining the original double clock for the end-time addition.
    put(m,p+0x170u,std::uint8_t{1});put(m,p+0x174u,strength);
    put(m,p+0x178u,float(clock));put(m,p+0x17cu,float(clock+double(seconds)));
}
void source_campaign_screen_clock_tu15(SourceRuntimeMemory &m,double clock) {
    //13AA2D..13AA38 owns the accepted elapsed accumulator;2B5B0/12A30
    // publishes that double at the beginning of the native render phase.
    put(m,0x4c6860u,clock);put(m,0x4920c0u,clock);
}
void source_campaign_screen_update_tu15(PhysicsLiveWorldTu15 &w) {w.call_actual(0x1fd00u);}
void source_campaign_screen_stop_tu15(PhysicsLiveWorldTu15 &w) {w.call_actual(0x1fcb0u);}
void source_campaign_screen_capture_complete_tu15(PhysicsLiveWorldTu15 &w) {w.call_actual(0x1fce0u);}
CinematicCrossfadeState source_campaign_crossfade_state_tu15(const SourceRuntimeMemory &m) {
    const auto p=bank(m);if(!p)return {};
    const auto raw=m.view(p,0x3fcu);const Bytes b(raw);
    return {raw[0xac]!=std::byte{},raw[0xad]!=std::byte{},raw[0x170]!=std::byte{},raw[0xa0]!=std::byte{},
        b.f32(0x174),b.f32(0x178),b.f32(0x17c),b.f32(0xa4),b.f32(0xa8)};
}
CinematicCrossfadeFrame source_campaign_crossfade_frame_tu15(const SourceRuntimeMemory &m) {
    return source_campaign_crossfade_state_tu15(m).frame();
}
}
