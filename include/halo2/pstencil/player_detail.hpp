#pragma once
#include <cstdint>
#include <span>
#include <utility>
#include <vector>
namespace halo2 {class PhysicsLiveWorldTu15;}
namespace halo2::pstencil {
// Rendering-only quality input. Native4BCC0 selects the authored LOD5 for
// bit19. Restore actual object flags before leaving the model-view callback.
class PlayerDetail {
public:
    PlayerDetail(PhysicsLiveWorldTu15&,std::span<const std::uint32_t> players,bool enabled);
    ~PlayerDetail();
    PlayerDetail(const PlayerDetail&)=delete;
    PlayerDetail& operator=(const PlayerDetail&)=delete;
private:
    PhysicsLiveWorldTu15& world_;
    std::vector<std::pair<std::uint32_t,std::uint32_t>> saved_;
};
}
