#pragma once
#include "halo2/native_tu15_constraint_schema_emitter.hpp"
namespace halo2 {
struct NativeTu15ConstraintProducer {std::uint32_t identity{};};
struct NativeTu15ConstraintProducerInput {std::uint32_t identity{};};
struct NativeTu15ConstraintBaseBuildRequest {
  std::uint32_t row_data{},row_count{},runtime{},frame{};
  NativeTu15ConstraintProducerInput input;
  NativeTu15ConstraintSchemaPacket builder;
};
struct NativeTu15ConstraintProducerServices {
  // Actual329720, cdecl6. The service performs genuine construction; no
  // recorded row, cursor or success value is supplied by this API.
  std::function<void(const NativeTu15ConstraintBaseBuildRequest&,NativeTu15PhysicsFrameFp&)>build_base;
};
struct NativeTu15ConstraintProducerStats {
  std::uint64_t axis_factor_calls{},scaled_calls{},base_calls{},property_lookups{};
  std::array<std::uint64_t,3>emissions{};
};
// Actual1D7630: first property2003 value, or0. Count is cached, signed, and
// source records are read lazily. No host lookup table changes source order.
std::uint32_t native_tu15_constraint_axis_factors(SourcePhysicsContactMemory&,std::uint32_t properties_owner);
// Actual3296E0, including the original header word/store order.
void native_tu15_begin_constraint_schemas(SourcePhysicsContactMemory&,NativeTu15ConstraintProducerInput,
  NativeTu15ConstraintSchemaPacket,const SourcePhysicsContactRecordObserver& = {});
// Actual325EF0, exposing the real six-argument base owner as a typed service.
void native_tu15_build_base_constraint(SourcePhysicsContactMemory&,NativeTu15ConstraintProducer,
  NativeTu15ConstraintProducerInput,NativeTu15ConstraintSchemaPacket,NativeTu15PhysicsFrameFp&,
  const NativeTu15ConstraintProducerServices&,NativeTu15ConstraintProducerStats* = nullptr);
// Actual31F770/31F860. Inputs are ordinary native value parameters; private
// cached factors and saved accumulator words are caller-private locals, not
// an aliased guest control frame. The original partial-store/exception order
// is retained: accumulator scaling is restored only on the completed path.
void native_tu15_build_axis_factor_constraint(SourcePhysicsContactMemory&,NativeTu15ConstraintProducer,
  NativeTu15ConstraintProducerInput,NativeTu15ConstraintSchemaPacket,NativeTu15PhysicsFrameFp&,
  unsigned preserved_x87_depth,const NativeTu15ConstraintProducerServices&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15ConstraintProducerStats* = nullptr);
void native_tu15_build_scaled_accumulator_constraint(SourcePhysicsContactMemory&,NativeTu15ConstraintProducer,
  NativeTu15ConstraintProducerInput,NativeTu15ConstraintSchemaPacket,NativeTu15PhysicsFrameFp&,
  unsigned preserved_x87_depth,const NativeTu15ConstraintProducerServices&,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15ConstraintProducerStats* = nullptr);
} // namespace halo2
