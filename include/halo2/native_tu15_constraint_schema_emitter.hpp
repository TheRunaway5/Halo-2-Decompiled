#pragma once
#include "halo2/native_tu15_constraint_step.hpp"
namespace halo2 {
struct NativeTu15ConstraintSchemaPacket {std::uint32_t identity{};};
struct NativeTu15ConstraintSchemaEmitterStats {std::uint64_t calls{};};
// Actual32A390's callback argument is read after header publication. The
// readonly source retains that actual input timing, including argument/header
// aliases. It supplies a callback identity, never a callback result.
using NativeTu15ConstraintSchemaCallbackSource=std::function<NativeTu15PhysicsVirtualOwner()>;
using NativeTu15ConstraintSchemaPhasePublisher=std::function<void(std::uint32_t actual_instruction)>;
// Proper native schema14 emitter. The early payload size and packet identity
// are the actual second/fourth cdecl inputs; the third input is unused. The
// return is the source's actual schema payload pointer. Publishing an unknown
// callback identity does not qualify or execute that callback.
std::uint32_t native_tu15_emit_custom_constraint_schema(SourcePhysicsContactMemory&,
    std::uint32_t payload_bytes,NativeTu15ConstraintSchemaPacket,
    const NativeTu15ConstraintSchemaCallbackSource&,
    const SourcePhysicsContactRecordObserver& = {},
    NativeTu15ConstraintSchemaEmitterStats* = nullptr,
    const NativeTu15ConstraintSchemaPhasePublisher& = {});
} // namespace halo2
