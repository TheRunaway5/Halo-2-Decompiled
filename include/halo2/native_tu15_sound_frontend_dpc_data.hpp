#pragma once
namespace halo2 {
class SourceRuntimeMemory;
// Authenticated immutable SDK vtable/default PCM route operands only. No
// source object, readiness, mutable listener or notification is preset.
// Reuse only fully covered immutable equal operands, including subextents of
// earlier SDK tables. Partial/mutable/changed rows are rejected; cold assembly
// rolls back only rows it has just added. Call on a quiescent source bank.
void mount_native_tu15_sound_frontend_dpc_data(SourceRuntimeMemory&);
}
