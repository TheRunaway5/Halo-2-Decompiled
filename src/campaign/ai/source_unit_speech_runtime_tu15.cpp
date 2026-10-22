#include "halo2/source_unit_speech_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <bit>
namespace halo2 {
void source_unit_speech_tu15(PhysicsLiveWorldTu15 &world,std::uint32_t object,
    std::uint32_t sound,std::int32_t pitch,std::int32_t permutation,float elapsed) {
    auto abi=world.prepare_source_call(std::array{object,std::uint32_t(pitch),
        std::uint32_t(permutation),std::bit_cast<std::uint32_t>(elapsed)});
    abi.gp[7]=sound;world.run_actual(0x18f510u,abi);
}
void source_unit_speech_stop_tu15(PhysicsLiveWorldTu15 &world,std::uint32_t object,std::uint32_t sound) {
    auto check=world.prepare_source_call(std::array{3u});check.gp[0]=object;
    world.run_actual(0xbcdf0u,check);if(!check.gp[0])return;
    auto abi=world.prepare_source_call(std::array{sound});abi.gp[0]=object;
    world.run_actual(0xd3300u,abi);
}
}
