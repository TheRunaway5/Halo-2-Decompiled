#pragma once
#include "halo2/physics_contact_batches.hpp"
namespace halo2 {
struct SourcePhysicsContactPacketFrame {
    std::uint32_t instruction;
    std::uint32_t entry;
    std::uint32_t entry_esp;
    std::uint32_t descriptor;
    std::uint32_t packet_input;
    std::uint32_t packet_bytes;
};
struct SourcePhysicsContactDispatchServices {
    SourcePhysicsBspChildServices shape_and_child;
    SourcePhysicsGjkServices gjk;
    SourcePhysicsContactBatchServices contact;
    std::function<std::uint64_t()> timestamp_counter;
    // 300400 copies retained guest frame bytes, including actual preceding
    // callee ABI stores. Semantic lower kernels do not produce those slots.
    // A live adapter must execute that owner; captured packet replay is not
    // a provider. With no owner, dispatch stops before append/count mutation.
    std::function<void(SourcePhysicsContactMemory &,
        const SourcePhysicsContactPacketFrame &,
        const SourcePhysicsContactRecordObserver &)> packet_frame_owner;
};
// Complete300870 body translated. The actual cold empty call and positive
// prefix are verified; the positive tail requires the unported packet frame.
// Composes actual child-cache/BSP shapes,
// narrowphase/GJK and packet retention; no completed workspace/contact replay.
// The platform estimate is explicit (Unicorn test estimates are not hardware
// evidence). Missing world/manager/solver/storage providers stop at their owner.
void source_dispatch_physics_contacts_1_1(SourcePhysicsContactMemory &,
    std::uint32_t parent,const std::array<std::uint32_t,4> &,
    std::uint32_t entry_esp,SourcePhysicsFactoryFloatingPoint,
    SourcePhysicsReciprocalSqrtEstimate,
    const SourcePhysicsContactDispatchServices & = {},
    const SourcePhysicsContactRecordObserver & = {});
} // namespace halo2
