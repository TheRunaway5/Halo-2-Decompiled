#include "halo2/physics_game_frame_runtime.hpp"

namespace halo2 {
void source_run_game_physics_frame(
    SourcePhysicsContactMemory &memory, SourcePhysicsWorldRuntimeAbi &abi,
    SourcePhysicsContactImage image, SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices &services,
    const SourcePhysicsContactRecordObserver &observe) {
    source_run_physics_world_runtime(memory, 0x1c7660u, abi, image, fp,
                                     services, observe);
}
} // namespace halo2
