#include "halo2/pstencil/player_detail.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <algorithm>
namespace halo2::pstencil {
PlayerDetail::PlayerDetail(PhysicsLiveWorldTu15& world,std::span<const std::uint32_t> players,bool enabled):world_(world) {
    if(!enabled)return;
    saved_.reserve(players.size());
    for(const auto owner:players) {
        if(owner==0xffffffffu || std::ranges::any_of(saved_,[&](const auto& p){return p.first==owner;}))continue;
        const auto body=world_.source_object_body(owner);
        const auto flags=Bytes(world_.memory().view(body+4,4)).u32(0);
        saved_.push_back({owner,flags});
    }
    // Validate every full generation before the first write.
    for(const auto [owner,flags]:saved_) {
        const auto high=flags|0x80000u;
        world_.write_source_call_input(world_.source_object_body(owner)+4,std::as_bytes(std::span(&high,1)));
    }
}
PlayerDetail::~PlayerDetail() {
    for(const auto [owner,flags]:saved_) {
        const auto body=world_.source_object_body(owner);
        const auto current=Bytes(world_.memory().view(body+4,4)).u32(0);
        const auto restored=(current&~0x80000u)|(flags&0x80000u);
        world_.write_source_call_input(body+4,std::as_bytes(std::span(&restored,1)));
    }
}
}
