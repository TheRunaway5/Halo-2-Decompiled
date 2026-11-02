#pragma once
#include "halo2/native_tu15_constraint_producer.hpp"
namespace halo2 {
using NativeTu15ConstraintVector4=std::array<std::uint32_t,4>;
struct NativeTu15JointConstraintAngularRow {
    NativeTu15ConstraintVector4 first_axis,second_axis,axis;
};
// Actual3288B0. Numeric row identities refer to ordinary48B arithmetic DATA;
// the native parent can instead pass initialized host values below. No row
// identity escapes into a schema or a reconstructed guest control frame.
void native_tu15_emit_joint_constraint_angular_row(SourcePhysicsContactMemory&,
    std::uint32_t row,NativeTu15ConstraintProducerInput,NativeTu15ConstraintSchemaPacket,
    NativeTu15PhysicsFrameFp&,unsigned actual_preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {});
// Same semantic kernel with real native x87/SSE environment already active.
// The caller owns that environment and supplies initialized typed row DATA.
void native_tu15_emit_joint_constraint_angular_row_active(SourcePhysicsContactMemory&,
    const NativeTu15JointConstraintAngularRow&,NativeTu15ConstraintProducerInput,
    NativeTu15ConstraintSchemaPacket,unsigned actual_preserved_x87_depth,
    const SourcePhysicsContactRecordObserver& = {});
} // namespace halo2
