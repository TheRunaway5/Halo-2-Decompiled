#include "halo2/source_scripted_sound_point_tu15.hpp"
#include "halo2/physics_live_world_tu15.hpp"
namespace halo2 {
Vec3 source_scripted_sound_point_tu15(PhysicsLiveWorldTu15 &world,std::uint32_t object) {
    // Caller scratch above the retained stack cannot alias BAD60's downward
    // frames. All consumed output bytes are written by the original owner.
    const auto output=world.prepare_source_call().gp[4]+0x100u;
    auto abi=world.prepare_source_call(std::array{object,0x04000095u,1u});
    abi.gp[3]=output;abi.gp[1]&=~255u;
    world.run_actual(0xbad60u,abi);
    const Bytes marker(world.memory().view(output,0x70));
    // BAD60's record contains local and WORLD matrices.18C090 retains the
    // local point with its object/node owner; the native mixer consumes the
    // completed WORLD point rather than discarding that association.
    return {marker.f32(0x60),marker.f32(0x64),marker.f32(0x68)};
}
}
