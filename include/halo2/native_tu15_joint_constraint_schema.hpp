#pragma once
#include "halo2/native_tu15_constraint_producer.hpp"
namespace halo2 {
// Actual328870, with its current schema/Jacobian cursors read before the two
// body-input words, followed by source-ordered six header stores/publication.
// Runtime identity/size are ordinary native value parameters. Source31D930
// discards the volatile return; only initialized persistent DATA is exposed.
void native_tu15_begin_joint_constraint_schemas(SourcePhysicsContactMemory&,
    NativeTu15ConstraintProducerInput,NativeTu15ConstraintSchemaPacket,
    std::uint32_t runtime,std::uint32_t runtime_bytes,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
